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
#include <QSslSocket>
#include <QString>
#include <QVariantMap>

#include "../configuration/configuration.h"
#include "../global/random.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "CGroupCommunicator.h"
#include "GroupSocket.h"
#include "groupaction.h"
#include "groupauthority.h"
#include "groupselection.h"
#include "mmapper2group.h"

using MessagesEnum = CGroupCommunicator::MessagesEnum;

GroupTcpServer::GroupTcpServer(GroupServer *const parent)
    : QTcpServer(parent)
{}

GroupTcpServer::~GroupTcpServer() = default;

void GroupTcpServer::incomingConnection(qintptr socketDescriptor)
{
    emit signal_incomingConnection(socketDescriptor);
}

void GroupServer::slot_onIncomingConnection(qintptr socketDescriptor)
{
    // connect the socket straight to the Communicator, as he handles all the state changes
    // data transfers and similar.
    clientsList.emplace_back(QPointer<GroupSocket>(new GroupSocket(getAuthority(), this)));
    auto &socket = clientsList.back();
    connectAll(socket);
    socket->setSocket(socketDescriptor);
    qDebug() << "Adding incoming client" << socket->getPeerName();
}

void GroupServer::slot_errorInConnection(GroupSocket *const socket, const QString &errorMessage)
{
    const auto &name = socket->getName();
    if (getGroup()->isNamePresent(name)) {
        sendRemoveUserNotification(socket, name);
        emit sig_sendLog(QString("'%1' encountered an error: %2")
                             .arg(QString::fromLatin1(socket->getName()), errorMessage));
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
        assert(false);
        return;
    }
    clientsList.erase(it);
}

void GroupServer::connectAll(GroupSocket *const client)
{
    connect(client, &GroupSocket::sig_incomingData, this, &GroupServer::slot_incomingData);
    connect(client,
            &GroupSocket::sig_connectionEstablished,
            this,
            &GroupServer::slot_connectionEstablished);
    connect(client, &GroupSocket::sig_connectionClosed, this, &GroupServer::slot_connectionClosed);
    connect(client, &GroupSocket::sig_errorInConnection, this, &GroupServer::slot_errorInConnection);
}

void GroupServer::disconnectAll(GroupSocket *const client)
{
    disconnect(client, &GroupSocket::sig_incomingData, this, &GroupServer::slot_incomingData);
    disconnect(client,
               &GroupSocket::sig_connectionEstablished,
               this,
               &GroupServer::slot_connectionEstablished);
    disconnect(client,
               &GroupSocket::sig_connectionClosed,
               this,
               &GroupServer::slot_connectionClosed);
    disconnect(client,
               &GroupSocket::sig_errorInConnection,
               this,
               &GroupServer::slot_errorInConnection);
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
        emit sig_sendLog(QString("Server encountered an error: %1").arg(server.errorString()));
    });
    connect(&server,
            &GroupTcpServer::signal_incomingConnection,
            this,
            &GroupServer::slot_onIncomingConnection);
    connect(getAuthority(),
            &GroupAuthority::sig_secretRevoked,
            this,
            &GroupServer::slot_onRevokeWhitelist);
    emit sig_sendLog("Server mode has been selected");
}

GroupServer::~GroupServer()
{
    disconnect(getAuthority(),
               &GroupAuthority::sig_secretRevoked,
               this,
               &GroupServer::slot_onRevokeWhitelist);
    closeAll();
    const auto localPort = static_cast<quint16>(getConfig().groupManager.localPort);
    if (portMapper.tryDeletePortMapping(localPort)) {
        emit sig_sendLog("Deleted port mapping from UPnP IGD router");
    }
}

void GroupServer::virt_connectionClosed(GroupSocket *const socket)
{
    const auto &name = socket->getName();
    if (getGroup()->isNamePresent(name)) {
        sendRemoveUserNotification(socket, name);
        emit sig_sendLog(QString("'%1' closed their connection and quit.").arg(QString(name)));
        emit sig_scheduleAction(std::make_shared<RemoveCharacter>(name));
    }
    closeOne(socket);
}

void GroupServer::slot_connectionEstablished(GroupSocket *const socket)
{
    QVariantMap handshake;
    handshake["protocolVersion"] = NO_OPEN_SSL ? PROTOCOL_VERSION_102 : PROTOCOL_VERSION_103;
    sendMessage(socket, MessagesEnum::REQ_HANDSHAKE, handshake);
}

static bool isEqualsCaseInsensitive(const QString &a, const QString &b)
{
    return a.compare(b, Qt::CaseInsensitive) == 0;
}

void GroupServer::virt_retrieveData(GroupSocket *const socket,
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
            emit sig_sendLog(QString("'%1' has successfully logged in.").arg(nameStr));
            sendMessage(socket, MessagesEnum::STATE_LOGGED);
            if (!NO_OPEN_SSL && socket->getProtocolVersion() == PROTOCOL_VERSION_102) {
                QVariantMap root;
                root["text"] = QString("WARNING: %1 joined the group with an insecure connection "
                                       "and needs to upgrade MMapper!")
                                   .arg(nameStr);
                root["from"] = "MMapper";
                slot_sendGroupTellMessage(root);
                emit sig_gTellArrived(root);
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
            if (!isEqualsCaseInsensitive(updateName, nameStr)) {
                emit sig_sendLog(QString("WARNING: '%1' spoofed as '%2'").arg(nameStr, updateName));
                return;
            }
            emit sig_scheduleAction(std::make_shared<UpdateCharacter>(data));
            slot_relayMessage(socket, MessagesEnum::UPDATE_CHAR, data);

        } else if (message == MessagesEnum::GTELL) {
            const auto &fromName = data["from"].toString().simplified();
            if (!isEqualsCaseInsensitive(fromName, nameStr)) {
                emit sig_sendLog(QString("WARNING: '%1' spoofed as '%2'").arg(nameStr, fromName));
                return;
            }
            emit sig_gTellArrived(data);
            slot_relayMessage(socket, MessagesEnum::GTELL, data);

        } else if (message == MessagesEnum::REQ_ACK) {
            sendMessage(socket, MessagesEnum::ACK);

        } else if (message == MessagesEnum::RENAME_CHAR) {
            const QString oldName = data["oldname"].toString().simplified();
            if (!isEqualsCaseInsensitive(oldName, nameStr)) {
                kickConnection(socket,
                               QString("Name spoof detected: %1 != %2").arg(oldName, nameStr));
                return;
            }
            const QString &newName = data["newname"].toString();
            if (!isEqualsCaseInsensitive(newName, newName.simplified())) {
                kickConnection(socket, "Your name must not include any whitespace");
                return;
            }
            for (const auto &target : clientsList) {
                if (socket == target)
                    continue;
                if (isEqualsCaseInsensitive(newName, QString::fromLatin1(target->getName()))) {
                    kickConnection(socket,
                                   QString("Someone was already using the name '%1'").arg(newName));
                    return;
                }
            }
            socket->setName(newName.toLatin1());
            emit sig_scheduleAction(std::make_shared<RenameCharacter>(data));
            slot_relayMessage(socket, MessagesEnum::RENAME_CHAR, data);

        } else {
            // ERROR: unexpected message marker!
            // try to ignore?
            qWarning("(Logged) Unexpected message marker. Trying to ignore.");
        }
    }
}

void GroupServer::virt_sendCharUpdate(const QVariantMap &map)
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
        assert(QSslSocket::supportsSsl());
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
    const QString tempName = QString("%1-%2").arg(nameStr).arg(getRandom(1000));
    socket->setName(tempName.toLatin1());
    emit sig_sendLog(QString("'%1' is trying to join the group as '%2'.").arg(tempName, nameStr));

    // Check credentials
    const auto secret = socket->getSecret();
    const bool isEncrypted = !secret.isEmpty()
                             && socket->getProtocolVersion() >= PROTOCOL_VERSION_103;
    const bool requireAuth = getConfig().groupManager.requireAuth;
    const bool validSecret = getAuthority()->validSecret(secret);
    const bool validCert = getAuthority()->validCertificate(socket);
    bool reconnect = false;
    if (isEncrypted) {
        emit sig_sendLog(QString("'%1's secret: %2").arg(tempName, QString::fromLatin1(secret)));

        // Verify only one secret can be connected at once
        for (const auto &target : clientsList) {
            if (socket == target)
                continue;
            if (socket->getPeerCertificate() == target->getPeerCertificate()) {
                kickConnection(target, "Someone reconnected to the server using your secret!");
                reconnect = true;
                break;
            }
        }

    } else {
        emit sig_sendLog(
            QString("<b>WARNING:</b> '%1' has no secret and their connection is not encrypted.")
                .arg(tempName));
    }
    emit sig_sendLog(QString("'%1's IP address: %2").arg(tempName, socket->getPeerName()));
    emit sig_sendLog(
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
    const QString simplifiedName = nameStr.simplified();
    if (!isEqualsCaseInsensitive(simplifiedName, nameStr)) {
        kickConnection(socket, "Your name must not include any whitespace");
        return;
    }
    if (simplifiedName.isEmpty()) {
        kickConnection(socket, "Your name cannot be empty");
        return;
    }
    if (getGroup()->isNamePresent(nameStr.toLatin1())) {
        kickConnection(socket, "The name you picked is already present!");
        return;
    } else {
        // Allow this name to now take effect
        emit sig_sendLog(QString("'%1' will now be known as '%2'").arg(tempName, nameStr));
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
    slot_relayMessage(socket, MessagesEnum::ADD_CHAR, charNode);
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

void GroupServer::virt_sendGroupTellMessage(const QVariantMap &root)
{
    const QByteArray &message = formMessageBlock(MessagesEnum::GTELL, root);
    sendToAll(message);
}

void GroupServer::slot_relayMessage(GroupSocket *const socket,
                                    const MessagesEnum message,
                                    const QVariantMap &data)
{
    const QByteArray &buffer = formMessageBlock(message, data);
    sendToAllExceptOne(socket, buffer);
}

void GroupServer::virt_sendCharRename(const QVariantMap &map)
{
    QByteArray message = formMessageBlock(MessagesEnum::RENAME_CHAR, map);
    sendToAll(message);
}

void GroupServer::virt_stop()
{
    closeAll();
    emit sig_scheduleAction(std::make_shared<ResetCharacters>());
    deleteLater();
}

bool GroupServer::virt_start()
{
    if (server.isListening()) {
        emit sig_sendLog("Closing connections and restarting server...");
        server.setMaxPendingConnections(0);
        closeAll();
        server.close();
    }
    const auto localPort = static_cast<quint16>(getConfig().groupManager.localPort);
    if (portMapper.tryAddPortMapping(localPort)) {
        const QString externalIp = QString::fromLocal8Bit(portMapper.tryGetExternalIp());
        emit sig_sendLog(
            QString("Added port mapping to UPnP IGD router with external IP: %1").arg(externalIp));
    }
    emit sig_sendLog(QString("Listening on port %1").arg(localPort));
    if (!server.listen(QHostAddress::Any, localPort)) {
        emit sig_sendLog("Failed to start a group Manager server");
        emit sig_messageBox(
            QString("Failed to start the groupManager server: %1.").arg(server.errorString()));
        return false;
    }
    return true;
}

void GroupServer::virt_kickCharacter(const QByteArray &name)
{
    for (auto &connection : clientsList) {
        if (connection->getName() == name) {
            kickConnection(connection, "You have been kicked by the host!");
            return;
        }
    }
}

void GroupServer::kickConnection(GroupSocket *const socket, const QString &message)
{
    if (socket->getProtocolVersion() == PROTOCOL_VERSION_102
        && socket->getProtocolState() != ProtocolStateEnum::AwaitingLogin) {
        // Protocol 102 does not support kicking outside of AwaitingLogin so we fake it with a group tell
        QVariantMap root;
        root["text"] = message;
        root["from"] = QString::fromLatin1(getGroup()->getSelf()->getName());
        sendMessage(socket, MessagesEnum::GTELL, root);

    } else {
        sendMessage(socket, MessagesEnum::STATE_KICKED, message.toLatin1());
    }
    const QString nameStr = QString::fromLatin1(socket->getName());
    const QString identifier = nameStr.isEmpty() ? socket->getPeerName() : nameStr;
    qDebug() << "Kicking" << identifier << "for" << message;
    emit sig_sendLog(QString("'%1' was kicked: %2").arg(identifier, message));

    const auto &name = socket->getName();
    if (getGroup()->isNamePresent(name)) {
        sendRemoveUserNotification(socket, name);
        emit sig_scheduleAction(std::make_shared<RemoveCharacter>(name));
    }
    closeOne(socket);
}

void GroupServer::slot_onRevokeWhitelist(const QByteArray &secret)
{
    if (getConfig().groupManager.requireAuth) {
        for (auto &connection : clientsList) {
            if (isEqualsCaseInsensitive(secret, connection->getSecret())) {
                kickConnection(connection, "You have been removed from the host's contacts!");
            }
        }
    }
}
