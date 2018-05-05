/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
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

#ifndef MUMESOCKET_H
#define MUMESOCKET_H

#include <QObject>
#include <QAbstractSocket>

class QTcpSocket;
class QWebSocket;

class MumeSocket : public QObject
{
    Q_OBJECT
public:
    explicit MumeSocket(QObject *parent = nullptr);

    virtual void disconnectFromHost() = 0;
    virtual void connectToHost() = 0;
    virtual void sendToMud(const QByteArray &ba) = 0;

protected slots:
    void onConnect();
    void onDisconnect();
    void onError(QAbstractSocket::SocketError e);

signals:
    void connected();
    void disconnected();
    void socketError(QAbstractSocket::SocketError);
    void processMudStream(const QByteArray& buffer);

};

class MumeMudSocket : public MumeSocket
{
    Q_OBJECT
public:
    MumeMudSocket(QObject *parent);
    ~MumeMudSocket();

    void disconnectFromHost();
    void connectToHost();
    QAbstractSocket::SocketError error();
    void sendToMud(const QByteArray &ba);

protected slots:
    void onConnect();
    void onReadyRead();

private:
    char m_buffer[ 8192 ];

    QTcpSocket* m_socket;

};

class MumeWebSocket : public MumeSocket
{
    Q_OBJECT
public:
    MumeWebSocket(QObject *parent);
    ~MumeWebSocket();

    void disconnectFromHost();
    void connectToHost();
    QAbstractSocket::SocketError error();
    void sendToMud(const QByteArray &ba);

protected slots:
    void onBinaryMessageReceived(const QByteArray &);

private:
    QWebSocket* m_socket;

};

#endif // MUMESOCKET_H
