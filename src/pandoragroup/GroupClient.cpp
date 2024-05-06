// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "GroupClient.h"

#include "../configuration/configuration.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "GroupSocket.h"
#include "groupaction.h"
#include "groupauthority.h"
#include "mmapper2group.h"

#include <memory>

#include <QAbstractSocket>
#include <QByteArray>
#include <QMessageLogContext>
#include <QObject>
#include <QSslSocket>
#include <QString>
#include <QVariantMap>

using MessagesEnum = CGroupCommunicator::MessagesEnum;

GroupClient::GroupClient(Mmapper2Group *parent)
    : CGroupCommunicator(GroupManagerStateEnum::Client, parent)
    , socket(parent->getAuthority(), this)
{
    connect(&socket, &GroupSocket::sig_incomingData, this, &GroupClient::slot_incomingData);
    connect(&socket, &GroupSocket::sig_sendLog, this, &GroupClient::slot_relayLog);
    connect(&socket,
            &GroupSocket::sig_errorInConnection,
            this,
            &GroupClient::slot_errorInConnection);
    connect(&socket,
            &GroupSocket::sig_connectionEstablished,
            this,
            &GroupClient::slot_connectionEstablished);
    connect(&socket, &GroupSocket::sig_connectionClosed, this, &GroupClient::slot_connectionClosed);
    connect(&socket,
            &GroupSocket::sig_connectionEncrypted,
            this,
            &GroupClient::slot_connectionEncrypted);

    emit sig_sendLog("Client mode has been selected");
}

GroupClient::~GroupClient()
{
    // TODO: remove these manual disconnects.
    // Destroying an object is supposed to remove its connections.
    // (If removing the disconnects causes segfaults, then you may
    // have a race condition that needs to be fixed elsewhere.)
    disconnect(&socket, &GroupSocket::sig_incomingData, this, &GroupClient::slot_incomingData);
    disconnect(&socket, &GroupSocket::sig_sendLog, this, &GroupClient::slot_relayLog);
    disconnect(&socket,
               &GroupSocket::sig_errorInConnection,
               this,
               &GroupClient::slot_errorInConnection);
    disconnect(&socket,
               &GroupSocket::sig_connectionEstablished,
               this,
               &GroupClient::slot_connectionEstablished);
    disconnect(&socket,
               &GroupSocket::sig_connectionClosed,
               this,
               &GroupClient::slot_connectionClosed);
    disconnect(&socket,
               &GroupSocket::sig_connectionEncrypted,
               this,
               &GroupClient::slot_connectionEncrypted);
}

void GroupClient::slot_connectionEstablished()
{
    clientConnected = true;
}

void GroupClient::virt_connectionClosed(GroupSocket * /*socket*/)
{
    if (!clientConnected)
        return;

    emit sig_sendLog("Server closed the connection");
    tryReconnecting();
}

void GroupClient::slot_errorInConnection(GroupSocket *const /* socket */, const QString &errorString)
{
    QString str;

    const auto log_message = [this](const QString &message) {
        if (reconnectAttempts <= 0)
            emit sig_messageBox(message);
        else
            emit sig_sendLog(message);
    };

    switch (socket.getSocketError()) {
    case QAbstractSocket::ConnectionRefusedError:
        str = QString("Tried to connect to %1 on port %2")
                  .arg(getConfig().groupManager.host.constData())
                  .arg(getConfig().groupManager.remotePort);
        log_message(QString("Connection refused: %1.").arg(str));
        break;

    case QAbstractSocket::RemoteHostClosedError:
        log_message(QString("Connection closed: %1.").arg(errorString));
        break;

    case QAbstractSocket::HostNotFoundError:
        str = QString("Host %1 not found ").arg(getConfig().groupManager.host.constData());
        log_message(QString("Connection refused: %1.").arg(str));
        break;

    case QAbstractSocket::SocketAccessError:
    case QAbstractSocket::UnsupportedSocketOperationError:
    case QAbstractSocket::ProxyAuthenticationRequiredError:
    case QAbstractSocket::UnknownSocketError:
    case QAbstractSocket::UnfinishedSocketOperationError:
    case QAbstractSocket::SocketResourceError:
    case QAbstractSocket::SocketTimeoutError:
    case QAbstractSocket::DatagramTooLargeError:
    case QAbstractSocket::NetworkError:
    case QAbstractSocket::AddressInUseError:
    case QAbstractSocket::SocketAddressNotAvailableError:
    case QAbstractSocket::SslHandshakeFailedError:
    case QAbstractSocket::ProxyConnectionRefusedError:
    case QAbstractSocket::ProxyConnectionClosedError:
    case QAbstractSocket::ProxyConnectionTimeoutError:
    case QAbstractSocket::ProxyNotFoundError:
    case QAbstractSocket::ProxyProtocolError:
    case QAbstractSocket::OperationError:
    case QAbstractSocket::SslInternalError:
    case QAbstractSocket::SslInvalidUserDataError:
    case QAbstractSocket::TemporaryError:
        log_message(QString("Connection error: %1.").arg(errorString));
        break;
    }
    tryReconnecting();
}

void GroupClient::virt_retrieveData(GroupSocket *const /* socket */,
                                    const MessagesEnum message,
                                    const QVariantMap &data)
{
    if (message == MessagesEnum::STATE_KICKED) {
        emit sig_messageBox(QString("You got kicked! Reason: %1").arg(data["text"].toString()));
        stop();
        return;
    }
    if (socket.getProtocolState() == ProtocolStateEnum::AwaitingLogin) {
        // Login state. either REQ_HANDSHAKE, REQ_LOGIN, or ACK should come
        if (message == MessagesEnum::REQ_HANDSHAKE) {
            sendHandshake(data);
        } else if (message == MessagesEnum::REQ_LOGIN) {
            assert(QSslSocket::supportsSsl());
            socket.setProtocolVersion(proposedProtocolVersion);
            socket.startClientEncrypted();
        } else if (message == MessagesEnum::ACK) {
            // aha! logged on!
            sendMessage(&socket, MessagesEnum::REQ_INFO);
            socket.setProtocolState(ProtocolStateEnum::AwaitingInfo);
        } else {
            // ERROR: unexpected message marker!
            // try to ignore?
            qWarning("(AwaitingLogin) Unexpected message marker. Trying to ignore.");
        }

    } else if (socket.getProtocolState() == ProtocolStateEnum::AwaitingInfo) {
        // almost connected. awaiting full information about the connection
        if (message == MessagesEnum::UPDATE_CHAR) {
            receiveGroupInformation(data);
        } else if (message == MessagesEnum::STATE_LOGGED) {
            socket.setProtocolState(ProtocolStateEnum::Logged);
        } else if (message == MessagesEnum::REQ_ACK) {
            sendMessage(&socket, MessagesEnum::ACK);
        } else {
            // ERROR: unexpected message marker!
            // try to ignore?
            qWarning("(AwaitingInfo) Unexpected message marker. Trying to ignore.");
        }

    } else if (socket.getProtocolState() == ProtocolStateEnum::Logged) {
        if (message == MessagesEnum::ADD_CHAR) {
            emit sig_scheduleAction(std::make_shared<AddCharacter>(data));
        } else if (message == MessagesEnum::REMOVE_CHAR) {
            emit sig_scheduleAction(std::make_shared<RemoveCharacter>(data));
        } else if (message == MessagesEnum::UPDATE_CHAR) {
            emit sig_scheduleAction(std::make_shared<UpdateCharacter>(data));
        } else if (message == MessagesEnum::RENAME_CHAR) {
            emit sig_scheduleAction(std::make_shared<RenameCharacter>(data));
        } else if (message == MessagesEnum::GTELL) {
            emit sig_gTellArrived(data);
        } else if (message == MessagesEnum::REQ_ACK) {
            sendMessage(&socket, MessagesEnum::ACK);
        } else {
            // ERROR: unexpected message marker!
            // try to ignore?
            qWarning("(Logged) Unexpected message marker. Trying to ignore.");
        }
    }
}

//
// Parsers and Senders of information and signals to upper and lower objects
//
void GroupClient::sendHandshake(const QVariantMap &data)
{
    const auto get_server_protocol_version = [](const auto &data) {
        if (!data.contains("protocolVersion")
            || !data["protocolVersion"].canConvert(QMetaType::UInt)) {
            return PROTOCOL_VERSION_102;
        }
        return data["protocolVersion"].toUInt();
    };
    const auto serverProtocolVersion = get_server_protocol_version(data);
    emit sig_sendLog(QString("Host's protocol version: %1").arg(serverProtocolVersion));
    const auto get_proposed_protocol_version = [](const auto serverProtocolVersion) {
        // Ensure we only pick a protocol within the bounds we understand
        if (!QSslSocket::supportsSsl()) {
            return PROTOCOL_VERSION_102;
        } else if (serverProtocolVersion >= PROTOCOL_VERSION_103) {
            return PROTOCOL_VERSION_103;
        } else if (serverProtocolVersion <= PROTOCOL_VERSION_102) {
            return PROTOCOL_VERSION_102;
        }
        return serverProtocolVersion;
    };
    proposedProtocolVersion = get_proposed_protocol_version(serverProtocolVersion);

    if (serverProtocolVersion == PROTOCOL_VERSION_102
        || proposedProtocolVersion == PROTOCOL_VERSION_102) {
        if (QSslSocket::supportsSsl() && getConfig().groupManager.requireAuth) {
            emit sig_messageBox(
                "Host does not support encryption.\n"
                "Consider disabling \"Require authorization\" under the Group Manager settings "
                "or ask the host to upgrade MMapper.");
            stop();
            return;
        } else {
            // MMapper 2.0.3 through MMapper 2.6 Protocol 102 does not do a version handshake
            // and goes directly to login
            if (QSslSocket::supportsSsl())
                emit sig_sendLog(
                    "<b>WARNING:</b> "
                    "Host does not support encryption and your connection is insecure.");

            sendLoginInformation();
        }

    } else {
        QVariantMap handshake;
        handshake["protocolVersion"] = proposedProtocolVersion;
        sendMessage(&socket, MessagesEnum::REQ_HANDSHAKE, handshake);
    }
}

void GroupClient::receiveGroupInformation(const QVariantMap &data)
{
    const auto &selection = getGroup()->selectAll();
    const bool isSolo = selection->size() == 1 && getGroup()->getSelf() == *selection->begin();
    if (isSolo) {
        // Update metadata and assume first received character is host
        const auto &secret = socket.getSecret();
        const auto &nameStr = QString::fromLatin1(CGroupChar::getNameFromUpdateChar(data));
        getAuthority()->setMetadata(secret, GroupMetadataEnum::NAME, nameStr);
        emit sig_sendLog("Host's name is most likely '" + nameStr + "'");
    }
    emit sig_scheduleAction(std::make_shared<AddCharacter>(data));
}

void GroupClient::sendLoginInformation()
{
    const SharedGroupChar &character = getGroup()->getSelf();
    QVariantMap loginData = character->toVariantMap();
    if (proposedProtocolVersion == PROTOCOL_VERSION_102) {
        // Protocol 102 does handshake and login in one step
        loginData["protocolVersion"] = socket.getProtocolVersion();
        socket.setProtocolVersion(PROTOCOL_VERSION_102);
    }
    QVariantMap root;
    root["loginData"] = loginData;
    sendMessage(&socket, MessagesEnum::UPDATE_CHAR, root);
}

void GroupClient::slot_connectionEncrypted()
{
    const auto secret = socket.getSecret();
    emit sig_sendLog(QString("Host's secret: %1").arg(secret.constData()));

    const bool requireAuth = getConfig().groupManager.requireAuth;
    const bool validSecret = getAuthority()->validSecret(secret);
    const bool validCert = getAuthority()->validCertificate(&socket);
    if (requireAuth && !validSecret) {
        emit sig_messageBox(
            QString("Host's secret is not in your contacts:\n%1").arg(secret.constData()));
        stop();
        return;
    }
    if (requireAuth && !validCert) {
        emit sig_messageBox(
            "WARNING: Host's secret has been compromised making the connection insecure.");
        stop();
        return;
    }

    sendLoginInformation();

    if (validCert) {
        // Assume that anyone connecting to a host will trust them (if auth is not required)
        if (!validSecret)
            getAuthority()->add(secret);
        // Update metadata
        getAuthority()->setMetadata(secret, GroupMetadataEnum::IP_ADDRESS, socket.getPeerName());
        getAuthority()->setMetadata(secret,
                                    GroupMetadataEnum::LAST_LOGIN,
                                    QDateTime::currentDateTime().toString());
        getAuthority()->setMetadata(secret,
                                    GroupMetadataEnum::CERTIFICATE,
                                    socket.getPeerCertificate().toPem());
        getAuthority()->setMetadata(secret,
                                    GroupMetadataEnum::PORT,
                                    QString("%1").arg(socket.getPeerPort()));
    }
}

void GroupClient::tryReconnecting()
{
    clientConnected = false;

    if (reconnectAttempts <= 0) {
        emit sig_sendLog("Exhausted reconnect attempts.");
        stop();
        return;
    }
    emit sig_sendLog(QString("Attempting to reconnect... (%1 left)").arg(reconnectAttempts));

    // Retry
    reconnectAttempts--;
    emit sig_scheduleAction(std::make_shared<ResetCharacters>());
    socket.connectToHost();
}

void GroupClient::virt_sendGroupTellMessage(const QVariantMap &root)
{
    sendMessage(&socket, MessagesEnum::GTELL, root);
}

void GroupClient::virt_sendCharUpdate(const QVariantMap &map)
{
    CGroupCommunicator::sendCharUpdate(&socket, map);
}

void GroupClient::virt_sendCharRename(const QVariantMap &map)
{
    sendMessage(&socket, MessagesEnum::RENAME_CHAR, map);
}

void GroupClient::virt_stop()
{
    clientConnected = false;
    socket.disconnectFromHost();
    emit sig_scheduleAction(std::make_shared<ResetCharacters>());
    deleteLater();
}

bool GroupClient::virt_start()
{
    socket.connectToHost();
    return true;
}

void GroupClient::virt_kickCharacter(const QByteArray &)
{
    throw std::runtime_error("impossible");
}
