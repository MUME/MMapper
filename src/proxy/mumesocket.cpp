// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mumesocket.h"

#include "../configuration/configuration.h"
#include "../global/AnsiOstream.h"
#include "../global/Consts.h"
#include "../global/io.h"

#include <sstream>
#include <tuple>

#include <QByteArray>
#include <QLocale>
#include <QMessageLogContext>
#include <QSslSocket>
#include <QString>
#include <QtNetwork>

static constexpr int TIMEOUT_MILLIS = 30000;
static constexpr auto ENCRYPTION_WARNING = "ENCRYPTION WARNING";

MumeSocketOutputs::~MumeSocketOutputs() = default;

void MumeSocket::virt_onConnect()
{
    m_outputs.onConnected();
}

void MumeSocket::virt_onDisconnect()
{
    m_outputs.onDisconnected();
}

void MumeSocket::virt_onError2(QAbstractSocket::SocketError e, const QString &errorString)
{
    QString errorStr = errorString;
    if (!errorStr.at(errorStr.length() - 1).isPunct()) {
        errorStr += "!";
    }

    switch (e) {
    case QAbstractSocket::SslInvalidUserDataError:
    case QAbstractSocket::SslInternalError:
    case QAbstractSocket::SslHandshakeFailedError:
        errorStr += "\n"
                    "\n"
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
    m_outputs.onSocketError(errorStr);
}

MumeSslSocket::MumeSslSocket(QObject *parent, MumeSocketOutputs &outputs)
    : MumeSocket(parent, outputs)
    , m_socket{this}
    , m_timer{this}
{
    const auto get_ssl_config = [this]() {
        auto config = m_socket.sslConfiguration();
        config.setPeerVerifyMode(QSslSocket::QueryPeer);
        return config;
    };
    m_socket.setSslConfiguration(get_ssl_config());

    connect(&m_socket, QOverload<>::of(&QAbstractSocket::connected), this, [this]() {
        onConnect();
    });
    connect(&m_socket, &QIODevice::readyRead, this, &MumeSslSocket::slot_onReadyRead);
    connect(&m_socket, &QAbstractSocket::disconnected, this, [this]() { onDisconnect(); });
    connect(&m_socket, &QSslSocket::encrypted, this, &MumeSslSocket::slot_onEncrypted);
    connect(&m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this,
            [this](QAbstractSocket::SocketError e) { this->MumeSslSocket::onError(e); });
    connect(&m_socket, &QSslSocket::peerVerifyError, this, &MumeSslSocket::slot_onPeerVerifyError);

    m_timer.setInterval(TIMEOUT_MILLIS);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &MumeSslSocket::slot_checkTimeout);
}

MumeSslSocket::~MumeSslSocket()
{
    m_socket.disconnectFromHost();
    m_timer.stop();
}

void MumeSslSocket::virt_connectToHost()
{
    // REVISIT: Most clients tell the user where they're connecting.
    const auto &settings = getConfig().connection;
    m_socket.connectToHostEncrypted(settings.remoteServerName,
                                    settings.remotePort,
                                    QIODevice::ReadWrite);
    m_timer.start();
}

void MumeSslSocket::virt_disconnectFromHost()
{
    m_timer.stop();
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.disconnectFromHost();
    }
}

void MumeSslSocket::virt_onConnect()
{
    proxy_log("Negotiating handshake with server ...");
    m_socket.setSocketOption(QAbstractSocket::LowDelayOption, true);
    m_socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    if (io::tuneKeepAlive(m_socket.socketDescriptor())) {
        proxy_log("Tuned TCP keep alive parameters for socket");
    }
    // Restart timer to check if encryption has successfully finished
    m_timer.start();
}

void MumeSslSocket::virt_onError(QAbstractSocket::SocketError e)
{
    // MUME disconnecting is not an error. We also handle timeouts separately.
    if (e != QAbstractSocket::RemoteHostClosedError && e != QAbstractSocket::SocketTimeoutError) {
        m_timer.stop();
        onError2(e, m_socket.errorString());
    }
}

void MumeSslSocket::slot_onEncrypted()
{
    m_timer.stop();
    // This message is misleading; text is buffered until the encryption succeeds.
    proxy_log("Connection now encrypted ...");
    constexpr const bool LOG_CERT_INFO = true;
    if ((LOG_CERT_INFO)) {
        /* TODO: If we save the cert to config file, then we can notify the user if it changes! */
        const auto cert = m_socket.peerCertificate();
        const auto commonNameList = cert.subjectInfo(QSslCertificate::CommonName);
        const auto commonName = commonNameList.isEmpty() ? "(n/a)" : commonNameList.front();
        const auto sha1 = cert.digest(QCryptographicHash::Algorithm::Sha1)
                              .toHex(char_consts::C_COLON)
                              .toStdString();
        const auto expStr = QLocale::system().toString(cert.expiryDate(), QLocale::LongFormat);
        proxy_log(QString("Peer certificate common name: %1.").arg(commonName));
        proxy_log(QString("Peer certificate SHA1: %1.").arg(sha1.c_str()));
        proxy_log(QString("Peer certificate expires: %1.").arg(expStr));
    }

    m_outputs.onConnected();
}

void MumeSslSocket::slot_onPeerVerifyError(const QSslError &error)
{
    auto msg = error.errorString();
    if (!msg.isEmpty() && !msg.back().isPunct()) {
        msg += char_consts::C_PERIOD;
    }

    // REVISIT: avoid sending HTML directly; send a title and message instead,
    // and let the logger decide how to format the title.
    //
    // (e.g. Maybe the handler for sig3_socketWarning should be the one to format and send the message.)
    proxy_log("<b>WARNING:</b> " + msg);
    qWarning() << "onPeerVerifyError" << m_socket.errorString() << msg;

    m_outputs.onSocketWarning(
        AnsiWarningMessage{AnsiColor16Enum::white, AnsiColor16Enum::red, ENCRYPTION_WARNING, msg});
}

void MumeSslSocket::slot_onReadyRead()
{
    // REVISIT: check return value?
    std::ignore = io::readAllAvailable(m_socket, m_buffer, [this](const QByteArray &byteArray) {
        assert(!byteArray.isEmpty());
        m_outputs.onProcessMudStream(TelnetIacBytes{byteArray});
    });
}

void MumeSslSocket::slot_checkTimeout()
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

void MumeSslSocket::virt_sendToMud(const TelnetIacBytes &ba)
{
    if (!isConnectedOrConnecting()) {
        qWarning() << "Socket is not connected";
        return;
    }
    m_socket.write(ba.getQByteArray());
}

void MumeTcpSocket::virt_connectToHost()
{
    const auto &settings = getConfig().connection;
    m_socket.connectToHost(settings.remoteServerName, settings.remotePort);
    m_timer.start();
}

void MumeTcpSocket::virt_onConnect()
{
    m_timer.stop();
    m_socket.setSocketOption(QAbstractSocket::LowDelayOption, true);
    m_socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    if (io::tuneKeepAlive(m_socket.socketDescriptor())) {
        proxy_log("Tuned TCP keep alive parameters for socket");
    }

    if (QSslSocket::supportsSsl()) {
        // Warn user of the insecure connection
        m_outputs.onSocketWarning(AnsiWarningMessage{
            AnsiColor16Enum::white,
            AnsiColor16Enum::red,
            ENCRYPTION_WARNING,
            "This connection is not secure! Disconnect and enable TLS encryption under the MMapper"
            " preferences to get rid of this message."});
    }

    m_outputs.onConnected();
}
