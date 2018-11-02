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

#include "CGroupClient.h"

#include <QByteArray>
#include <QMessageLogContext>
#include <QString>

CGroupClient::CGroupClient(QObject *parent)
    : QTcpSocket(parent)
{
    connect(this, &QAbstractSocket::connected, this, [this]() {
        this->setConnectionState(ConnectionStates::Connected);
    });
    connect(this, &QAbstractSocket::disconnected, this, [this]() {
        this->setConnectionState(ConnectionStates::Closed);
    });
    connect(this, &QIODevice::readyRead, this, &CGroupClient::onReadyRead);
    connect(this,
            static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(
                &QAbstractSocket::error),
            this,
            &CGroupClient::onError);
}

CGroupClient::~CGroupClient()
{
    disconnectFromHost();
    qDebug() << "Destructed CGroupClient" << socketDescriptor();
}

void CGroupClient::setSocket(qintptr socketDescriptor)
{
    if (!setSocketDescriptor(socketDescriptor)) {
        qWarning("Connection failed. Native socket not recognized.");
        onError(QAbstractSocket::SocketAccessError);
        return;
    }
    setSocketOption(QAbstractSocket::KeepAliveOption, true);
    setConnectionState(ConnectionStates::Connected);
}

void CGroupClient::setConnectionState(const ConnectionStates val)
{
    //    qInfo("Connection state: %i", val);
    connectionState = val;
    switch (val) {
    case ConnectionStates::Connecting:
        emit sendLog(QString("Connecting to remote host."));
        break;
    case ConnectionStates::Connected:
        emit sendLog(QString("Connection established."));
        setProtocolState(ProtocolStates::AwaitingLogin);
        emit connectionEstablished(this);
        break;
    case ConnectionStates::Closed:
        emit sendLog(QString("Connection closed."));
        emit connectionClosed(this);
        break;
    case ConnectionStates::Quiting:
        emit sendLog(QString("Closing the socket. Quitting."));
        break;
    default:
        qWarning("Unknown state change: %i", static_cast<int>(val));
        break;
    }
}

void CGroupClient::onError(QAbstractSocket::SocketError /*socketError*/)
{
    setConnectionState(ConnectionStates::Quiting);
    emit errorInConnection(this, errorString());
}

void CGroupClient::onReadyRead()
{
    QByteArray message;
    QByteArray rest;

    //    qInfo("Incoming Data [conn %i, IP: %s]", socketDescriptor(),
    //            (const char *) peerAddress().toString().toLatin1() );

    QByteArray tmp = readAll();

    buffer += tmp;

    //      qInfo("RAW data buffer: %s", (const char *) buffer);

    while (currentMessageLen < buffer.size()) {
        //              qInfo("in data-receiving cycle, buffer %s", (const char *) buffer);
        cutMessageFromBuffer();
    }
}

void CGroupClient::cutMessageFromBuffer()
{
    QByteArray rest;

    if (currentMessageLen == 0) {
        int index = buffer.indexOf(' ');

        QString len = buffer.left(index + 1);
        currentMessageLen = len.toInt();
        //              qInfo("Incoming buffer length: %i, incoming message length %i",
        //                              buffer.size(), currentMessageLen);

        rest = buffer.right(buffer.size() - index - 1);
        buffer = rest;

        if (buffer.size() == currentMessageLen) {
            cutMessageFromBuffer();
        }

        //qInfo("returning from cutMessageFromBuffer");
        return;
    }

    //      qInfo("cutting off one message case");
    emit incomingData(this, buffer.left(currentMessageLen));
    rest = buffer.right(buffer.size() - currentMessageLen);
    buffer = rest;
    currentMessageLen = 0;
}

void CGroupClient::sendData(const QByteArray &data)
{
    //      qInfo("%i ", data.size());
    QByteArray buff;
    QString len = QString("%1 ").arg(data.size());
    buff = len.toLatin1();
    buff += data;
    write(buff);
}
