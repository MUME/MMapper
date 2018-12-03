/************************************************************************
**
** Authors:   Azazello <lachupe@gmail.com>,
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "GroupServer.h"

#include <QAbstractSocket>
#include <QByteArray>
#include <QHostAddress>
#include <QList>
#include <QMessageLogContext>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include "../configuration/configuration.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "CGroupCommunicator.h"
#include "GroupSocket.h"
#include "groupaction.h"
#include "groupauthority.h"
#include "groupselection.h"
#include "mmapper2group.h"

using Messages = CGroupCommunicator::Messages;

GroupTcpServer::GroupTcpServer(GroupServer *parent)
    : QTcpServer(parent)
{}

GroupTcpServer::~GroupTcpServer() = default;

void GroupTcpServer::incomingConnection(qintptr socketDescriptor)
{
    emit signal_incomingConnection(socketDescriptor);
}

void GroupServer::onIncomingConnection(qintptr socketDescriptor)
{
    // connect the socket straight to the Communicator, as he handles all the state changes
    // data transfers and similar.
    auto *socket = new GroupSocket(getAuthority(), this);
    clientsList.append(socket);
    connectAll(socket);

    socket->setSocket(socketDescriptor);
    qDebug() << "Adding incoming client" << socket->getPeerAddress().toString();
}

void GroupServer::errorInConnection(GroupSocket *const socket, const QString &errorMessage)
{
    const auto &name = socket->getName();
    if (getGroup()->isNamePresent(name)) {
        sendRemoveUserNotification(socket, name);
        emit sendLog(QString("'%1' encountered an error: %2")
                         .arg(socket->getName().constData())
                         .arg(errorMessage));
        emit scheduleAction(new RemoveCharacter(name));
    }
    closeOne(socket);
}

void GroupServer::sendToAll(const QByteArray &message)
{
    sendToAllExceptOne(nullptr, message);
}

void GroupServer::sendToAllExceptOne(GroupSocket *const exception, const QByteArray &message)
{
    for (auto &connection : clientsList) {
        if (connection == exception)
            continue;
        if (connection->getProtocolState() == ProtocolState::Logged) {
            connection->sendData(message);
        }
    }
}

void GroupServer::closeAll()
{
    for (auto &connection : clientsList) {
        if (connection != nullptr) {
            connection->disconnectFromHost();
            disconnectAll(connection);
            connection->deleteLater();
        }
    }
    clientsList.clear();
}

void GroupServer::closeOne(GroupSocket *const socket)
{
    if (clientsList.removeOne(socket)) {
        socket->disconnectFromHost();
        disconnectAll(socket);
        socket->deleteLater();
    }
}

void GroupServer::connectAll(GroupSocket *const client)
{
    connect(client, &GroupSocket::incomingData, this, &GroupServer::incomingData);
    connect(client, &GroupSocket::connectionEstablished, this, &GroupServer::connectionEstablished);
    connect(client, &GroupSocket::connectionClosed, this, &GroupServer::connectionClosed);
    connect(client, &GroupSocket::errorInConnection, this, &GroupServer::errorInConnection);
}

void GroupServer::disconnectAll(GroupSocket *const client)
{
    disconnect(client, &GroupSocket::incomingData, this, &GroupServer::incomingData);
    disconnect(client,
               &GroupSocket::connectionEstablished,
               this,
               &GroupServer::connectionEstablished);
    disconnect(client, &GroupSocket::connectionClosed, this, &GroupServer::connectionClosed);
    disconnect(client, &GroupSocket::errorInConnection, this, &GroupServer::errorInConnection);
}

//
// ******************** S E R V E R   S I D E ******************
//
// Server side of the communication protocol
GroupServer::GroupServer(Mmapper2Group *parent)
    : CGroupCommunicator(GroupManagerState::Server, parent)
    , server(this)
{
    connect(&server,
            &GroupTcpServer::signal_incomingConnection,
            this,
            &GroupServer::onIncomingConnection);
    connect(getAuthority(), &GroupAuthority::secretRevoked, this, &GroupServer::onRevokeWhitelist);
    emit sendLog("Server mode has been selected");
    start();
}

GroupServer::~GroupServer()
{
    disconnect(getAuthority(),
               &GroupAuthority::secretRevoked,
               this,
               &GroupServer::onRevokeWhitelist);
    closeAll();
}

void GroupServer::connectionClosed(GroupSocket *const socket)
{
    const auto &name = socket->getName();
    if (getGroup()->isNamePresent(name)) {
        sendRemoveUserNotification(socket, name);
        emit sendLog(QString("'%1' closed their connection and quit.").arg(QString(name)));
        emit scheduleAction(new RemoveCharacter(name));
    }
    closeOne(socket);
}

void GroupServer::serverStartupFailed()
{
    emit messageBox(
        QString("Failed to start the groupManager server: %1.").arg(server.errorString()));
    stop();
}

void GroupServer::connectionEstablished(GroupSocket *const socket)
{
    QVariantMap handshake;
    handshake["protocolVersion"] = NO_OPEN_SSL ? PROTOCOL_VERSION_102 : PROTOCOL_VERSION_103;
    sendMessage(socket, Messages::REQ_HANDSHAKE, handshake);
}

void GroupServer::retrieveData(GroupSocket *const socket,
                               const Messages message,
                               const QVariantMap &data)
{
    // ---------------------- AwaitingLogin  --------------------
    if (socket->getProtocolState() == ProtocolState::AwaitingLogin) {
        // Login state. either REQ_HANDSHAKE, UPDATE_CHAR, or ACK should come
        if (message == Messages::REQ_HANDSHAKE) {
            // Shaking hands with client
            parseHandshake(socket, data);
        } else if (message == Messages::UPDATE_CHAR) {
            // aha! parse the data
            if (socket->getProtocolVersion() >= PROTOCOL_VERSION_103)
                parseLoginInformation(socket, data);
            else
                parseHandshake(socket, data); // Protocol 102 skips the handshake
        } else {
            // ERROR: unexpected message marker!
            // try to ignore?
            qWarning("(AwaitingLogin) Unexpected message marker. Trying to ignore.");
        }
        // ---------------------- AwaitingInfo  --------------------
    } else if (socket->getProtocolState() == ProtocolState::AwaitingInfo) {
        // almost connected. awaiting full information about the connection
        if (message == Messages::REQ_INFO) {
            sendGroupInformation(socket);
            sendMessage(socket, Messages::REQ_ACK);
        } else if (message == Messages::ACK) {
            socket->setProtocolState(ProtocolState::Logged);
            emit sendLog(
                QString("'%1' has successfully logged in.").arg(socket->getName().constData()));
            sendMessage(socket, Messages::STATE_LOGGED);
            if (!NO_OPEN_SSL && socket->getProtocolVersion() == PROTOCOL_VERSION_102) {
                QVariantMap root;
                root["text"] = QString("WARNING: %1 joined the group with an insecure connection "
                                       "and needs to upgrade MMapper!")
                                   .arg(socket->getName().constData());
                root["from"] = "MMapper";
                sendGroupTellMessage(root);
                emit gTellArrived(root);
            }
        } else {
            // ERROR: unexpected message marker!
            // try to ignore?
            qWarning("(AwaitingInfo) Unexpected message marker. Trying to ignore.");
        }

        // ---------------------- LOGGED --------------------
    } else if (socket->getProtocolState() == ProtocolState::Logged) {
        // usual update situation. receive update, unpack, apply.
        if (message == Messages::UPDATE_CHAR) {
            const QString &updateName = CGroupChar::getNameFromUpdateChar(data);
            if (updateName.compare(socket->getName(), Qt::CaseInsensitive) != 0) {
                emit sendLog(QString("WARNING: '%1' spoofed as '%2'")
                                 .arg(socket->getName().constData())
                                 .arg(updateName));
                return;
            }
            emit scheduleAction(new UpdateCharacter(data));
            relayMessage(socket, Messages::UPDATE_CHAR, data);

        } else if (message == Messages::GTELL) {
            const auto &fromName = data["from"].toString().simplified();
            if (fromName.compare(socket->getName(), Qt::CaseInsensitive) != 0) {
                emit sendLog(QString("WARNING: '%1' spoofed as '%2'")
                                 .arg(socket->getName().constData())
                                 .arg(fromName));
                return;
            }
            emit gTellArrived(data);
            relayMessage(socket, Messages::GTELL, data);

        } else if (message == Messages::REQ_ACK) {
            sendMessage(socket, Messages::ACK);

        } else if (message == Messages::RENAME_CHAR) {
            const QString oldName = data["oldname"].toString().simplified();
            if (oldName.compare(socket->getName(), Qt::CaseInsensitive) != 0) {
                kickConnection(socket,
                               QString("Name spoof detected: %1 != %2")
                                   .arg(oldName)
                                   .arg(socket->getName().constData()));
                return;
            }
            const QString &newName = data["newname"].toString().simplified();
            if (newName.compare(data["newname"].toByteArray(), Qt::CaseInsensitive) != 0) {
                kickConnection(socket, "Your name must not include any whitespace");
                return;
            }
            for (auto &target : clientsList) {
                if (socket == target)
                    continue;
                if (newName.compare(target->getName(), Qt::CaseInsensitive) != 0) {
                    kickConnection(socket, "Someone was already using that name");
                    return;
                }
            }
            socket->setName(newName.toLatin1());
            emit scheduleAction(new RenameCharacter(data));
            relayMessage(socket, Messages::RENAME_CHAR, data);

        } else {
            // ERROR: unexpected message marker!
            // try to ignore?
            qWarning("(Logged) Unexpected message marker. Trying to ignore.");
        }
    }
}

void GroupServer::sendCharUpdate(const QVariantMap &map)
{
    if (getConfig().groupManager.shareSelf) {
        const QByteArray &message = formMessageBlock(Messages::UPDATE_CHAR, map);
        sendToAll(message);
    }
}

void GroupServer::parseHandshake(GroupSocket *const socket, const QVariantMap &data)
{
    if (!data.contains("protocolVersion") || !data["protocolVersion"].canConvert(QMetaType::UInt)) {
        kickConnection(socket, "Payload did not include the 'protocolVersion' attribute");
        return;
    }
    const ProtocolVersion clientProtocolVersion = data["protocolVersion"].toUInt();
    if (clientProtocolVersion < PROTOCOL_VERSION_102) {
        kickConnection(socket,
                       "Host requires a newer version of the group protocol. "
                       "Please upgrade to the latest MMapper.");
        return;
    }
    if (getConfig().groupManager.requireAuth && clientProtocolVersion == PROTOCOL_VERSION_102) {
        kickConnection(socket,
                       "Host requires authorization. "
                       "Please upgrade to the latest MMapper.");
        return;
    }
    auto supportedProtocolVersion = NO_OPEN_SSL ? PROTOCOL_VERSION_102 : PROTOCOL_VERSION_103;
    if (clientProtocolVersion > supportedProtocolVersion) {
        kickConnection(socket, "Host uses an older version of MMapper and needs to upgrade.");
        return;
    }
    if (clientProtocolVersion == PROTOCOL_VERSION_102) {
        socket->setProtocolVersion(clientProtocolVersion);
        parseLoginInformation(socket, data);
    } else {
        assert(!NO_OPEN_SSL);
        sendMessage(socket, Messages::REQ_LOGIN);
        socket->setProtocolVersion(clientProtocolVersion);
        socket->startServerEncrypted();
    }
}

void GroupServer::parseLoginInformation(GroupSocket *socket, const QVariantMap &data)
{
    if (!data.contains("playerData") || !data["playerData"].canConvert(QMetaType::QVariantMap)) {
        kickConnection(socket, "Payload did not include 'playerData' element.");
        return;
    }
    const QVariantMap &playerData = data["playerData"].toMap();
    if (!playerData.contains("name") || !playerData["name"].canConvert(QMetaType::QByteArray)) {
        kickConnection(socket, "Payload did not include 'name' attribute.");
        return;
    }
    const QByteArray name = playerData["name"].toByteArray();
    const QString tempName = QString("%1-%2").arg(name.constData()).arg(rand() % 1000); //NOLINT
    socket->setName(tempName.toLatin1());
    emit sendLog(
        QString("'%1' is trying to join the group as '%2'.").arg(tempName).arg(name.constData()));

    // Check credentials
    const auto secret = socket->getSecret();
    const bool isEncrypted = !secret.isEmpty()
                             && socket->getProtocolVersion() >= PROTOCOL_VERSION_103;
    const bool requireAuth = getConfig().groupManager.requireAuth;
    const bool validSecret = getAuthority()->validSecret(secret);
    const bool validCert = getAuthority()->validCertificate(socket);
    if (isEncrypted) {
        emit sendLog(QString("'%1's secret: %2").arg(tempName).arg(secret.constData()));

        // Verify only one secret can be connected at once
        QString secretStr = QString::fromLatin1(socket->getSecret());
        for (auto &target : clientsList) {
            if (socket == target)
                continue;
            if (secretStr.compare(target->getSecret(), Qt::CaseInsensitive) == 0) {
                if (!requireAuth || validCert) {
                    kickConnection(target, "Someone reconnected to the server using your secret!");
                    return;
                } else {
                    kickConnection(socket, "Host does not trust your compromised secret.");
                    return;
                }
            }
        }

    } else {
        emit sendLog(
            QString("<b>WARNING:</b> '%1' has no secret and their connection is not encrypted.")
                .arg(tempName));
    }
    emit sendLog(
        QString("'%1's IP address: %2").arg(tempName).arg(socket->getPeerAddress().toString()));
    emit sendLog(
        QString("'%1's protocol version: %2").arg(tempName).arg(socket->getProtocolVersion()));
    if (requireAuth && !validSecret) {
        kickConnection(socket, "Host has not added your secret to their contacts!");
        return;
    }
    if (isEncrypted && requireAuth && !validCert) {
        kickConnection(socket, "Host does not trust your compromised secret.");
        return;
    }
    QString simplifiedName = QString(name).simplified();
    if (simplifiedName.compare(name, Qt::CaseInsensitive) != 0) {
        kickConnection(socket, "Your name must not include any whitespace");
        return;
    }
    if (getGroup()->isNamePresent(name)) {
        kickConnection(socket, "The name you picked is already present!");
        return;
    } else {
        // Allow this name to now take effect
        emit sendLog(QString("'%1' will now be known as '%2'").arg(tempName).arg(name.constData()));
        socket->setName(name);
    }

    // Client is allowed to log in
    if (isEncrypted && validSecret) {
        // Update metadata
        getAuthority()->setMetadata(secret, GroupMetadata::NAME, name);
        getAuthority()->setMetadata(secret,
                                    GroupMetadata::IP_ADDRESS,
                                    socket->getPeerAddress().toString());
        getAuthority()->setMetadata(secret,
                                    GroupMetadata::LAST_LOGIN,
                                    QDateTime::currentDateTime().toString());
        getAuthority()->setMetadata(secret,
                                    GroupMetadata::CERTIFICATE,
                                    socket->getPeerCertificate().toPem());
    }

    // Strip protocolVersion from original QVariantMap
    QVariantMap charNode;
    charNode["playerData"] = playerData;
    emit scheduleAction(new AddCharacter(charNode));
    relayMessage(socket, Messages::ADD_CHAR, charNode);
    sendMessage(socket, Messages::ACK);
    socket->setProtocolState(ProtocolState::AwaitingInfo);
}

void GroupServer::sendGroupInformation(GroupSocket *const socket)
{
    GroupSelection *selection = getGroup()->selectAll();
    for (const auto &character : *selection) {
        // Only send group information for other characters
        if (character->getName() == socket->getName()) {
            continue;
        }
        // Only share self if we enabled it
        if (getGroup()->getSelf() == character && !getConfig().groupManager.shareSelf) {
            continue;
        }
        CGroupCommunicator::sendCharUpdate(socket, character->toVariantMap());
    }
    getGroup()->unselect(selection);
}

void GroupServer::sendRemoveUserNotification(GroupSocket *const socket, const QByteArray &name)
{
    GroupSelection *selection = getGroup()->selectByName(name);
    for (const auto &character : *selection) {
        if (character->getName() == name) {
            const QVariantMap &map = character->toVariantMap();
            const QByteArray &message = formMessageBlock(Messages::REMOVE_CHAR, map);
            sendToAllExceptOne(socket, message);
        }
    }
    getGroup()->unselect(selection);
}

void GroupServer::sendGroupTellMessage(const QVariantMap &root)
{
    const QByteArray &message = formMessageBlock(Messages::GTELL, root);
    sendToAll(message);
}

void GroupServer::relayMessage(GroupSocket *const socket,
                               const Messages message,
                               const QVariantMap &data)
{
    const QByteArray &buffer = formMessageBlock(message, data);
    sendToAllExceptOne(socket, buffer);
}

void GroupServer::sendCharRename(const QVariantMap &map)
{
    QByteArray message = formMessageBlock(Messages::RENAME_CHAR, map);
    sendToAll(message);
}

void GroupServer::stop()
{
    closeAll();
    emit scheduleAction(new ResetCharacters());
    deleteLater();
}

void GroupServer::start()
{
    const auto localPort = static_cast<quint16>(getConfig().groupManager.localPort);
    emit sendLog(QString("Listening on port %1").arg(localPort));
    if (!server.listen(QHostAddress::Any, localPort)) {
        emit sendLog("Failed to start a group Manager server");
        serverStartupFailed();
    }
}

bool GroupServer::kickCharacter(const QByteArray &name)
{
    if (getGroup()->getSelf()->getName() == name) {
        emit messageBox("You can't kick yourself!");
        return false;
    }
    for (auto &connection : clientsList) {
        if (connection->getName() == name) {
            kickConnection(connection, "You have been kicked by the host!");
            return true;
        }
    }
    return false;
}

void GroupServer::kickConnection(GroupSocket *const socket, const QString &message)
{
    if (socket->getProtocolVersion() == PROTOCOL_VERSION_102
        && socket->getProtocolState() != ProtocolState::AwaitingLogin) {
        // Protocol 102 does not support kicking outside of AwaitingLogin so we fake it with a group tell
        QVariantMap root;
        root["text"] = message;
        root["from"] = QString::fromLatin1(getConfig().groupManager.charName);
        sendMessage(socket, Messages::GTELL, root);

    } else {
        sendMessage(socket, Messages::STATE_KICKED, message.toLatin1());
    }
    QString identifier = socket->getName().isEmpty() ? socket->getPeerAddress().toString()
                                                     : socket->getName();
    qDebug() << "Kicking" << identifier << "for" << message;
    emit sendLog(QString("'%1' was kicked: %2").arg(identifier).arg(message));

    const auto &name = socket->getName();
    if (getGroup()->isNamePresent(name)) {
        sendRemoveUserNotification(socket, name);
        emit scheduleAction(new RemoveCharacter(name));
    }
    closeOne(socket);
}

void GroupServer::onRevokeWhitelist(const QByteArray &secret)
{
    if (getConfig().groupManager.requireAuth) {
        for (auto &connection : clientsList) {
            if (QString(secret).compare(connection->getSecret(), Qt::CaseInsensitive) == 0) {
                kickConnection(connection, "You have been removed from the host's contacts!");
            }
        }
    }
}
