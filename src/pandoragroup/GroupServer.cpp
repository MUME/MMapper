// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "GroupServer.h"

#include <algorithm>
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

using MessagesEnum = CGroupCommunicator::MessagesEnum;

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
    clientsList.emplace_back(QPointer<GroupSocket>(new GroupSocket(getAuthority(), this)));
    auto &socket = clientsList.back();
    connectAll(socket);
    socket->setSocket(socketDescriptor);
    qDebug() << "Adding incoming client" << socket->getPeerName();
}

void GroupServer::errorInConnection(GroupSocket *const socket, const QString &errorMessage)
{
    const auto &name = socket->getName();
    if (getGroup()->isNamePresent(name)) {
        sendRemoveUserNotification(socket, name);
        emit sendLog(QString("'%1' encountered an error: %2")
                         .arg(QString::fromLatin1(socket->getName()))
                         .arg(errorMessage));
        emit sig_scheduleAction(std::make_shared<RemoveCharacter>(name));
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
        if (connection->getProtocolState() == ProtocolStateEnum::Logged) {
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
        }
    }
    clientsList.clear();
}

void GroupServer::closeOne(GroupSocket *const target)
{
    target->disconnectFromHost();
    disconnectAll(target);

    auto it = std::find_if(clientsList.begin(), clientsList.end(), [&target](const auto &socket) {
        return socket == target;
    });
    if (it == clientsList.end()) {
        qWarning() << "Could not find" << target->getName() << "among clients";
        return assert(false);
    }
    clientsList.erase(it);
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
    : CGroupCommunicator(GroupManagerStateEnum::Server, parent)
    , server(this)
{
    connect(&server, &GroupTcpServer::acceptError, this, [this]() {
        emit sendLog(QString("Server encountered an error: %1").arg(server.errorString()));
    });
    connect(&server,
            &GroupTcpServer::signal_incomingConnection,
            this,
            &GroupServer::onIncomingConnection);
    connect(getAuthority(), &GroupAuthority::secretRevoked, this, &GroupServer::onRevokeWhitelist);
    emit sendLog("Server mode has been selected");
}

GroupServer::~GroupServer()
{
    disconnect(getAuthority(),
               &GroupAuthority::secretRevoked,
               this,
               &GroupServer::onRevokeWhitelist);
    closeAll();
    const auto localPort = static_cast<quint16>(getConfig().groupManager.localPort);
    if (portMapper.tryDeletePortMapping(localPort)) {
        emit sendLog("Deleted port mapping from UPnP IGD router");
    }
}

void GroupServer::connectionClosed(GroupSocket *const socket)
{
    const auto &name = socket->getName();
    if (getGroup()->isNamePresent(name)) {
        sendRemoveUserNotification(socket, name);
        emit sendLog(QString("'%1' closed their connection and quit.").arg(QString(name)));
        emit sig_scheduleAction(std::make_shared<RemoveCharacter>(name));
    }
    closeOne(socket);
}

void GroupServer::connectionEstablished(GroupSocket *const socket)
{
    QVariantMap handshake;
    handshake["protocolVersion"] = NO_OPEN_SSL ? PROTOCOL_VERSION_102 : PROTOCOL_VERSION_103;
    sendMessage(socket, MessagesEnum::REQ_HANDSHAKE, handshake);
}

void GroupServer::retrieveData(GroupSocket *const socket,
                               const MessagesEnum message,
                               const QVariantMap &data)
{
    const QString nameStr = QString::fromLatin1(socket->getName());
    // ---------------------- AwaitingLogin  --------------------
    if (socket->getProtocolState() == ProtocolStateEnum::AwaitingLogin) {
        // Login state. either REQ_HANDSHAKE, UPDATE_CHAR, or ACK should come
        if (message == MessagesEnum::REQ_HANDSHAKE) {
            // Shaking hands with client
            parseHandshake(socket, data);
        } else if (message == MessagesEnum::UPDATE_CHAR) {
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
    } else if (socket->getProtocolState() == ProtocolStateEnum::AwaitingInfo) {
        // almost connected. awaiting full information about the connection
        if (message == MessagesEnum::REQ_INFO) {
            sendGroupInformation(socket);
            sendMessage(socket, MessagesEnum::REQ_ACK);
        } else if (message == MessagesEnum::ACK) {
            socket->setProtocolState(ProtocolStateEnum::Logged);
            emit sendLog(QString("'%1' has successfully logged in.").arg(nameStr));
            sendMessage(socket, MessagesEnum::STATE_LOGGED);
            if (!NO_OPEN_SSL && socket->getProtocolVersion() == PROTOCOL_VERSION_102) {
                QVariantMap root;
                root["text"] = QString("WARNING: %1 joined the group with an insecure connection "
                                       "and needs to upgrade MMapper!")
                                   .arg(nameStr);
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
    } else if (socket->getProtocolState() == ProtocolStateEnum::Logged) {
        // usual update situation. receive update, unpack, apply.
        if (message == MessagesEnum::UPDATE_CHAR) {
            const QString &updateName = QString::fromLatin1(CGroupChar::getNameFromUpdateChar(data));
            if (updateName.compare(nameStr, Qt::CaseInsensitive) != 0) {
                emit sendLog(QString("WARNING: '%1' spoofed as '%2'").arg(nameStr).arg(updateName));
                return;
            }
            emit sig_scheduleAction(std::make_shared<UpdateCharacter>(data));
            relayMessage(socket, MessagesEnum::UPDATE_CHAR, data);

        } else if (message == MessagesEnum::GTELL) {
            const auto &fromName = QString::fromLatin1(data["from"].toByteArray()).simplified();
            if (fromName.compare(nameStr, Qt::CaseInsensitive) != 0) {
                emit sendLog(QString("WARNING: '%1' spoofed as '%2'").arg(nameStr).arg(fromName));
                return;
            }
            emit gTellArrived(data);
            relayMessage(socket, MessagesEnum::GTELL, data);

        } else if (message == MessagesEnum::REQ_ACK) {
            sendMessage(socket, MessagesEnum::ACK);

        } else if (message == MessagesEnum::RENAME_CHAR) {
            const QString oldName = QString::fromLatin1(data["oldname"].toByteArray()).simplified();
            if (oldName.compare(nameStr, Qt::CaseInsensitive) != 0) {
                kickConnection(socket,
                               QString("Name spoof detected: %1 != %2").arg(oldName).arg(nameStr));
                return;
            }
            const QString &newName = QString::fromLatin1(data["newname"].toByteArray()).simplified();
            if (newName.compare(data["newname"].toByteArray(), Qt::CaseInsensitive) != 0) {
                kickConnection(socket, "Your name must not include any whitespace");
                return;
            }
            for (const auto &target : clientsList) {
                if (socket == target)
                    continue;
                if (newName.compare(QString::fromLatin1(target->getName()), Qt::CaseInsensitive)
                    != 0) {
                    kickConnection(socket, "Someone was already using that name");
                    return;
                }
            }
            socket->setName(newName.toLatin1());
            emit sig_scheduleAction(std::make_shared<RenameCharacter>(data));
            relayMessage(socket, MessagesEnum::RENAME_CHAR, data);

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
        const QByteArray &message = formMessageBlock(MessagesEnum::UPDATE_CHAR, map);
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
        sendMessage(socket, MessagesEnum::REQ_LOGIN);
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
    if (!playerData.contains("name") || !playerData["name"].canConvert(QMetaType::QString)) {
        kickConnection(socket, "Payload did not include 'name' attribute.");
        return;
    }
    const QString &nameStr = playerData["name"].toString();
    const QString tempName = QString("%1-%2").arg(nameStr).arg(rand() % 1000); //NOLINT
    socket->setName(tempName.toLatin1());
    emit sendLog(QString("'%1' is trying to join the group as '%2'.").arg(tempName).arg(nameStr));

    // Check credentials
    const auto secret = socket->getSecret();
    const bool isEncrypted = !secret.isEmpty()
                             && socket->getProtocolVersion() >= PROTOCOL_VERSION_103;
    const bool requireAuth = getConfig().groupManager.requireAuth;
    const bool validSecret = getAuthority()->validSecret(secret);
    const bool validCert = getAuthority()->validCertificate(socket);
    bool reconnect = false;
    if (isEncrypted) {
        emit sendLog(QString("'%1's secret: %2").arg(tempName).arg(secret.constData()));

        // Verify only one secret can be connected at once
        for (auto &target : clientsList) {
            if (socket == target)
                continue;
            if (socket->getPeerCertificate() == target->getPeerCertificate()) {
                kickConnection(target, "Someone reconnected to the server using your secret!");
                reconnect = true;
                break;
            }
        }

    } else {
        emit sendLog(
            QString("<b>WARNING:</b> '%1' has no secret and their connection is not encrypted.")
                .arg(tempName));
    }
    emit sendLog(QString("'%1's IP address: %2").arg(tempName).arg(socket->getPeerName()));
    emit sendLog(
        QString("'%1's protocol version: %2").arg(tempName).arg(socket->getProtocolVersion()));
    if (requireAuth && !validSecret) {
        kickConnection(socket, "Host has not added your secret to their contacts!");
        return;
    }
    if (getConfig().groupManager.lockGroup && !reconnect) {
        kickConnection(socket, "Host has locked the group!");
        return;
    }
    if (isEncrypted && requireAuth && !validCert) {
        kickConnection(socket, "Host does not trust your compromised secret.");
        return;
    }
    QString simplifiedName = nameStr.simplified();
    if (simplifiedName.compare(nameStr, Qt::CaseInsensitive) != 0) {
        kickConnection(socket, "Your name must not include any whitespace");
        return;
    }
    if (getGroup()->isNamePresent(nameStr.toLatin1())) {
        kickConnection(socket, "The name you picked is already present!");
        return;
    } else {
        // Allow this name to now take effect
        emit sendLog(QString("'%1' will now be known as '%2'").arg(tempName).arg(nameStr));
        socket->setName(nameStr.toLatin1());
    }

    // Client is allowed to log in
    if (isEncrypted && validSecret) {
        // Update metadata
        getAuthority()->setMetadata(secret, GroupMetadataEnum::NAME, nameStr);
        getAuthority()->setMetadata(secret, GroupMetadataEnum::IP_ADDRESS, socket->getPeerName());
        getAuthority()->setMetadata(secret,
                                    GroupMetadataEnum::LAST_LOGIN,
                                    QDateTime::currentDateTime().toString());
        getAuthority()->setMetadata(secret,
                                    GroupMetadataEnum::CERTIFICATE,
                                    socket->getPeerCertificate().toPem());
    }

    // Strip protocolVersion from original QVariantMap
    QVariantMap charNode;
    charNode["playerData"] = playerData;
    emit sig_scheduleAction(std::make_shared<AddCharacter>(charNode));
    relayMessage(socket, MessagesEnum::ADD_CHAR, charNode);
    sendMessage(socket, MessagesEnum::ACK);
    socket->setProtocolState(ProtocolStateEnum::AwaitingInfo);
}

void GroupServer::sendGroupInformation(GroupSocket *const socket)
{
    auto selection = getGroup()->selectAll();
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
}

void GroupServer::sendRemoveUserNotification(GroupSocket *const socket, const QByteArray &name)
{
    auto selection = getGroup()->selectByName(name);
    for (const auto &character : *selection) {
        if (character->getName() == name) {
            const QVariantMap &map = character->toVariantMap();
            const QByteArray &message = formMessageBlock(MessagesEnum::REMOVE_CHAR, map);
            sendToAllExceptOne(socket, message);
        }
    }
}

void GroupServer::sendGroupTellMessage(const QVariantMap &root)
{
    const QByteArray &message = formMessageBlock(MessagesEnum::GTELL, root);
    sendToAll(message);
}

void GroupServer::relayMessage(GroupSocket *const socket,
                               const MessagesEnum message,
                               const QVariantMap &data)
{
    const QByteArray &buffer = formMessageBlock(message, data);
    sendToAllExceptOne(socket, buffer);
}

void GroupServer::sendCharRename(const QVariantMap &map)
{
    QByteArray message = formMessageBlock(MessagesEnum::RENAME_CHAR, map);
    sendToAll(message);
}

void GroupServer::stop()
{
    closeAll();
    emit sig_scheduleAction(std::make_shared<ResetCharacters>());
    deleteLater();
}

bool GroupServer::start()
{
    if (server.isListening()) {
        emit sendLog("Closing connections and restarting server...");
        server.setMaxPendingConnections(0);
        closeAll();
        server.close();
    }
    const auto localPort = static_cast<quint16>(getConfig().groupManager.localPort);
    if (portMapper.tryAddPortMapping(localPort)) {
        const QString externalIp = QString::fromLocal8Bit(portMapper.tryGetExternalIp());
        emit sendLog(
            QString("Added port mapping to UPnP IGD router with external IP: %1").arg(externalIp));
    }
    emit sendLog(QString("Listening on port %1").arg(localPort));
    if (!server.listen(QHostAddress::Any, localPort)) {
        emit sendLog("Failed to start a group Manager server");
        emit messageBox(
            QString("Failed to start the groupManager server: %1.").arg(server.errorString()));
        return false;
    }
    return true;
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
        && socket->getProtocolState() != ProtocolStateEnum::AwaitingLogin) {
        // Protocol 102 does not support kicking outside of AwaitingLogin so we fake it with a group tell
        QVariantMap root;
        root["text"] = message;
        root["from"] = QString::fromLatin1(getConfig().groupManager.charName);
        sendMessage(socket, MessagesEnum::GTELL, root);

    } else {
        sendMessage(socket, MessagesEnum::STATE_KICKED, message.toLatin1());
    }
    const QString nameStr = QString::fromLatin1(socket->getName());
    const QString identifier = nameStr.isEmpty() ? socket->getPeerName() : nameStr;
    qDebug() << "Kicking" << identifier << "for" << message;
    emit sendLog(QString("'%1' was kicked: %2").arg(identifier).arg(message));

    const auto &name = socket->getName();
    if (getGroup()->isNamePresent(name)) {
        sendRemoveUserNotification(socket, name);
        emit sig_scheduleAction(std::make_shared<RemoveCharacter>(name));
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
