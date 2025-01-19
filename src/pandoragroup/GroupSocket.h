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

class NODISCARD_QOBJECT GroupSocket final : public QObject
{
    Q_OBJECT

private:
    QSslSocket m_socket;
    QTimer m_timer;

    ProtocolStateEnum m_protocolState = ProtocolStateEnum::Unconnected;
    ProtocolVersion m_protocolVersion = 102;

    enum class NODISCARD GroupMessageStateEnum {
        /// integer string representing the message length
        LENGTH,
        /// message payload
        PAYLOAD
    } m_state
        = GroupMessageStateEnum::LENGTH;

    io::buffer<(1 << 15)> m_ioBuffer;
    QByteArray m_buffer;
    QByteArray m_secret;
    QByteArray m_name;
    unsigned int m_currentMessageLen = 0;

public:
    explicit GroupSocket(GroupAuthority *authority, QObject *parent);
    ~GroupSocket() final;

    void setSocket(qintptr socketDescriptor);
    void connectToHost();
    void disconnectFromHost();
    void startServerEncrypted() { m_socket.startServerEncryption(); }
    void startClientEncrypted() { m_socket.startClientEncryption(); }

    NODISCARD QByteArray getSecret() const { return m_secret; }
    NODISCARD QString getPeerName() const;
    NODISCARD uint16_t getPeerPort() const { return m_socket.peerPort(); }

    NODISCARD QAbstractSocket::SocketError getSocketError() const { return m_socket.error(); }
    NODISCARD QSslCertificate getPeerCertificate() const { return m_socket.peerCertificate(); }

    void setProtocolState(ProtocolStateEnum val);
    NODISCARD ProtocolStateEnum getProtocolState() const { return m_protocolState; }

    void setProtocolVersion(const ProtocolVersion val) { m_protocolVersion = val; }
    NODISCARD ProtocolVersion getProtocolVersion() const { return m_protocolVersion; }

    void setName(const QByteArray &val) { m_name = val; }
    NODISCARD const QByteArray &getName() { return m_name; }

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
    void onReadInternal(char c);
};
