#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QAbstractSocket>
#include <QByteArray>
#include <QObject>
#include <QSslSocket>
#include <QString>
#include <QTimer>
#include <QtCore>

#include "../global/io.h"

class QSslError;

class MumeSocket : public QObject
{
    Q_OBJECT
public:
    explicit MumeSocket(QObject *parent = nullptr)
        : QObject(parent)
    {}

    virtual void disconnectFromHost() = 0;
    virtual void connectToHost() = 0;
    virtual void sendToMud(const QByteArray &ba) = 0;
    virtual QAbstractSocket::SocketState state() = 0;

protected slots:
    virtual void onConnect();
    virtual void onDisconnect();
    virtual void onError(QAbstractSocket::SocketError e) = 0;
    virtual void onError2(QAbstractSocket::SocketError e, const QString &errorString);

signals:
    void connected();
    void disconnected();
    void socketError(const QString &errorString);
    void processMudStream(const QByteArray &buffer);
    void log(const QString &, const QString &);
};

class MumeSslSocket : public MumeSocket
{
    Q_OBJECT
public:
    explicit MumeSslSocket(QObject *parent);
    ~MumeSslSocket() override;

    void disconnectFromHost() override;
    void connectToHost() override;
    void sendToMud(const QByteArray &ba) override;
    QAbstractSocket::SocketState state() override { return m_socket.state(); }

protected slots:
    void onConnect() override;
    void onError(QAbstractSocket::SocketError e) override;
    void onReadyRead();
    void onEncrypted();
    void onPeerVerifyError(const QSslError &error);
    void checkTimeout();

protected:
    io::null_padded_buffer<(1 << 13)> m_buffer{};
    QSslSocket m_socket;
    QTimer m_timer;
};

class MumeTcpSocket final : public MumeSslSocket
{
    Q_OBJECT
public:
    explicit MumeTcpSocket(QObject *parent)
        : MumeSslSocket(parent)
    {}

    void connectToHost() override;

protected slots:
    void onConnect() override;
};
