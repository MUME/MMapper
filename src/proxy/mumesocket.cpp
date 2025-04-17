// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mumesocket.h"

#include "../configuration/configuration.h"
#include "../global/Consts.h"
#include "../global/MakeQPointer.h"
#include "../global/io.h"

#include <tuple>

#include <QByteArray>
#include <QLocale>
#include <QMessageLogContext>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QString>

static constexpr int PING_MILLIS = 45000;
static constexpr int TIMEOUT_MILLIS = 5000;
static constexpr auto ENCRYPTION_WARNING = "ENCRYPTION WARNING";
static constexpr auto CONNECTION_WARNING = "Warning";

MumeSocketOutputs::~MumeSocketOutputs() = default;

MumeFallbackSocket::MumeFallbackSocket(QObject *parent, MumeSocketOutputs &outputs)
    : QObject(parent)
    , m_timer{this}
    , m_outputs{outputs}
{
    m_timer.setInterval(TIMEOUT_MILLIS);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        assert(m_socket);
        switch (m_socket->state()) {
        case QAbstractSocket::ConnectedState:
            break;
        case QAbstractSocket::HostLookupState:
        case QAbstractSocket::UnconnectedState:
        case QAbstractSocket::ConnectingState:
        case QAbstractSocket::BoundState:
        case QAbstractSocket::ListeningState:
        case QAbstractSocket::ClosingState:
        default:
            m_socket->deleteLater();
            onSocketError(QStringLiteral("Connection has timed out due to network issues."));
            break;
        }
    });

    struct NODISCARD OutputWrapper final : public MumeSocketOutputs
    {
    private:
        MumeFallbackSocket &m_fallback;
        MumeSocketOutputs &m_outputs;

    public:
        explicit OutputWrapper(MumeFallbackSocket &fallback, MumeSocketOutputs &outputs)
            : m_fallback{fallback}
            , m_outputs{outputs}
        {}

    private:
        void virt_onConnected() final
        {
            m_fallback.stopTimer();
            m_outputs.onConnected();
        }
        void virt_onDisconnected() final { m_outputs.onDisconnected(); }
        void virt_onSocketWarning(const AnsiWarningMessage &msg) final
        {
            m_outputs.onSocketWarning(msg);
        }
        void virt_onSocketError(const QString &errorString) final
        {
            m_fallback.onSocketError(errorString);
        }
        void virt_onSocketStatus(const QString &statusString) final
        {
            m_outputs.onSocketStatus(statusString);
        }
        void virt_onProcessMudStream(const TelnetIacBytes &buffer) final
        {
            m_outputs.onProcessMudStream(buffer);
        }
        void virt_onLog(const QString &s) final { m_outputs.onLog(s); }
    };
    m_wrapper = std::make_unique<OutputWrapper>(*this, outputs);
}

MumeFallbackSocket::~MumeFallbackSocket()
{
    stopTimer();
    m_socket->deleteLater();
}

void MumeFallbackSocket::disconnectFromHost()
{
    stopTimer();
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
    m_state = SocketTypeEnum::SSL;
}

void MumeFallbackSocket::connectToHost()
{
    stopTimer();
    if (m_state == SocketTypeEnum::SSL && !QSslSocket::supportsSsl()) {
        m_state = SocketTypeEnum::WEBSOCKET;
    }
    if (m_state == SocketTypeEnum::WEBSOCKET && NO_WEBSOCKET) {
        m_state = SocketTypeEnum::INSECURE;
    }
    if (m_state == SocketTypeEnum::INSECURE && (QSslSocket::supportsSsl() || !NO_WEBSOCKET)
        && getConfig().connection.tlsEncryption) {
        // Request user to disable encryption
        m_outputs.onSocketError("Attempt was rejected because insecure connections are"
                                " disabled in your MMapper preferences. Disable requiring"
                                " encryption at your own risk and try again.");
        disconnectFromHost();
        return;
    }

    m_socket->deleteLater();
    auto &wrapper = deref(m_wrapper);
    switch (m_state) {
    case SocketTypeEnum::SSL:
        m_socket = mmqt::makeQPointer<MumeSslSocket>(this, wrapper);
        break;
    case SocketTypeEnum::WEBSOCKET:
        assert(!NO_WEBSOCKET);
        m_socket = mmqt::makeQPointer<MumeWebSocket>(this, wrapper);
        break;
    case SocketTypeEnum::INSECURE:
        m_socket = mmqt::makeQPointer<MumeTcpSocket>(this, wrapper);
        break;
    default:
        assert(false);
    }
    m_socket->connectToHost();
    m_timer.start();
}

void MumeFallbackSocket::sendToMud(const TelnetIacBytes &ba)
{
    assert(m_socket);
    m_socket->sendToMud(ba);
}

void MumeFallbackSocket::onSocketError(const QString &errorString)
{
    if (m_state == SocketTypeEnum::INSECURE) {
        m_outputs.onSocketError(errorString);
        disconnectFromHost();
        return;
    } else {
        m_outputs.onSocketWarning(AnsiWarningMessage{AnsiColor16Enum::white,
                                                     AnsiColor16Enum::yellow,
                                                     CONNECTION_WARNING,
                                                     errorString});
    }

    if (m_state == SocketTypeEnum::SSL) {
        m_state = NO_WEBSOCKET ? SocketTypeEnum::INSECURE : SocketTypeEnum::WEBSOCKET;
    } else if (m_state == SocketTypeEnum::WEBSOCKET) {
        m_state = SocketTypeEnum::INSECURE;
    }
    switch (m_state) {
    case SocketTypeEnum::WEBSOCKET:
        m_outputs.onSocketStatus("Attempting using WebSocket...");
        break;
    case SocketTypeEnum::INSECURE:
        m_outputs.onSocketStatus("Attempting insecure plain text...");
        break;
    case SocketTypeEnum::SSL:
    default:
        assert(false);
    };
    connectToHost();
}

bool MumeFallbackSocket::isConnectedOrConnecting() const
{
    return m_socket ? m_socket->isConnectedOrConnecting() : false;
}

void MumeSocket::virt_onError2(const QString &errorString)
{
    QString errorStr = errorString;
    if (!errorStr.at(errorStr.length() - 1).isPunct()) {
        errorStr += "!";
    }
    m_outputs.onSocketError(errorStr);
}

MumeSslSocket::MumeSslSocket(QObject *parent, MumeSocketOutputs &outputs)
    : MumeSocket(parent, outputs)
    , m_socket{this}
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
}

MumeSslSocket::~MumeSslSocket()
{
    m_socket.abort();
    m_socket.close();
}

void MumeSslSocket::virt_connectToHost()
{
    // REVISIT: Most clients tell the user where they're connecting.
    const auto &settings = getConfig().connection;
    m_socket.connectToHostEncrypted(settings.remoteServerName,
                                    settings.remotePort,
                                    QIODevice::ReadWrite);
}

void MumeSslSocket::virt_disconnectFromHost()
{
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.disconnectFromHost();
    }
}

QAbstractSocket::SocketState MumeSslSocket::virt_state()
{
    if (m_socket.state() == QAbstractSocket::ConnectedState && !m_socket.isEncrypted()) {
        return QAbstractSocket::SocketState::ConnectingState;
    }
    return m_socket.state();
}

void MumeSslSocket::virt_onConnect()
{
    proxy_log("Negotiating handshake with server ...");
    m_socket.setSocketOption(QAbstractSocket::LowDelayOption, true);
    m_socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    if (io::tuneKeepAlive(m_socket.socketDescriptor())) {
        proxy_log("Tuned TCP keep alive parameters for socket");
    }
}

void MumeSslSocket::virt_onError(QAbstractSocket::SocketError e)
{
    // MUME disconnecting is not an error. We also handle timeouts separately.
    if (e != QAbstractSocket::RemoteHostClosedError && e != QAbstractSocket::SocketTimeoutError) {
        onError2(m_socket.errorString());
    }
}

void MumeSslSocket::slot_onEncrypted()
{
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

    proxy_log("WARNING: " + msg);
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
}

void MumeTcpSocket::virt_onConnect()
{
    // Warn user of the insecure connection
    if (!QSslSocket::supportsSsl() && NO_WEBSOCKET) {
        m_outputs.onSocketWarning(
            AnsiWarningMessage{AnsiColor16Enum::white,
                               AnsiColor16Enum::red,
                               ENCRYPTION_WARNING,
                               "This connection is not secure! Disconnect and recompile"
                               " MMapper with OpenSSL or WebSocket support to get rid of"
                               " this message."});

    } else if (QSslSocket::supportsSsl() || !NO_WEBSOCKET) {
        m_outputs.onSocketWarning(
            AnsiWarningMessage{AnsiColor16Enum::white,
                               AnsiColor16Enum::red,
                               ENCRYPTION_WARNING,
                               "This connection is not secure! Disconnect and enable secure"
                               " connections under the MMapper preferences to get rid of"
                               " this message."});
    }

    m_socket.setSocketOption(QAbstractSocket::LowDelayOption, true);
    m_socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    if (io::tuneKeepAlive(m_socket.socketDescriptor())) {
        proxy_log("Tuned TCP keep alive parameters for socket");
    }

    m_outputs.onConnected();
}

MumeWebSocket::MumeWebSocket(QObject *parent, MumeSocketOutputs &outputs)
    : MumeSocket(parent, outputs)
#ifndef MMAPPER_NO_WEBSOCKET
    , m_socket{QLatin1String(""), QWebSocketProtocol::VersionLatest, this}
#endif
{
#ifndef MMAPPER_NO_WEBSOCKET
    connect(&m_socket,
            &QWebSocket::binaryMessageReceived,
            this,
            &MumeWebSocket::slot_onBinaryMessageReceived);
    connect(&m_socket, &QWebSocket::disconnected, this, [this]() { onDisconnect(); });
    connect(&m_socket, &QWebSocket::connected, this, [this]() { onConnect(); });
    connect(&m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this,
            [this](QAbstractSocket::SocketError e) { onError(e); });
    connect(&m_socket,
            QOverload<const QList<QSslError> &>::of(&QWebSocket::sslErrors),
            this,
            &MumeWebSocket::onSslErrors);
    connect(&m_pingTimer, &QTimer::timeout, &m_socket, [this]() { m_socket.ping(); });
#endif

    // Periodically ping to avoid proxies killing the connection
    m_pingTimer.setInterval(PING_MILLIS);
}

MumeWebSocket::~MumeWebSocket()
{
#ifndef MMAPPER_NO_WEBSOCKET
    m_socket.close();
#endif
    m_pingTimer.stop();
}

void MumeWebSocket::virt_connectToHost()
{
    const auto &conf = getConfig().connection;
    QUrl url;
    url.setScheme("wss");
    url.setHost(conf.remoteServerName);
    url.setPort(443);
    url.setPath("/ws-play/");

#ifndef MMAPPER_NO_WEBSOCKET
    QNetworkRequest req(url);
    req.setRawHeader("Sec-WebSocket-Protocol", "binary");
    m_socket.open(req);
#endif
}

void MumeWebSocket::virt_onConnect()
{
    m_pingTimer.start();

    m_outputs.onConnected();
}

void MumeWebSocket::virt_disconnectFromHost()
{
#ifndef MMAPPER_NO_WEBSOCKET
    m_socket.close();
#endif
    m_pingTimer.stop();
}

void MumeWebSocket::slot_onBinaryMessageReceived(const QByteArray &byteArray)
{
    m_outputs.onProcessMudStream(TelnetIacBytes{byteArray});
    m_pingTimer.start();
}

void MumeWebSocket::virt_sendToMud(const TelnetIacBytes &ba)
{
    if (!isConnectedOrConnecting()) {
        qWarning() << "Socket is not connected";
        return;
    }
#ifndef MMAPPER_NO_WEBSOCKET
    m_socket.sendBinaryMessage(ba.getQByteArray());
#endif
    m_pingTimer.start();
}

void MumeWebSocket::virt_onError(QAbstractSocket::SocketError e)
{
    // MUME disconnecting is not an error. We also handle timeouts separately.
    if (e != QAbstractSocket::RemoteHostClosedError && e != QAbstractSocket::SocketTimeoutError) {
#ifndef MMAPPER_NO_WEBSOCKET
        onError2(m_socket.errorString());
#endif
        m_pingTimer.stop();
    }
}

void MumeWebSocket::onSslErrors(const QList<QSslError> &errors)
{
    QString msg;
    for (const auto &error : errors) {
        if (!msg.isEmpty())
            msg += char_consts::C_SEMICOLON + char_consts::C_SPACE;
        msg += error.errorString();
    }
    if (!msg.isEmpty() && !msg.back().isPunct()) {
        msg += char_consts::C_PERIOD;
    }

    proxy_log("WARNING: " + msg);
#ifndef MMAPPER_NO_WEBSOCKET
    qWarning() << "onSslErrors" << m_socket.errorString() << msg;
#endif

    m_outputs.onSocketWarning(
        AnsiWarningMessage{AnsiColor16Enum::white, AnsiColor16Enum::red, ENCRYPTION_WARNING, msg});
}
