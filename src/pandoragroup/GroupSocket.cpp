// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "GroupSocket.h"

#include "../configuration/configuration.h"
#include "../global/TextUtils.h"
#include "../global/io.h"
#include "../proxy/TaggedBytes.h"
#include "groupauthority.h"

#include <cassert>
#include <tuple>

#include <QByteArray>
#include <QHostAddress>
#include <QMessageLogContext>
#include <QSslConfiguration>
#include <QString>
#include <QTcpSocket>
#include <QTimer>

static constexpr const bool DEBUG = false;
static constexpr const auto THIRTY_SECOND_TIMEOUT = 30000;

GroupSocket::GroupSocket(GroupAuthority *const authority, QObject *const parent)
    : QObject(parent)
    , m_socket{this}
    , m_timer{this}
{
    auto &timer = m_timer;
    timer.setInterval(THIRTY_SECOND_TIMEOUT);
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, &GroupSocket::slot_onTimeout);

    const auto get_ssl_config = [this, authority]() {
        auto config = m_socket.sslConfiguration();
        config.setCaCertificates({});
        if (!NO_OPEN_SSL) {
            config.setLocalCertificate(authority->getLocalCertificate());
            config.setPrivateKey(authority->getPrivateKey());
        }
        config.setPeerVerifyMode(QSslSocket::QueryPeer);

        // CVE-2012-4929 forced the below option to be enabled by default but we can disable it because
        // the vulernability only impacts browsers
        config.setSslOption(QSsl::SslOption::SslOptionDisableCompression, false);
        return config;
    };
    auto &socket = m_socket;
    socket.setSslConfiguration(get_ssl_config());
    socket.setPeerVerifyName(GROUP_COMMON_NAME);
    connect(&socket, &QAbstractSocket::hostFound, this, [this]() {
        emit sig_sendLog("Host found...");
    });
    connect(&socket, &QAbstractSocket::connected, this, [this]() {
        m_socket.setSocketOption(QAbstractSocket::LowDelayOption, true);
        m_socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
        if (io::tuneKeepAlive(m_socket.socketDescriptor())) {
            emit sig_sendLog("Tuned TCP keep alive parameters for socket");
        }
        setProtocolState(ProtocolStateEnum::AwaitingLogin);
        emit sig_sendLog("Connection established...");
        emit sig_connectionEstablished(this);
    });
    connect(&socket, &QSslSocket::encrypted, this, [this]() {
        m_timer.stop();
        m_secret = GroupSecret{
            m_socket.peerCertificate().digest(QCryptographicHash::Algorithm::Sha1).toHex().toLower()};
        emit sig_sendLog("Connection successfully encrypted...");
        emit sig_connectionEncrypted(this);
    });
    connect(&socket, &QAbstractSocket::disconnected, this, [this]() {
        m_timer.stop();
        emit sig_connectionClosed(this);
    });
    connect(&socket, &QIODevice::readyRead, this, &GroupSocket::slot_onReadyRead);
    connect(&socket,
            QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this,
            &GroupSocket::slot_onError);
    connect(&socket, &QSslSocket::peerVerifyError, this, &GroupSocket::slot_onPeerVerifyError);
}

GroupSocket::~GroupSocket()
{
    m_timer.stop();
    m_socket.disconnectFromHost();
}

void GroupSocket::connectToHost()
{
    auto &socket = m_socket;
    bool retry = false;
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        retry = true;
        socket.abort();
    }
    reset();
    m_timer.start();
    const auto &groupConfig = getConfig().groupManager;
    const auto remoteHost = groupConfig.host;
    const auto remotePort = static_cast<uint16_t>(groupConfig.remotePort);
    sendLog(QString("%1 to remote host %2:%3")
                .arg(retry ? "Reconnecting" : "Connecting")
                .arg(remoteHost.simplified())
                .arg(remotePort));
    socket.connectToHost(remoteHost, remotePort);
}

void GroupSocket::disconnectFromHost()
{
    m_timer.stop();
    auto &socket = m_socket;
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        socket.flush();
        sendLog("Closing the socket. Quitting.");
        socket.disconnectFromHost();
        setProtocolState(ProtocolStateEnum::Unconnected);
    }
}

QString GroupSocket::getPeerName() const
{
    auto s = m_socket.peerName();
    if (!s.isEmpty()) {
        return s;
    }
    return m_socket.peerAddress().toString();
}

void GroupSocket::setSocket(qintptr socketDescriptor)
{
    auto &socket = m_socket;
    if (!socket.setSocketDescriptor(socketDescriptor)) {
        qWarning() << "Connection failed. Native socket not recognized.";
        slot_onError(QAbstractSocket::SocketAccessError);
        return;
    }
    socket.setSocketOption(QAbstractSocket::LowDelayOption, true);
    socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    if (io::tuneKeepAlive(socket.socketDescriptor())) {
        sendLog("Tuned TCP keep alive parameters for socket");
    }
    setProtocolState(ProtocolStateEnum::AwaitingLogin);
    emit sig_connectionEstablished(this);
}

void GroupSocket::setProtocolState(const ProtocolStateEnum val)
{
    auto &timer = m_timer;
    timer.stop();
    if (DEBUG) {
        qInfo() << "Protocol state:" << static_cast<int>(val);
    }
    m_protocolState = val;
    switch (val) {
    case ProtocolStateEnum::AwaitingLogin:
        // Restart timer to verify that info was sent
        timer.start();
        break;
    case ProtocolStateEnum::AwaitingInfo:
        // Restart timer to verify that login occurred
        sendLog("Receiving group information...");
        m_timer.start();
        break;
    case ProtocolStateEnum::Logged:
        sendLog("Group information received. Login completed successfully.");
        break;
    case ProtocolStateEnum::Unconnected:
        break;
    default:
        abort();
    }
}

void GroupSocket::slot_onError(QAbstractSocket::SocketError e)
{
    // Disconnecting and timeouts are not an error
    if (e != QAbstractSocket::RemoteHostClosedError && e != QAbstractSocket::SocketTimeoutError) {
        qDebug() << "onError" << static_cast<int>(e) << m_socket.errorString();
        m_timer.stop();
        emit sig_errorInConnection(this, m_socket.errorString());
    }
}

void GroupSocket::slot_onPeerVerifyError(const QSslError &error)
{
    // Ignore expected warnings
    if (error.error() == QSslError::SelfSignedCertificate) {
        return;
    }

    sendLog("<b>WARNING:</b> " + error.errorString());
    qWarning() << "onPeerVerifyError" << static_cast<int>(m_socket.error())
               << m_socket.errorString() << error.errorString();
}

void GroupSocket::slot_onTimeout()
{
    auto &socket = m_socket;
    switch (socket.state()) {
    case QAbstractSocket::ConnectedState:
        switch (m_protocolState) {
        case ProtocolStateEnum::Unconnected:
        case ProtocolStateEnum::AwaitingLogin:
            if (!socket.isEncrypted()) {
                const QString msg = socket.isEncrypted() ? socket.errorString()
                                                         : "Connection not successfully encrypted";
                emit sig_errorInConnection(this, msg);
                return;
            }
            goto continue_common_timeout;

        continue_common_timeout:
        case ProtocolStateEnum::AwaitingInfo:
            emit sig_errorInConnection(this, "Login timed out");
            break;
        case ProtocolStateEnum::Logged:
            // Race condition? Protocol was successfully logged
            break;
        }
        break;
    case QAbstractSocket::HostLookupState:
        emit sig_errorInConnection(this, "Host not found");
        return;
    case QAbstractSocket::UnconnectedState:
    case QAbstractSocket::ConnectingState:
    case QAbstractSocket::BoundState:
    case QAbstractSocket::ListeningState:
    case QAbstractSocket::ClosingState:
    default:
        emit sig_errorInConnection(this, "Connection timed out");
        break;
    }
}

void GroupSocket::slot_onReadyRead()
{
    // REVISIT: check return value?
    std::ignore = io::readAllAvailable(m_socket, m_ioBuffer, [this](const QByteArray &byteArray) {
        assert(!byteArray.isEmpty());
        for (const auto &c : byteArray) {
            onReadInternal(c);
        }
    });
}

void GroupSocket::onReadInternal(const char c)
{
    auto &buffer = m_buffer;
    auto &currentMessageLen = m_currentMessageLen;
    switch (m_state) {
    case GroupMessageStateEnum::LENGTH:
        if (c == char_consts::C_SPACE && currentMessageLen > 0) {
            // Terminating space received
            m_state = GroupMessageStateEnum::PAYLOAD;
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
            if (DEBUG) {
                qDebug() << "Incoming message:" << buffer;
            }
            emit sig_incomingData(this, QString::fromUtf8(buffer.getQByteArray()));

            // Reset state machine
            buffer.clear();
            currentMessageLen = 0;
            m_state = GroupMessageStateEnum::LENGTH;
        }
        break;
    }
}

/*
 * Protocol is <message length as string> <space> <message XML>
 */
void GroupSocket::sendData(const QString &data)
{
    if (m_socket.state() != QAbstractSocket::ConnectedState) {
        qWarning() << "Socket is not connected";
        return;
    }

    // This is a special case; we're formatting the bytes to be sent to the socket,
    auto utf8 = data.toUtf8();
    QString len = QString("%1 ").arg(utf8.size());
    QByteArray buff = len.toUtf8(); // it's actually ASCII
    buff += utf8;
    if (DEBUG) {
        qDebug() << "Sending message:" << buff;
    }
    m_socket.write(buff);
}

void GroupSocket::reset()
{
    m_protocolState = ProtocolStateEnum::Unconnected;
    m_protocolVersion = defaultProtocolVersion;
    m_buffer.clear();
    m_secret.clear();
    m_name.clear();
    m_state = GroupMessageStateEnum::LENGTH;
    m_currentMessageLen = 0;
}
