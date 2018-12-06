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

#include "GroupClient.h"

#include <QAbstractSocket>
#include <QByteArray>
#include <QMessageLogContext>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include "../configuration/configuration.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "GroupSocket.h"
#include "groupaction.h"
#include "groupauthority.h"
#include "mmapper2group.h"

using Messages = CGroupCommunicator::Messages;

GroupClient::GroupClient(Mmapper2Group *parent)
    : CGroupCommunicator(GroupManagerState::Client, parent)
    , socket(parent->getAuthority(), this)
{
    emit sendLog("Client mode has been selected");
    connect(&socket, &GroupSocket::incomingData, this, &GroupClient::incomingData);
    connect(&socket, &GroupSocket::sendLog, this, &GroupClient::relayLog);
    connect(&socket, &GroupSocket::errorInConnection, this, &GroupClient::errorInConnection);
    connect(&socket, &GroupSocket::connectionEstablished, this, &GroupClient::connectionEstablished);
    connect(&socket, &GroupSocket::connectionClosed, this, &GroupClient::connectionClosed);
    connect(&socket, &GroupSocket::connectionEncrypted, this, &GroupClient::connectionEncrypted);
}

GroupClient::~GroupClient()
{
    disconnect(&socket, &GroupSocket::incomingData, this, &GroupClient::incomingData);
    disconnect(&socket, &GroupSocket::sendLog, this, &GroupClient::relayLog);
    disconnect(&socket, &GroupSocket::errorInConnection, this, &GroupClient::errorInConnection);
    disconnect(&socket,
               &GroupSocket::connectionEstablished,
               this,
               &GroupClient::connectionEstablished);
    disconnect(&socket, &GroupSocket::connectionClosed, this, &GroupClient::connectionClosed);
    disconnect(&socket, &GroupSocket::connectionEncrypted, this, &GroupClient::connectionEncrypted);
}

void GroupClient::connectionEstablished()
{
    clientConnected = true;
}

void GroupClient::connectionClosed(GroupSocket * /*socket*/)
{
    if (!clientConnected)
        return;

    emit sendLog("Server closed the connection");
    tryReconnecting();
}

void GroupClient::errorInConnection(GroupSocket *const socket, const QString &errorString)
{
    QString str;

    const auto log_message = [this](const QString &message) {
        if (reconnectAttempts <= 0)
            emit messageBox(message);
        else
            emit sendLog(message);
    };

    switch (socket->getSocketError()) {
    case QAbstractSocket::ConnectionRefusedError:
        str = QString("Tried to connect to %1 on port %2")
                  .arg(getConfig().groupManager.host.constData())
                  .arg(getConfig().groupManager.remotePort);
        log_message(QString("Connection refused: %1.").arg(str));
        break;

    case QAbstractSocket::RemoteHostClosedError:
        emit messageBox(QString("Connection closed: %1.").arg(errorString));
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

void GroupClient::retrieveData(GroupSocket *const socket,
                               const Messages message,
                               const QVariantMap &data)
{
    if (message == Messages::STATE_KICKED) {
        emit messageBox(QString("You got kicked! Reason: %1").arg(data["text"].toString()));
        stop();
        return;
    }
    if (socket->getProtocolState() == ProtocolState::AwaitingLogin) {
        // Login state. either REQ_HANDSHAKE, REQ_LOGIN, or ACK should come
        if (message == Messages::REQ_HANDSHAKE) {
            sendHandshake(data);
        } else if (message == Messages::REQ_LOGIN) {
            assert(!NO_OPEN_SSL);
            socket->setProtocolVersion(proposedProtocolVersion);
            socket->startClientEncrypted();
        } else if (message == Messages::ACK) {
            // aha! logged on!
            sendMessage(socket, Messages::REQ_INFO);
            socket->setProtocolState(ProtocolState::AwaitingInfo);
        } else {
            // ERROR: unexpected message marker!
            // try to ignore?
            qWarning("(AwaitingLogin) Unexpected message marker. Trying to ignore.");
        }

    } else if (socket->getProtocolState() == ProtocolState::AwaitingInfo) {
        // almost connected. awaiting full information about the connection
        if (message == Messages::UPDATE_CHAR) {
            emit scheduleAction(new AddCharacter(data));
        } else if (message == Messages::STATE_LOGGED) {
            socket->setProtocolState(ProtocolState::Logged);
        } else if (message == Messages::REQ_ACK) {
            sendMessage(socket, Messages::ACK);
        } else {
            // ERROR: unexpected message marker!
            // try to ignore?
            qWarning("(AwaitingInfo) Unexpected message marker. Trying to ignore.");
        }

    } else if (socket->getProtocolState() == ProtocolState::Logged) {
        if (message == Messages::ADD_CHAR) {
            emit scheduleAction(new AddCharacter(data));
        } else if (message == Messages::REMOVE_CHAR) {
            emit scheduleAction(new RemoveCharacter(data));
        } else if (message == Messages::UPDATE_CHAR) {
            emit scheduleAction(new UpdateCharacter(data));
        } else if (message == Messages::RENAME_CHAR) {
            emit scheduleAction(new RenameCharacter(data));
        } else if (message == Messages::GTELL) {
            emit gTellArrived(data);
        } else if (message == Messages::REQ_ACK) {
            sendMessage(socket, Messages::ACK);
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
    emit sendLog(QString("Host's protocol version: %1").arg(serverProtocolVersion));
    const auto get_proposed_protocol_version = [](const auto serverProtocolVersion) {
        // Ensure we only pick a protocol within the bounds we understand
        if (NO_OPEN_SSL) {
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
        if (!NO_OPEN_SSL && getConfig().groupManager.requireAuth) {
            emit messageBox(
                "Host does not support encryption.\n"
                "Consider disabling \"Require authorization\" under the Group Manager settings "
                "or ask the host to upgrade MMapper.");
            stop();
            return;
        } else {
            // MMapper 2.0.3 through MMapper 2.6 Protocol 102 does not do a version handshake
            // and goes directly to login
            if (!NO_OPEN_SSL)
                emit sendLog("<b>WARNING:</b> "
                             "Host does not support encryption and your connection is insecure.");

            sendLoginInformation();
        }

    } else {
        QVariantMap handshake;
        handshake["protocolVersion"] = proposedProtocolVersion;
        sendMessage(&socket, Messages::REQ_HANDSHAKE, handshake);
    }
}

void GroupClient::sendLoginInformation()
{
    const CGroupChar *character = getGroup()->getSelf();
    QVariantMap loginData = character->toVariantMap();
    if (proposedProtocolVersion == PROTOCOL_VERSION_102) {
        // Protocol 102 does handshake and login in one step
        loginData["protocolVersion"] = socket.getProtocolVersion();
        socket.setProtocolVersion(PROTOCOL_VERSION_102);
    }
    QVariantMap root;
    root["loginData"] = loginData;
    sendMessage(&socket, Messages::UPDATE_CHAR, root);
}

void GroupClient::connectionEncrypted()
{
    const auto secret = socket.getSecret();
    emit sendLog(QString("Host's secret: %1").arg(secret.constData()));

    const bool requireAuth = getConfig().groupManager.requireAuth;
    const bool validSecret = getAuthority()->validSecret(secret);
    const bool validCert = getAuthority()->validCertificate(&socket);
    if (requireAuth && !validSecret) {
        emit messageBox(
            QString("Host's secret is not in your contacts:\n%1").arg(secret.constData()));
        stop();
        return;
    }
    if (requireAuth && !validCert) {
        emit messageBox(
            "WARNING: Host's secret has been compromised making the connection insecure.");
        stop();
        return;
    }

    sendLoginInformation();

    if (validSecret) {
        // Update metadata
        getAuthority()->setMetadata(secret,
                                    GroupMetadata::IP_ADDRESS,
                                    socket.getPeerAddress().toString());
        getAuthority()->setMetadata(secret,
                                    GroupMetadata::LAST_LOGIN,
                                    QDateTime::currentDateTime().toString());
        getAuthority()->setMetadata(secret,
                                    GroupMetadata::CERTIFICATE,
                                    socket.getPeerCertificate().toPem());
    }
}

void GroupClient::tryReconnecting()
{
    clientConnected = false;

    if (reconnectAttempts <= 0) {
        emit sendLog("Exhausted reconnect attempts.");
        stop();
        return;
    }
    emit sendLog(QString("Attempting to reconnect... (%1 left)").arg(reconnectAttempts));

    // Retry
    reconnectAttempts--;
    emit scheduleAction(new ResetCharacters());
    socket.connectToHost();
}

void GroupClient::sendGroupTellMessage(const QVariantMap &root)
{
    sendMessage(&socket, Messages::GTELL, root);
}

void GroupClient::sendCharUpdate(const QVariantMap &map)
{
    CGroupCommunicator::sendCharUpdate(&socket, map);
}

void GroupClient::sendCharRename(const QVariantMap &map)
{
    sendMessage(&socket, Messages::RENAME_CHAR, map);
}

void GroupClient::stop()
{
    clientConnected = false;
    socket.disconnectFromHost();
    emit scheduleAction(new ResetCharacters());
    deleteLater();
}

bool GroupClient::start()
{
    socket.connectToHost();
    return true;
}

bool GroupClient::kickCharacter(const QByteArray &)
{
    emit messageBox("Clients cannot kick players.");
    return false;
}
