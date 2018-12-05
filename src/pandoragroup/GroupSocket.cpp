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

#include "GroupSocket.h"

#include <cassert>
#include <QByteArray>
#include <QMessageLogContext>
#include <QSslConfiguration>
#include <QString>
#include <QTcpSocket>
#include <QTimer>

#include "../configuration/configuration.h"
#include "../global/io.h"
#include "groupauthority.h"

static constexpr const bool DEBUG = false;
static constexpr const auto FIVE_SECOND_TIMEOUT = 5000;

GroupSocket::GroupSocket(GroupAuthority *authority, QObject *parent)
    : QObject(parent)
    , socket{this}
    , timer{this}
    , authority(authority)
{
    timer.setInterval(FIVE_SECOND_TIMEOUT);
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, &GroupSocket::onTimeout);

    auto config = socket.sslConfiguration();
    config.setCaCertificates({});
    config.setLocalCertificate(authority->getLocalCertificate());
    config.setPrivateKey(authority->getPrivateKey());
    config.setPeerVerifyMode(QSslSocket::QueryPeer);
    socket.setSslConfiguration(config);
    socket.setPeerVerifyName(GROUP_COMMON_NAME);
    connect(&socket, &QAbstractSocket::hostFound, this, [this]() { emit sendLog("Host found."); });
    connect(&socket, &QAbstractSocket::connected, this, [this]() {
        socket.setSocketOption(QAbstractSocket::LowDelayOption, true);
        socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
        setProtocolState(ProtocolState::AwaitingLogin);
        emit sendLog("Connection established.");
        emit connectionEstablished(this);
    });
    connect(&socket, &QSslSocket::encrypted, this, [this]() {
        timer.stop();
        secret
            = socket.peerCertificate().digest(QCryptographicHash::Algorithm::Sha1).toHex().toLower();
        emit sendLog("Connection successfully encrypted.");
        emit connectionEncrypted(this);
    });
    connect(&socket, &QAbstractSocket::disconnected, this, [this]() {
        timer.stop();
        emit connectionClosed(this);
    });
    connect(&socket, &QIODevice::readyRead, this, &GroupSocket::onReadyRead);
    connect(&socket,
            static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(
                &QAbstractSocket::error),
            this,
            &GroupSocket::onError);
    connect(&socket, &QSslSocket::peerVerifyError, this, &GroupSocket::onPeerVerifyError);
}

GroupSocket::~GroupSocket()
{
    timer.stop();
    socket.disconnectFromHost();
}

void GroupSocket::connectToHost()
{
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        emit sendLog("Aborting previous connection");
        socket.abort();
    }
    timer.start();
    const auto &groupConfig = getConfig().groupManager;
    const auto remoteHost = groupConfig.host;
    const auto remotePort = static_cast<quint16>(groupConfig.remotePort);
    emit sendLog(QString("Connecting to remote host %1:%2")
                     .arg(remoteHost.simplified().constData())
                     .arg(remotePort));
    socket.connectToHost(remoteHost, remotePort);
}

void GroupSocket::disconnectFromHost()
{
    timer.stop();
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        socket.flush();
        emit sendLog("Closing the socket. Quitting.");
        socket.disconnectFromHost();
        setProtocolState(ProtocolState::Unconnected);
    }
}

void GroupSocket::setSocket(qintptr socketDescriptor)
{
    if (!socket.setSocketDescriptor(socketDescriptor)) {
        qWarning() << "Connection failed. Native socket not recognized.";
        onError(QAbstractSocket::SocketAccessError);
        return;
    }
    socket.setSocketOption(QAbstractSocket::LowDelayOption, true);
    socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    setProtocolState(ProtocolState::AwaitingLogin);
    emit connectionEstablished(this);
}

void GroupSocket::setProtocolState(const ProtocolState val)
{
    timer.stop();
    if (DEBUG)
        qInfo() << "Protocol state:" << static_cast<int>(val);
    protocolState = val;
    switch (val) {
    case ProtocolState::AwaitingLogin:
        // Restart timer to verify that info was sent
        timer.start();
        break;
    case ProtocolState::AwaitingInfo:
        // Restart timer to verify that login occurred
        emit sendLog("Receiving group information...");
        timer.start();
        break;
    case ProtocolState::Logged:
        emit sendLog("Group information received. Login completed successfully.");
        break;
    case ProtocolState::Unconnected:
        break;
    default:
        abort();
    }
}

void GroupSocket::onError(QAbstractSocket::SocketError e)
{
    // Disconnecting and timeouts are not an error
    if (e != QAbstractSocket::RemoteHostClosedError && e != QAbstractSocket::SocketTimeoutError) {
        qDebug() << "onError" << static_cast<int>(e) << socket.errorString();
        timer.stop();
        emit errorInConnection(this, socket.errorString());
    }
}

void GroupSocket::onPeerVerifyError(const QSslError &error)
{
    // Ignore expected warnings
    if (error.error() == QSslError::SelfSignedCertificate)
        return;

    emit sendLog("<b>WARNING:</b> " + error.errorString());
    qWarning() << "onPeerVerifyError" << static_cast<int>(socket.error()) << socket.errorString()
               << error.errorString();
}

void GroupSocket::onTimeout()
{
    switch (socket.state()) {
    case QAbstractSocket::ConnectedState:
        switch (protocolState) {
        case ProtocolState::Unconnected:
        case ProtocolState::AwaitingLogin:
            if (!socket.isEncrypted()) {
                socket.disconnectFromHost();
                const QString msg = socket.isEncrypted() ? socket.errorString()
                                                         : "Connection not successfully encrypted";
                emit errorInConnection(this, msg);
                return;
            }
            goto continue_common_timeout;

        continue_common_timeout:
        case ProtocolState::AwaitingInfo:
            socket.disconnectFromHost();
            emit errorInConnection(this, "Login timed out");
            break;
        case ProtocolState::Logged:
            // Race condition? Protocol was successfully logged
            break;
        }
        break;
    case QAbstractSocket::HostLookupState:
        socket.abort();
        emit errorInConnection(this, "Host not found");
        return;
    case QAbstractSocket::UnconnectedState:
    case QAbstractSocket::ConnectingState:
    case QAbstractSocket::BoundState:
    case QAbstractSocket::ListeningState:
    case QAbstractSocket::ClosingState:
    default:
        socket.abort();
        emit errorInConnection(this, "Connection timed out");
        break;
    }
}

void GroupSocket::onReadyRead()
{
    io::readAllAvailable(socket, ioBuffer, [this](const QByteArray &byteArray) {
        buffer += byteArray;
        while (currentMessageLen < buffer.size()) {
            cutMessageFromBuffer();
        }
    });
}

void GroupSocket::cutMessageFromBuffer()
{
    // REVISIT: Turn this into a state machine
    if (currentMessageLen == 0) {
        // Find the next message length
        int spaceIndex = buffer.indexOf(' ');
        QString messageLengthStr = buffer.left(spaceIndex + 1);
        currentMessageLen = messageLengthStr.toInt();

        // Update buffer with remainder
        buffer = buffer.right(buffer.size() - spaceIndex - 1);

        // Do we have enough for a message?
        if (currentMessageLen == buffer.size())
            cutMessageFromBuffer();
        return;
    }

    // Cut message from buffer
    QByteArray message = buffer.left(currentMessageLen);
    if (DEBUG)
        qDebug() << "Incoming message:" << message;
    emit incomingData(this, message);
    buffer = buffer.right(buffer.size() - currentMessageLen);
    currentMessageLen = 0;
}

/*
 * Protocol is <message length as string> <space> <message XML>
 */
void GroupSocket::sendData(const QByteArray &data)
{
    QByteArray buff;
    QString len = QString("%1 ").arg(data.size());
    buff = len.toLatin1();
    buff += data;
    if (DEBUG)
        qDebug() << "Sending message:" << buff;
    socket.write(buff);
}
