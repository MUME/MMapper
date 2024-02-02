#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QAbstractSocket>
#include <QByteArray>
#include <QObject>
#ifndef Q_OS_WASM
#include <QSslSocket>
#else
#include <QTcpSocket>
#endif
#include <QString>
#include <QTimer>
#include <QtCore>

#include "../global/io.h"

class QSslError;

class MumeSocket : public QObject
{
    Q_OBJECT
public:
    explicit MumeSocket(QObject *parent)
        : QObject(parent)
    {}

private:
    virtual void virt_disconnectFromHost() = 0;
    virtual void virt_connectToHost() = 0;
    virtual void virt_sendToMud(const QByteArray &ba) = 0;
    NODISCARD virtual QAbstractSocket::SocketState virt_state() = 0;
    virtual void virt_onConnect();
    virtual void virt_onDisconnect();
    virtual void virt_onError(QAbstractSocket::SocketError e) = 0;
    virtual void virt_onError2(QAbstractSocket::SocketError e, const QString &errorString);

protected:
    void proxy_log(const QString &msg) { emit sig_log("Proxy", msg); }

public:
    void disconnectFromHost() { virt_disconnectFromHost(); }
    void connectToHost() { virt_connectToHost(); }
    void sendToMud(const QByteArray &ba) { virt_sendToMud(ba); }
    NODISCARD QAbstractSocket::SocketState state() { return virt_state(); }

protected slots:
    void slot_onConnect() { virt_onConnect(); }
    void slot_onDisconnect() { virt_onDisconnect(); }
    void slot_onError(QAbstractSocket::SocketError e) { virt_onError(e); }
    void slot_onError2(QAbstractSocket::SocketError e, const QString &errorString)
    {
        virt_onError2(e, errorString);
    }

signals:
    void sig_connected();
    void sig_disconnected();
    void sig_socketError(const QString &errorString);
    void sig_processMudStream(const QByteArray &buffer);
    void sig_log(const QString &, const QString &);
};

class MumeSslSocket : public MumeSocket
{
    Q_OBJECT
public:
    explicit MumeSslSocket(QObject *parent);
    ~MumeSslSocket() override;

private:
    void virt_disconnectFromHost() final;
    void virt_connectToHost() override;
    void virt_sendToMud(const QByteArray &ba) final;
    NODISCARD QAbstractSocket::SocketState virt_state() final { return m_socket.state(); }
    void virt_onConnect() override;
    void virt_onError(QAbstractSocket::SocketError e) final;

protected slots:
    void slot_onReadyRead();
    void slot_checkTimeout();

#ifndef Q_OS_WASM
public slots:
    void slot_onEncrypted();
    void slot_onPeerVerifyError(const QSslError &error);

protected:
    QSslSocket m_socket;
#else
protected:
    QTcpSocket m_socket;
#endif

    io::buffer<(1 << 13)> m_buffer;
    QTimer m_timer;
};

class MumeTcpSocket final : public MumeSslSocket
{
    Q_OBJECT
public:
    explicit MumeTcpSocket(QObject *parent)
        : MumeSslSocket(parent)
    {}

private:
    void virt_connectToHost() final;
    void virt_onConnect() final;
};
