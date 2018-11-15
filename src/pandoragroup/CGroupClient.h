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

#ifndef CGROUPCLIENT_H_
#define CGROUPCLIENT_H_

#include <QAbstractSocket>
#include <QByteArray>
#include <QHostAddress>
#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QtCore>
#include <QtGlobal>

#include "../global/io.h"

class QTimer;
enum class ProtocolStates { Unconnected, AwaitingLogin, AwaitingInfo, Logged };

class CGroupClient final : public QObject
{
    Q_OBJECT
public:
    explicit CGroupClient(QObject *parent);
    virtual ~CGroupClient();

    void setSocket(qintptr socketDescriptor);
    void connectToHost();
    void disconnectFromHost();

    QHostAddress getPeerAddress() const { return socket.peerAddress(); }
    QAbstractSocket::SocketError getSocketError() const { return socket.error(); }

    QAbstractSocket::SocketState getSocketState() const { return socket.state(); }
    void setProtocolState(const ProtocolStates val);
    ProtocolStates getProtocolState() const { return protocolState; }
    void sendData(const QByteArray &data);

protected slots:
    void onError(QAbstractSocket::SocketError socketError);
    void onReadyRead();
    void onTimeout();

signals:
    void sendLog(const QString &);
    void connectionClosed(CGroupClient *);
    void errorInConnection(CGroupClient *, const QString &);
    void incomingData(CGroupClient *, QByteArray);
    void connectionEstablished(CGroupClient *);

private:
    QTcpSocket socket;
    QTimer *timer;
    void cutMessageFromBuffer();

    ProtocolStates protocolState = ProtocolStates::Unconnected;

    io::null_padded_buffer<(1 << 15)> ioBuffer{};
    QByteArray buffer{};
    int currentMessageLen = 0;
};

#endif /*CGROUPCLIENT_H_*/
