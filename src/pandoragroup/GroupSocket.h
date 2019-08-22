#pragma once
/************************************************************************
**
** Authors:   Azazello <lachupe@gmail.com>,
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

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
