// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "GroupServer.h"

#include "../configuration/configuration.h"
#include "../global/MakeQPointer.h"
#include "../global/random.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "CGroupCommunicator.h"
#include "GroupSocket.h"
#include "groupaction.h"
#include "groupauthority.h"
#include "groupselection.h"
#include "mmapper2group.h"

#include <algorithm>
#include <memory>

#include <QAbstractSocket>
#include <QByteArray>
#include <QHostAddress>
#include <QList>
#include <QMessageLogContext>
#include <QObject>
#include <QSslSocket>
#include <QString>
#include <QVariantMap>

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
    GroupSocket &socket = deref(
        filterClientList().emplace_back(mmqt::makeQPointer<GroupSocket>(getAuthority(), this)));
    connectAll(socket);
    socket.setSocket(socketDescriptor);
    qDebug() << "Adding incoming client" << socket.getPeerName();
}

void GroupServer::slot_errorInConnection(GroupSocket *const pSocket, const QString &errorMessage)
{
    GroupSocket &socket = deref(pSocket);
    const QString name = socket.getName();
    if (getGroup()->isNamePresent(name)) {
        sendRemoveUserNotification(socket, name);
        emit sig_sendLog(QString("'%1' encountered an error: %2").arg(name, errorMessage));
        emit sig_scheduleAction(std::make_shared<RemoveCharacter>(name));
    }
    closeOne(socket);
}

void GroupServer::sendToAll(const QString &message)
{
    sendToAllExceptOne(nullptr, message);
}

void GroupServer::sendToAllExceptOne(GroupSocket *const exception, const QString &message)
{
    for (auto &connection : filterClientList()) {
        if (connection == exception) {
            continue;
        }
        if (connection->getProtocolState() == ProtocolStateEnum::Logged) {
            connection->sendData(message);
        }
    }
}

void GroupServer::closeAll()
{
    auto clients = std::exchange(filterClientList(), {});
    for (auto &pConnection : clients) {
        auto &connection = deref(pConnection);
        connection.disconnectFromHost();
        disconnectAll(connection);
    }
}

void GroupServer::closeOne(GroupSocket &target)
{
    target.disconnectFromHost();
    disconnectAll(target);

    auto &clients = filterClientList();
    const bool num_erased = utils::erase_if(clients,
                                            [&target](const QPointer<GroupSocket> &socket) -> bool {
                                                return socket == &target;
                                            });

    if (num_erased == 0) {
        qWarning() << "Could not find" << target.getName() << "among clients";
        assert(false);
    }
}

void GroupServer::connectAll(GroupSocket &clientRef)
{
    GroupSocket *const client = &clientRef;
    connect(client, &GroupSocket::sig_incomingData, this, &GroupServer::slot_incomingData);
    connect(client,
            &GroupSocket::sig_connectionEstablished,
            this,
            &GroupServer::slot_connectionEstablished);
    connect(client, &GroupSocket::sig_connectionClosed, this, &GroupServer::slot_connectionClosed);
    connect(client, &GroupSocket::sig_errorInConnection, this, &GroupServer::slot_errorInConnection);
}

void GroupServer::disconnectAll(GroupSocket &clientRef)
{
    GroupSocket *const client = &clientRef;
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
    , m_server{this}
{
    connect(&m_server, &GroupTcpServer::acceptError, this, [this]() {
        emit sig_sendLog(QString("Server encountered an error: %1").arg(m_server.errorString()));
    });
    connect(&m_server,
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
    const auto localPort = static_cast<uint16_t>(getConfig().groupManager.localPort);
    if (m_portMapper.tryDeletePortMapping(localPort)) {
        emit sig_sendLog("Deleted port mapping from UPnP IGD router");
    }
}

void GroupServer::virt_connectionClosed(GroupSocket &socket)
{
    const auto &name = socket.getName();
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
    handshake["protocolVersion"] = NO_OPEN_SSL ? PROTOCOL_VERSION_104_insecure
                                               : PROTOCOL_VERSION_105_secure;
    sendMessage(deref(socket), MessagesEnum::REQ_HANDSHAKE, handshake);
}

static bool isEqualsCaseInsensitive(const QString &a, const QString &b)
{
    return a.compare(b, Qt::CaseInsensitive) == 0;
}

void GroupServer::virt_retrieveData(GroupSocket &socket,
                                    const MessagesEnum message,
                                    const QVariantMap &data)
{
    const QString nameStr = socket.getName();
    // ---------------------- AwaitingLogin  --------------------
    if (socket.getProtocolState() == ProtocolStateEnum::AwaitingLogin) {
        // Login state. either REQ_HANDSHAKE, UPDATE_CHAR, or ACK should come
        if (message == MessagesEnum::REQ_HANDSHAKE) {
            // Shaking hands with client
            parseHandshake(socket, data);
        } else if (message == MessagesEnum::UPDATE_CHAR) {
            // aha! parse the data
            if (socket.getProtocolVersion() >= PROTOCOL_VERSION_105_secure) {
                parseLoginInformation(socket, data);
            } else {
                parseHandshake(socket, data); // Protocol 102 skips the handshake
            }
        } else {
            // ERROR: unexpected message marker!
            // try to ignore?
            qWarning("(AwaitingLogin) Unexpected message marker. Trying to ignore.");
        }
        // ---------------------- AwaitingInfo  --------------------
    } else if (socket.getProtocolState() == ProtocolStateEnum::AwaitingInfo) {
        // almost connected. awaiting full information about the connection
        if (message == MessagesEnum::REQ_INFO) {
            sendGroupInformation(socket);
            sendMessage(socket, MessagesEnum::REQ_ACK);
        } else if (message == MessagesEnum::ACK) {
            socket.setProtocolState(ProtocolStateEnum::Logged);
            emit sig_sendLog(QString("'%1' has successfully logged in.").arg(nameStr));
            sendMessage(socket, MessagesEnum::STATE_LOGGED);
            if (!NO_OPEN_SSL && socket.getProtocolVersion() == PROTOCOL_VERSION_104_insecure) {
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
    } else if (socket.getProtocolState() == ProtocolStateEnum::Logged) {
        // usual update situation. receive update, unpack, apply.
        if (message == MessagesEnum::UPDATE_CHAR) {
            const QString &updateName = CGroupChar::getNameFromUpdateChar(data);
            if (!isEqualsCaseInsensitive(updateName, nameStr)) {
                emit sig_sendLog(QString("WARNING: '%1' spoofed as '%2'").arg(nameStr, updateName));
                return;
            }
            emit sig_scheduleAction(std::make_shared<UpdateCharacter>(data));
            relayMessage(socket, MessagesEnum::UPDATE_CHAR, data);

        } else if (message == MessagesEnum::GTELL) {
            const auto &fromName = data["from"].toString().simplified();
            if (!isEqualsCaseInsensitive(fromName, nameStr)) {
                emit sig_sendLog(QString("WARNING: '%1' spoofed as '%2'").arg(nameStr, fromName));
                return;
            }
            emit sig_gTellArrived(data);
            relayMessage(socket, MessagesEnum::GTELL, data);

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
            for (const auto &pTarget : filterClientList()) {
                if (&socket == pTarget) {
                    continue;
                }
                auto &target = deref(pTarget);
                if (isEqualsCaseInsensitive(newName, target.getName())) {
                    kickConnection(socket,
                                   QString("Someone was already using the name '%1'").arg(newName));
                    return;
                }
            }
            socket.setName(newName);
            emit sig_scheduleAction(std::make_shared<RenameCharacter>(data));
            relayMessage(socket, MessagesEnum::RENAME_CHAR, data);

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

void GroupServer::parseHandshake(GroupSocket &socket, const QVariantMap &data)
{
    if (!data.contains("protocolVersion") || !data["protocolVersion"].canConvert(QMetaType::UInt)) {
        kickConnection(socket, "Payload did not include the 'protocolVersion' attribute");
        return;
    }
    const ProtocolVersion clientProtocolVersion = data["protocolVersion"].toUInt();
    if (clientProtocolVersion < PROTOCOL_VERSION_104_insecure) {
        kickConnection(socket,
                       "Host requires a newer version of the group protocol. "
                       "Please upgrade to the latest MMapper.");
        return;
    }
    if (getConfig().groupManager.requireAuth
        && clientProtocolVersion == PROTOCOL_VERSION_104_insecure) {
        kickConnection(socket,
                       "Host requires authorization. "
                       "Please upgrade to the latest MMapper.");
        return;
    }
    auto supportedProtocolVersion = NO_OPEN_SSL ? PROTOCOL_VERSION_104_insecure
                                                : PROTOCOL_VERSION_105_secure;
    if (clientProtocolVersion > supportedProtocolVersion) {
        kickConnection(socket, "Host uses an older version of MMapper and needs to upgrade.");
        return;
    }
    if (clientProtocolVersion == PROTOCOL_VERSION_104_insecure) {
        socket.setProtocolVersion(clientProtocolVersion);
        parseLoginInformation(socket, data);
    } else {
        assert(QSslSocket::supportsSsl());
        sendMessage(socket, MessagesEnum::REQ_LOGIN);
        socket.setProtocolVersion(clientProtocolVersion);
        socket.startServerEncrypted();
    }
}

void GroupServer::parseLoginInformation(GroupSocket &socket, const QVariantMap &data)
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
    socket.setName(tempName);
    emit sig_sendLog(QString("'%1' is trying to join the group as '%2'.").arg(tempName, nameStr));

    // Check credentials
    const auto secret = socket.getSecret();
    const bool isEncrypted = !secret.isEmpty()
                             && socket.getProtocolVersion() >= PROTOCOL_VERSION_105_secure;
    const bool requireAuth = getConfig().groupManager.requireAuth;
    const bool validSecret = getAuthority()->validSecret(secret);
    const bool validCert = GroupAuthority::validCertificate(socket);
    bool reconnect = false;
    if (isEncrypted) {
        emit sig_sendLog(
            QString("'%1's secret: %2").arg(tempName, QString::fromUtf8(secret.getQByteArray())));

        // Verify only one secret can be connected at once
        for (const auto &pTarget : filterClientList()) {
            if (&socket == pTarget) {
                continue;
            }
            auto &target = deref(pTarget);
            if (socket.getPeerCertificate() == target.getPeerCertificate()) {
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
    emit sig_sendLog(QString("'%1's IP address: %2").arg(tempName, socket.getPeerName()));
    emit sig_sendLog(
        QString("'%1's protocol version: %2").arg(tempName).arg(socket.getProtocolVersion()));
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
    if (getGroup()->isNamePresent(nameStr)) {
        kickConnection(socket, "The name you picked is already present!");
        return;
    } else {
        // Allow this name to now take effect
        emit sig_sendLog(QString("'%1' will now be known as '%2'").arg(tempName, nameStr));
        socket.setName(nameStr);
    }

    // Client is allowed to log in
    if (isEncrypted && validSecret) {
        // Update metadata
        GroupAuthority::setMetadata(secret, GroupMetadataEnum::NAME, nameStr);
        GroupAuthority::setMetadata(secret, GroupMetadataEnum::IP_ADDRESS, socket.getPeerName());
        GroupAuthority::setMetadata(secret,
                                    GroupMetadataEnum::LAST_LOGIN,
                                    QDateTime::currentDateTime().toString());
        GroupAuthority::setMetadata(secret,
                                    GroupMetadataEnum::CERTIFICATE,
                                    socket.getPeerCertificate().toPem());
    }

    // Strip protocolVersion from original QVariantMap
    QVariantMap charNode;
    charNode["playerData"] = playerData;
    emit sig_scheduleAction(std::make_shared<AddCharacter>(charNode));
    relayMessage(socket, MessagesEnum::ADD_CHAR, charNode);
    sendMessage(socket, MessagesEnum::ACK);
    socket.setProtocolState(ProtocolStateEnum::AwaitingInfo);
}

void GroupServer::sendGroupInformation(GroupSocket &socket)
{
    auto selection = getGroup()->selectAll();
    for (const auto &character : *selection) {
        // Only send group information for other characters
        if (character->getName() == socket.getName()) {
            continue;
        }
        // Only share self if we enabled it
        if (getGroup()->getSelf() == character && !getConfig().groupManager.shareSelf) {
            continue;
        }
        CGroupCommunicator::sendCharUpdate(socket, character->toVariantMap());
    }
}

void GroupServer::sendRemoveUserNotification(GroupSocket &socket, const QString &name)
{
    auto selection = getGroup()->selectByName(name);
    for (const auto &character : *selection) {
        if (character->getName() == name) {
            const QVariantMap &map = character->toVariantMap();
            const QByteArray &message = formMessageBlock(MessagesEnum::REMOVE_CHAR, map);
            sendToAllExceptOne(&socket, message);
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
    if (m_server.isListening()) {
        emit sig_sendLog("Closing connections and restarting server...");
        m_server.setMaxPendingConnections(0);
        closeAll();
        m_server.close();
    }
    const auto localPort = static_cast<uint16_t>(getConfig().groupManager.localPort);
    if (m_portMapper.tryAddPortMapping(localPort)) {
        const QString externalIp = QString::fromLocal8Bit(m_portMapper.tryGetExternalIp());
        emit sig_sendLog(
            QString("Added port mapping to UPnP IGD router with external IP: %1").arg(externalIp));
    }
    emit sig_sendLog(QString("Listening on port %1").arg(localPort));
    if (!m_server.listen(QHostAddress::Any, localPort)) {
        emit sig_sendLog("Failed to start a group Manager server");
        emit sig_messageBox(
            QString("Failed to start the groupManager server: %1.").arg(m_server.errorString()));
        return false;
    }
    return true;
}

void GroupServer::virt_kickCharacter(const QString &name)
{
    for (auto &pConn : filterClientList()) {
        auto &connection = deref(pConn);
        if (connection.getName() == name) {
            kickConnection(connection, "You have been kicked by the host!");
            return;
        }
    }
}

void GroupServer::kickConnection(GroupSocket &socket, const QString &message)
{
    if (socket.getProtocolVersion() == PROTOCOL_VERSION_104_insecure
        && socket.getProtocolState() != ProtocolStateEnum::AwaitingLogin) {
        // Protocol 102 does not support kicking outside of AwaitingLogin so we fake it with a group tell
        QVariantMap root;
        root["text"] = message;
        root["from"] = getGroup()->getSelf()->getName();
        sendMessage(socket, MessagesEnum::GTELL, root);

    } else {
        sendMessage(socket, MessagesEnum::STATE_KICKED, message);
    }
    const QString nameStr = socket.getName();
    const QString identifier = nameStr.isEmpty() ? socket.getPeerName() : nameStr;
    qDebug() << "Kicking" << identifier << "for" << message;
    emit sig_sendLog(QString("'%1' was kicked: %2").arg(identifier, message));

    const auto &name = socket.getName();
    if (getGroup()->isNamePresent(name)) {
        sendRemoveUserNotification(socket, name);
        emit sig_scheduleAction(std::make_shared<RemoveCharacter>(name));
    }
    closeOne(socket);
}

void GroupServer::slot_onRevokeWhitelist(const GroupSecret &secret)
{
    if (getConfig().groupManager.requireAuth) {
        for (auto &pConnection : filterClientList()) {
            auto &connection = deref(pConnection);
            if (secret.getQByteArray() == connection.getSecret().getQByteArray()) {
                kickConnection(connection, "You have been removed from the host's contacts!");
            }
        }
    }
}

// This function exists because QPointer<GroupSocket> can become nullptr behind our back.
GroupServer::ClientList &GroupServer::filterClientList()
{
    auto &clients = m_clientsList;
    utils::erase_if(clients,
                    [](const QPointer<GroupSocket> &sock) -> bool { return sock == nullptr; });
    return clients;
}
