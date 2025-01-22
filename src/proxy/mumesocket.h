#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/AnsiTextUtils.h"
#include "../global/Signal2.h"
#include "../global/io.h"
#include "TaggedBytes.h"

#include <QAbstractSocket>
#include <QByteArray>
#include <QObject>
#include <QSslSocket>
#include <QString>
#include <QTimer>
#include <QtCore>

class QSslError;

struct NODISCARD AnsiWarningMessage final
{
    AnsiColor16Enum fg = AnsiColor16Enum::white;
    AnsiColor16Enum bg = AnsiColor16Enum::red;
    QString title;
    QString msg;
};

struct NODISCARD MumeSocketOutputs
{
public:
    virtual ~MumeSocketOutputs();

public:
    void onConnected() { virt_onConnected(); }
    void onDisconnected() { virt_onDisconnected(); }
    void onSocketWarning(const AnsiWarningMessage &msg) { virt_onSocketWarning(msg); }
    void onSocketError(const QString &msg) { virt_onSocketError(msg); }
    void onProcessMudStream(const TelnetIacBytes &bytes) { virt_onProcessMudStream(bytes); }
    void onLog(const QString &msg) { virt_onLog(msg); }

private:
    virtual void virt_onConnected() = 0;
    virtual void virt_onDisconnected() = 0;
    virtual void virt_onSocketWarning(const AnsiWarningMessage &) = 0;
    virtual void virt_onSocketError(const QString & /*errorString*/) = 0;
    virtual void virt_onProcessMudStream(const TelnetIacBytes & /*buffer*/) = 0;
    virtual void virt_onLog(const QString &) = 0;
};

class NODISCARD_QOBJECT MumeSocket : public QObject
{
    Q_OBJECT

protected:
    MumeSocketOutputs &m_outputs;

public:
    explicit MumeSocket(QObject *parent, MumeSocketOutputs &outputs)
        : QObject(parent)
        , m_outputs{outputs}
    {}

private:
    virtual void virt_disconnectFromHost() = 0;
    virtual void virt_connectToHost() = 0;
    virtual void virt_sendToMud(const TelnetIacBytes &) = 0;
    NODISCARD virtual QAbstractSocket::SocketState virt_state() = 0;
    virtual void virt_onConnect();
    virtual void virt_onDisconnect();
    virtual void virt_onError(QAbstractSocket::SocketError e) = 0;
    virtual void virt_onError2(QAbstractSocket::SocketError e, const QString &errorString);

protected:
    void proxy_log(const QString &msg) { m_outputs.onLog(msg); }
    void onConnect() { virt_onConnect(); }
    void onDisconnect() { virt_onDisconnect(); }
    void onError(QAbstractSocket::SocketError e) { virt_onError(e); }
    void onError2(QAbstractSocket::SocketError e, const QString &errorString)
    {
        virt_onError2(e, errorString);
    }

public:
    void disconnectFromHost() { virt_disconnectFromHost(); }
    void connectToHost() { virt_connectToHost(); }
    void sendToMud(const TelnetIacBytes &ba) { virt_sendToMud(ba); }
    void sendToMud(const QString &s) { sendToMud(TelnetIacBytes{s.toUtf8()}); }
    NODISCARD QAbstractSocket::SocketState state() { return virt_state(); }
    NODISCARD bool isConnectedOrConnecting()
    {
        switch (state()) {
        case QAbstractSocket::ConnectingState:
        case QAbstractSocket::ConnectedState:
            return true;
        case QAbstractSocket::UnconnectedState:
        case QAbstractSocket::HostLookupState:
        case QAbstractSocket::BoundState:
        case QAbstractSocket::ListeningState:
        case QAbstractSocket::ClosingState:
            return false;
        }
        std::abort();
    }
};

class NODISCARD_QOBJECT MumeSslSocket : public MumeSocket
{
    Q_OBJECT

protected:
    io::buffer<(1 << 13)> m_buffer;
    QSslSocket m_socket;
    QTimer m_timer;

public:
    explicit MumeSslSocket(QObject *parent, MumeSocketOutputs &outputs);
    ~MumeSslSocket() override;

private:
    void virt_disconnectFromHost() final;
    void virt_connectToHost() override;
    void virt_sendToMud(const TelnetIacBytes &ba) final;
    NODISCARD QAbstractSocket::SocketState virt_state() final { return m_socket.state(); }
    void virt_onConnect() override;
    void virt_onError(QAbstractSocket::SocketError e) final;

protected slots:
    void slot_onReadyRead();
    void slot_onEncrypted();
    void slot_onPeerVerifyError(const QSslError &error);
    void slot_checkTimeout();
};

class NODISCARD_QOBJECT MumeTcpSocket final : public MumeSslSocket
{
    Q_OBJECT

public:
    explicit MumeTcpSocket(QObject *parent, MumeSocketOutputs &outputs)
        : MumeSslSocket(parent, outputs)
    {}

private:
    void virt_connectToHost() final;
    void virt_onConnect() final;
};
