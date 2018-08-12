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
#include <QString>
#include <QTcpSocket>
#include <QtCore>
#include <QtGlobal>

class QObject;

enum class ConnectionStates { Closed, Connecting, Connected, Quiting };
enum class ProtocolStates { Idle, AwaitingLogin, AwaitingInfo, Logged };

class CGroupClient : public QTcpSocket
{
    Q_OBJECT
public:
    explicit CGroupClient(QObject *parent);
    virtual ~CGroupClient();

    void setSocket(qintptr socketDescriptor);

    ConnectionStates getConnectionState() const { return connectionState; }
    void setConnectionState(ConnectionStates val);
    void setProtocolState(ProtocolStates val) { protocolState = val; }
    ProtocolStates getProtocolState() const { return protocolState; }
    void sendData(const QByteArray &data);

protected slots:
    void onError(QAbstractSocket::SocketError socketError);
    void onReadyRead();

signals:
    void sendLog(const QString &);
    void connectionClosed(CGroupClient *);
    void errorInConnection(CGroupClient *, const QString &);
    void incomingData(CGroupClient *, QByteArray);
    void connectionEstablished(CGroupClient *);

private:
    void cutMessageFromBuffer();

    ConnectionStates connectionState = ConnectionStates::Closed;
    ProtocolStates protocolState = ProtocolStates::Idle;

    QByteArray buffer{};
    int currentMessageLen = 0;
};

#endif /*CGROUPCLIENT_H_*/
