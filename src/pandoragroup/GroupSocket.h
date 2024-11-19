#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/io.h"

#include <QAbstractSocket>
#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QSslSocket>
#include <QString>
#include <QTimer>
#include <QtCore>
#include <QtGlobal>

class GroupAuthority;

enum class NODISCARD ProtocolStateEnum { Unconnected, AwaitingLogin, AwaitingInfo, Logged };
using ProtocolVersion = uint32_t;

class GroupSocket final : public QObject
{
    Q_OBJECT
public:
    explicit GroupSocket(GroupAuthority *authority, QObject *parent);
    ~GroupSocket() final;

    void setSocket(qintptr socketDescriptor);
    void connectToHost();
    void disconnectFromHost();
    void startServerEncrypted() { socket.startServerEncryption(); }
    void startClientEncrypted() { socket.startClientEncryption(); }

    NODISCARD QByteArray getSecret() const { return secret; }
    NODISCARD QString getPeerName() const;
    NODISCARD quint16 getPeerPort() const { return socket.peerPort(); }

    NODISCARD QAbstractSocket::SocketError getSocketError() const { return socket.error(); }
    NODISCARD QSslCertificate getPeerCertificate() const { return socket.peerCertificate(); }

    void setProtocolState(ProtocolStateEnum val);
    NODISCARD ProtocolStateEnum getProtocolState() const { return protocolState; }

    void setProtocolVersion(const ProtocolVersion val) { protocolVersion = val; }
    NODISCARD ProtocolVersion getProtocolVersion() { return protocolVersion; }

    void setName(const QByteArray &val) { name = val; }
    NODISCARD const QByteArray &getName() { return name; }

    void sendData(const QByteArray &data);

protected slots:
    void slot_onError(QAbstractSocket::SocketError socketError);
    void slot_onPeerVerifyError(const QSslError &error);
    void slot_onReadyRead();
    void slot_onTimeout();

signals:
    void sig_sendLog(const QString &);
    void sig_connectionClosed(GroupSocket *);
    void sig_errorInConnection(GroupSocket *, const QString &);
    void sig_incomingData(GroupSocket *, QByteArray);
    void sig_connectionEstablished(GroupSocket *);
    void sig_connectionEncrypted(GroupSocket *);

private:
    void reset();
    void sendLog(const QString &msg) { emit sig_sendLog(msg); }

private:
    QSslSocket socket;
    QTimer timer;
    void onReadInternal(char c);

    ProtocolStateEnum protocolState = ProtocolStateEnum::Unconnected;
    ProtocolVersion protocolVersion = 102;

    enum class NODISCARD GroupMessageStateEnum {
        /// integer string representing the message length
        LENGTH,
        /// message payload
        PAYLOAD
    } state
        = GroupMessageStateEnum::LENGTH;

    io::buffer<(1 << 15)> ioBuffer;
    QByteArray buffer;
    QByteArray secret;
    QByteArray name;
    unsigned int currentMessageLen = 0;
};
