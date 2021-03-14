// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "GroupSocket.h"

#include <cassert>
#include <QByteArray>
#include <QHostAddress>
#include <QMessageLogContext>
#include <QSslConfiguration>
#include <QString>
#include <QTcpSocket>
#include <QTimer>

#include "../configuration/configuration.h"
#include "../global/io.h"
#include "groupauthority.h"

static constexpr const bool DEBUG = false;
static constexpr const auto THIRTY_SECOND_TIMEOUT = 30000;

GroupSocket::GroupSocket(GroupAuthority *authority, QObject *parent)
    : QObject(parent)
    , socket{this}
    , timer{this}
    , authority(authority)
{
    timer.setInterval(THIRTY_SECOND_TIMEOUT);
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, &GroupSocket::onTimeout);

    const auto get_ssl_config = [this, authority]() {
        auto config = socket.sslConfiguration();
        config.setCaCertificates({});
        config.setLocalCertificate(authority->getLocalCertificate());
        config.setPrivateKey(authority->getPrivateKey());
        config.setPeerVerifyMode(QSslSocket::QueryPeer);

        // CVE-2012-4929 forced the below option to be enabled by default but we can disable it because
        // the vulernability only impacts browsers
        config.setSslOption(QSsl::SslOption::SslOptionDisableCompression, false);
        return config;
    };
    socket.setSslConfiguration(get_ssl_config());
    socket.setPeerVerifyName(GROUP_COMMON_NAME);
    connect(&socket, &QAbstractSocket::hostFound, this, [this]() { emit sendLog("Host found..."); });
    connect(&socket, &QAbstractSocket::connected, this, [this]() {
        socket.setSocketOption(QAbstractSocket::LowDelayOption, true);
        socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
        if (io::tuneKeepAlive(socket.socketDescriptor())) {
            emit sendLog("Tuned TCP keep alive parameters for socket");
        }
        setProtocolState(ProtocolStateEnum::AwaitingLogin);
        emit sendLog("Connection established...");
        emit connectionEstablished(this);
    });
    connect(&socket, &QSslSocket::encrypted, this, [this]() {
        timer.stop();
        secret
            = socket.peerCertificate().digest(QCryptographicHash::Algorithm::Sha1).toHex().toLower();
        emit sendLog("Connection successfully encrypted...");
        emit connectionEncrypted(this);
    });
    connect(&socket, &QAbstractSocket::disconnected, this, [this]() {
        timer.stop();
        emit connectionClosed(this);
    });
    connect(&socket, &QIODevice::readyRead, this, &GroupSocket::onReadyRead);
    connect(&socket,
            QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
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
    bool retry = false;
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        retry = true;
        socket.abort();
    }
    reset();
    timer.start();
    const auto &groupConfig = getConfig().groupManager;
    const auto remoteHost = groupConfig.host;
    const auto remotePort = static_cast<quint16>(groupConfig.remotePort);
    emit sendLog(QString("%1 to remote host %2:%3")
                     .arg(retry ? "Reconnecting" : "Connecting")
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
        setProtocolState(ProtocolStateEnum::Unconnected);
    }
}

QString GroupSocket::getPeerName() const
{
    const auto s = socket.peerName();
    if (!s.isEmpty())
        return s;
    return socket.peerAddress().toString();
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
    if (io::tuneKeepAlive(socket.socketDescriptor())) {
        emit sendLog("Tuned TCP keep alive parameters for socket");
    }
    setProtocolState(ProtocolStateEnum::AwaitingLogin);
    emit connectionEstablished(this);
}

void GroupSocket::setProtocolState(const ProtocolStateEnum val)
{
    timer.stop();
    if (DEBUG)
        qInfo() << "Protocol state:" << static_cast<int>(val);
    protocolState = val;
    switch (val) {
    case ProtocolStateEnum::AwaitingLogin:
        // Restart timer to verify that info was sent
        timer.start();
        break;
    case ProtocolStateEnum::AwaitingInfo:
        // Restart timer to verify that login occurred
        emit sendLog("Receiving group information...");
        timer.start();
        break;
    case ProtocolStateEnum::Logged:
        emit sendLog("Group information received. Login completed successfully.");
        break;
    case ProtocolStateEnum::Unconnected:
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
        case ProtocolStateEnum::Unconnected:
        case ProtocolStateEnum::AwaitingLogin:
            if (!socket.isEncrypted()) {
                const QString msg = socket.isEncrypted() ? socket.errorString()
                                                         : "Connection not successfully encrypted";
                emit errorInConnection(this, msg);
                return;
            }
            goto continue_common_timeout;

        continue_common_timeout:
        case ProtocolStateEnum::AwaitingInfo:
            emit errorInConnection(this, "Login timed out");
            break;
        case ProtocolStateEnum::Logged:
            // Race condition? Protocol was successfully logged
            break;
        }
        break;
    case QAbstractSocket::HostLookupState:
        emit errorInConnection(this, "Host not found");
        return;
    case QAbstractSocket::UnconnectedState:
    case QAbstractSocket::ConnectingState:
    case QAbstractSocket::BoundState:
    case QAbstractSocket::ListeningState:
    case QAbstractSocket::ClosingState:
    default:
        emit errorInConnection(this, "Connection timed out");
        break;
    }
}

void GroupSocket::onReadyRead()
{
    // REVISIT: check return value?
    MAYBE_UNUSED const auto ignored = //
        io::readAllAvailable(socket, ioBuffer, [this](const QByteArray &byteArray) {
            for (const auto &c : byteArray) {
                onReadInternal(c);
            }
        });
}

void GroupSocket::onReadInternal(const char c)
{
    switch (state) {
    case GroupMessageStateEnum::LENGTH:
        if (c == ' ' && currentMessageLen > 0) {
            // Terminating space received
            state = GroupMessageStateEnum::PAYLOAD;
        } else if (c >= '0' && c <= '9') {
            // Digit received
            currentMessageLen *= 10;
            currentMessageLen += static_cast<unsigned int>(c - '0');
        } else {
            // Reset due to garbage
            currentMessageLen = 0;
        }
        break;
    case GroupMessageStateEnum::PAYLOAD:
        buffer.append(c);
        if (static_cast<unsigned int>(buffer.size()) == currentMessageLen) {
            // Cut message from buffer
            if (DEBUG)
                qDebug() << "Incoming message:" << buffer;
            emit incomingData(this, buffer);

            // Reset state machine
            buffer.clear();
            currentMessageLen = 0;
            state = GroupMessageStateEnum::LENGTH;
        }
        break;
    }
}

/*
 * Protocol is <message length as string> <space> <message XML>
 */
void GroupSocket::sendData(const QByteArray &data)
{
    if (socket.state() != QAbstractSocket::ConnectedState) {
        qWarning() << "Socket is not connected";
        return;
    }
    QByteArray buff;
    QString len = QString("%1 ").arg(data.size());
    buff = len.toLatin1();
    buff += data;
    if (DEBUG)
        qDebug() << "Sending message:" << buff;
    socket.write(buff);
}

void GroupSocket::reset()
{
    protocolState = ProtocolStateEnum::Unconnected;
    protocolVersion = 102;
    buffer.clear();
    secret.clear();
    name.clear();
    state = GroupMessageStateEnum::LENGTH;
    currentMessageLen = 0;
}
