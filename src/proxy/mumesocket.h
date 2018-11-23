#pragma once
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

protected slots:
    virtual void onConnect();
    virtual void onDisconnect();
    virtual void onError(QAbstractSocket::SocketError e) = 0;
    virtual void onError2(QAbstractSocket::SocketError e, const QString &errorString = QString());

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

#endif // MUMESOCKET_H
