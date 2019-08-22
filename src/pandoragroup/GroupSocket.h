#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QAbstractSocket>
#include <QByteArray>
#include <QHostAddress>
#include <QMap>
#include <QObject>
#include <QSslSocket>
#include <QString>
#include <QTimer>
#include <QtCore>
#include <QtGlobal>

#include "../global/io.h"

class GroupAuthority;

enum class ProtocolState { Unconnected, AwaitingLogin, AwaitingInfo, Logged };
using ProtocolVersion = uint32_t;

class GroupSocket final : public QObject
{
    Q_OBJECT
public:
    explicit GroupSocket(GroupAuthority *authority, QObject *parent);
    virtual ~GroupSocket();

    void setSocket(qintptr socketDescriptor);
    void connectToHost();
    void disconnectFromHost();
    void startServerEncrypted() { socket.startServerEncryption(); }
    void startClientEncrypted() { socket.startClientEncryption(); }

    QByteArray getSecret() const { return secret; }
    QHostAddress getPeerAddress() const { return socket.peerAddress(); }
    quint16 getPeerPort() const { return socket.peerPort(); }

    QAbstractSocket::SocketError getSocketError() const { return socket.error(); }
    QSslCertificate getPeerCertificate() const { return socket.peerCertificate(); }

    void setProtocolState(const ProtocolState val);
    ProtocolState getProtocolState() const { return protocolState; }

    void setProtocolVersion(const ProtocolVersion val) { protocolVersion = val; }
    ProtocolVersion getProtocolVersion() { return protocolVersion; }

    void setName(const QByteArray val) { name = val; }
    const QByteArray &getName() { return name; }

    void sendData(const QByteArray &data);

protected slots:
    void onError(QAbstractSocket::SocketError socketError);
    void onPeerVerifyError(const QSslError &error);
    void onReadyRead();
    void onTimeout();

signals:
    void sendLog(const QString &);
    void connectionClosed(GroupSocket *);
    void errorInConnection(GroupSocket *, const QString &);
    void incomingData(GroupSocket *, QByteArray);
    void connectionEstablished(GroupSocket *);
    void connectionEncrypted(GroupSocket *);

private:
    void reset();

    QSslSocket socket;
    QTimer timer;
    GroupAuthority *authority;
    void onReadInternal(const char c);

    ProtocolState protocolState = ProtocolState::Unconnected;
    ProtocolVersion protocolVersion = 102;

    enum class GroupMessageState {
        /// integer string representing the messge length
        MESSAGE_LENGTH,
        /// message payload
        MESSAGE_PAYLOAD
    } state
        = GroupMessageState::MESSAGE_LENGTH;

    io::null_padded_buffer<(1 << 15)> ioBuffer{};
    QByteArray buffer{};
    QByteArray secret{};
    QByteArray name{};
    unsigned int currentMessageLen = 0;
};
