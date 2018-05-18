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
class QSslSocket;
class QSslError;
class QTimer;

class MumeSocket : public QObject
{
    Q_OBJECT
public:
    explicit MumeSocket(QObject *parent = nullptr) : QObject(parent) {}

    virtual void disconnectFromHost() = 0;
    virtual void connectToHost() = 0;
    virtual void sendToMud(const QByteArray &ba) = 0;

protected slots:
    virtual void onConnect();
    virtual void onDisconnect();
    virtual void onError(QAbstractSocket::SocketError e);

signals:
    void connected();
    void disconnected();
    void socketError(QAbstractSocket::SocketError);
    void processMudStream(const QByteArray &buffer);
    void log(const QString &, const QString &);

};

class MumeSslSocket : public MumeSocket
{
    Q_OBJECT
public:
    MumeSslSocket(QObject *parent);
    ~MumeSslSocket();

    void disconnectFromHost();
    void connectToHost();
    QAbstractSocket::SocketError error();
    void sendToMud(const QByteArray &ba);

protected slots:
    void onConnect();
    void onError(QAbstractSocket::SocketError e);
    void onReadyRead();
    void onEncrypted();
    void onPeerVerifyError(const QSslError &error);
    void checkTimeout();

protected:
    char m_buffer[ 8192 ] {};

    QSslSocket *m_socket;
    QTimer *m_timer;

};

class MumeTcpSocket : public MumeSslSocket
{
    Q_OBJECT
public:
    MumeTcpSocket(QObject *parent) : MumeSslSocket(parent) {}

    void connectToHost() override;

protected slots:
    void onConnect() override;

};

#endif // MUMESOCKET_H
