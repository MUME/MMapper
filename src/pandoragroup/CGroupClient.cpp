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

void CGroupClient::linkSignals()
{
    connect(this, &QAbstractSocket::disconnected, this, &CGroupClient::lostConnection);
    connect(this, &QIODevice::readyRead, this, &CGroupClient::dataIncoming);
    connect(this,
            SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(errorHandler(QAbstractSocket::SocketError)));

    connect(this, SIGNAL(sendLog(const QString &)), parent(), SLOT(relayLog(const QString &)));
    connect(this,
            SIGNAL(errorInConnection(CGroupClient *, const QString &)),
            parent(),
            SLOT(errorInConnection(CGroupClient *, const QString &)));
}

CGroupClient::CGroupClient(const QByteArray &host, const int remotePort, QObject *const parent)
    : QTcpSocket(parent)
{
    linkSignals();
    connect(this,
            SIGNAL(incomingData(CGroupClient *, QByteArray)),
            parent,
            SLOT(incomingData(CGroupClient *, QByteArray)));
    setConnectionState(ConnectionStates::Connecting);
    connectToHost(host, static_cast<quint16>(remotePort));
    protocolState = ProtocolStates::AwaitingLogin;
    if (!waitForConnected(5000)) {
        if (getConnectionState() == ConnectionStates::Connecting) {
            errorHandler(QAbstractSocket::SocketTimeoutError);
        }
    } else {
        // Linux needs to have this option set after the server has established a connection
        setSocketOption(QAbstractSocket::KeepAliveOption, true);
        connectionEstablished();
    }
}

CGroupClient::CGroupClient(QObject *parent)
    : QTcpSocket(parent)
{
    linkSignals();
}

void CGroupClient::setSocket(qintptr socketDescriptor)
{
    if (!setSocketDescriptor(socketDescriptor)) {
        qWarning("Connection failed. Native socket not recognized.");
        errorHandler(QAbstractSocket::SocketAccessError);
        return;
    }
    setSocketOption(QAbstractSocket::KeepAliveOption, true);
    setConnectionState(ConnectionStates::Connected);
}

void CGroupClient::setProtocolState(ProtocolStates val)
{
    //    qInfo("Protocol state: %i", val);
    protocolState = val;
}

void CGroupClient::setConnectionState(ConnectionStates val)
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

CGroupClient::~CGroupClient()
{
    disconnectFromHost();
}

void CGroupClient::lostConnection()
{
    setConnectionState(ConnectionStates::Closed);
}

void CGroupClient::connectionEstablished()
{
    setConnectionState(ConnectionStates::Connected);
}

void CGroupClient::errorHandler(QAbstractSocket::SocketError /*socketError*/)
{
    setConnectionState(ConnectionStates::Quiting);
    emit errorInConnection(this, errorString());
}

void CGroupClient::dataIncoming()
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
