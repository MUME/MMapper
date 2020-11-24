// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mumesocket.h"

#include <QByteArray>
#include <QLocale>
#include <QMessageLogContext>
#include <QSslSocket>
#include <QString>
#include <QtNetwork>

#include "../configuration/configuration.h"
#include "../global/io.h"

static constexpr int TIMEOUT_MILLIS = 10000;

void MumeSocket::onConnect()
{
    emit connected();
}

void MumeSocket::onDisconnect()
{
    emit disconnected();
}

void MumeSocket::onError2(QAbstractSocket::SocketError e, const QString &errorString)
{
    QString errorStr = errorString;
    if (!errorStr.at(errorStr.length() - 1).isPunct())
        errorStr += "!";
    switch (e) {
    case QAbstractSocket::SslInvalidUserDataError:
    case QAbstractSocket::SslInternalError:
    case QAbstractSocket::SslHandshakeFailedError:
        errorStr += "\r\n\r\n"
                    "Uncheck TLS encryption under the MMapper preferences at your own risk.";
        break;

    case QAbstractSocket::ConnectionRefusedError:
    case QAbstractSocket::RemoteHostClosedError:
    case QAbstractSocket::HostNotFoundError:
    case QAbstractSocket::SocketAccessError:
    case QAbstractSocket::SocketResourceError:
    case QAbstractSocket::SocketTimeoutError:
    case QAbstractSocket::DatagramTooLargeError:
    case QAbstractSocket::NetworkError:
    case QAbstractSocket::AddressInUseError:
    case QAbstractSocket::SocketAddressNotAvailableError:
    case QAbstractSocket::UnsupportedSocketOperationError:
    case QAbstractSocket::UnfinishedSocketOperationError:
    case QAbstractSocket::ProxyAuthenticationRequiredError:
    case QAbstractSocket::ProxyConnectionRefusedError:
    case QAbstractSocket::ProxyConnectionClosedError:
    case QAbstractSocket::ProxyConnectionTimeoutError:
    case QAbstractSocket::ProxyNotFoundError:
    case QAbstractSocket::ProxyProtocolError:
    case QAbstractSocket::OperationError:
    case QAbstractSocket::TemporaryError:
    case QAbstractSocket::UnknownSocketError:
    default:
        break;
    }
    emit socketError(errorStr);
}

MumeSslSocket::MumeSslSocket(QObject *parent)
    : MumeSocket(parent)
    , m_socket{this}
    , m_timer{this}
{
    const auto get_ssl_config = [this]() {
        auto config = m_socket.sslConfiguration();
        config.setPeerVerifyMode(QSslSocket::QueryPeer);
        return config;
    };
    m_socket.setSslConfiguration(get_ssl_config());
    connect(&m_socket,
            QOverload<>::of(&QAbstractSocket::connected),
            this,
            &MumeSslSocket::onConnect);
    connect(&m_socket, &QIODevice::readyRead, this, &MumeSslSocket::onReadyRead);
    connect(&m_socket, &QAbstractSocket::disconnected, this, &MumeSslSocket::onDisconnect);
    connect(&m_socket, &QSslSocket::encrypted, this, &MumeSslSocket::onEncrypted);
    connect(&m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this,
            &MumeSslSocket::onError);
    connect(&m_socket, &QSslSocket::peerVerifyError, this, &MumeSslSocket::onPeerVerifyError);

    m_timer.setInterval(TIMEOUT_MILLIS);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &MumeSslSocket::checkTimeout);
}

MumeSslSocket::~MumeSslSocket()
{
    m_socket.disconnectFromHost();
    m_timer.stop();
}

void MumeSslSocket::connectToHost()
{
    const auto &settings = getConfig().connection;
    m_socket.connectToHostEncrypted(settings.remoteServerName,
                                    settings.remotePort,
                                    QIODevice::ReadWrite);
    m_timer.start();
}

void MumeSslSocket::disconnectFromHost()
{
    m_timer.stop();
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.disconnectFromHost();
    }
}

void MumeSslSocket::onConnect()
{
    emit log("Proxy", "Negotiating handshake with server ...");
    m_socket.setSocketOption(QAbstractSocket::LowDelayOption, true);
    m_socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    if (io::tuneKeepAlive(m_socket.socketDescriptor())) {
        emit log("Proxy", "Tuned TCP keep alive parameters for socket");
    }
    // Restart timer to check if encryption has successfully finished
    m_timer.start();
}

void MumeSslSocket::onError(QAbstractSocket::SocketError e)
{
    // MUME disconnecting is not an error. We also handle timeouts separately.
    if (e != QAbstractSocket::RemoteHostClosedError && e != QAbstractSocket::SocketTimeoutError) {
        m_timer.stop();
        onError2(e, m_socket.errorString());
    }
}

void MumeSslSocket::onEncrypted()
{
    m_timer.stop();
    emit log("Proxy", "Connection now encrypted ...");
    constexpr const bool LOG_CERT_INFO = true;
    if ((LOG_CERT_INFO)) {
        /* TODO: If we save the cert to config file, then we can notify the user if it changes! */
        const auto cert = m_socket.peerCertificate();
        const auto commonNameList = cert.subjectInfo(cert.CommonName);
        const auto commonName = commonNameList.isEmpty() ? "(n/a)" : commonNameList.front();
        const auto sha1 = cert.digest(QCryptographicHash::Algorithm::Sha1).toHex(':').toStdString();
        const auto expStr = QLocale::system().toString(cert.expiryDate(), QLocale::LongFormat);
        emit log("Proxy", QString("Peer certificate common name: %1.").arg(commonName));
        emit log("Proxy", QString("Peer certificate SHA1: %1.").arg(sha1.c_str()));
        emit log("Proxy", QString("Peer certificate expires: %1.").arg(expStr));
    }

    MumeSocket::onConnect();
}

void MumeSslSocket::onPeerVerifyError(const QSslError &error)
{
    emit log("Proxy", "<b>WARNING:</b> " + error.errorString());
    qWarning() << "onPeerVerifyError" << m_socket.errorString() << error.errorString();

    // Warn user of possible compromise
    QByteArray byteArray = QByteArray("\r\n\033[1;37;41mENCRYPTION WARNING:\033[0;37;41m ")
                               .append(error.errorString().toLatin1())
                               .append("!\033[0m\r\n\r\n");
    emit processMudStream(byteArray);
}

void MumeSslSocket::onReadyRead()
{
    io::readAllAvailable(m_socket, m_buffer, [this](const QByteArray &byteArray) {
        if (!byteArray.isEmpty())
            emit processMudStream(byteArray);
    });
}

void MumeSslSocket::checkTimeout()
{
    switch (m_socket.state()) {
    case QAbstractSocket::ConnectedState:
        if (!m_socket.isEncrypted()) {
            onError2(QAbstractSocket::SslHandshakeFailedError,
                     "Timeout during encryption handshake.");
            return;
        } else {
            // Race condition? Connection was successfully encrypted
        }
        break;
    case QAbstractSocket::HostLookupState:
        m_socket.abort();
        onError2(QAbstractSocket::HostNotFoundError, "Could not find host!");
        return;

    case QAbstractSocket::UnconnectedState:
    case QAbstractSocket::ConnectingState:
    case QAbstractSocket::BoundState:
    case QAbstractSocket::ListeningState:
    case QAbstractSocket::ClosingState:
    default:
        m_socket.abort();
        onError2(QAbstractSocket::SocketTimeoutError, "Connection timed out!");
        break;
    }
}

void MumeSslSocket::sendToMud(const QByteArray &ba)
{
    if (m_socket.state() != QAbstractSocket::ConnectedState) {
        qWarning() << "Socket is not connected";
        return;
    }
    m_socket.write(ba);
}

void MumeTcpSocket::connectToHost()
{
    const auto &settings = getConfig().connection;
    m_socket.connectToHost(settings.remoteServerName, settings.remotePort);
    m_timer.start();
}

void MumeTcpSocket::onConnect()
{
    m_timer.stop();
    m_socket.setSocketOption(QAbstractSocket::LowDelayOption, true);
    m_socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    if (io::tuneKeepAlive(m_socket.socketDescriptor())) {
        emit log("Proxy", "Tuned TCP keep alive parameters for socket");
    }

    if (!NO_OPEN_SSL) {
        // Warn user of the insecure connection
        QByteArray byteArray = QByteArray(
            "\r\n\033[1;37;41mWARNING:\033[0;37;41m "
            "This connection is not secure! Disconnect and enable TLS encryption under the MMapper"
            " preferences to get rid of this message."
            "\033[0m\r\n\r\n");
        emit processMudStream(byteArray);
    }

    MumeSocket::onConnect();
}
