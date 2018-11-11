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
#include <QSslSocket>
#include <QString>
#include <QTimer>

#include "../configuration/configuration.h"
#include "../global/io.h"

static constexpr const bool DEBUG = false;

CGroupClient::CGroupClient(QObject *parent)
    : QObject(parent)
    , timer(new QTimer(this))
{
    timer->setInterval(5000);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &CGroupClient::onTimeout);

    connect(&socket, &QAbstractSocket::connected, this, [this]() {
        timer->stop();
        socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
        this->setConnectionState(ConnectionStates::Connected);
    });
    connect(&socket, &QAbstractSocket::disconnected, this, [this]() {
        this->setConnectionState(ConnectionStates::Closed);
        timer->stop();
    });
    connect(&socket, &QIODevice::readyRead, this, &CGroupClient::onReadyRead);
    connect(&socket,
            static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(
                &QAbstractSocket::error),
            this,
            &CGroupClient::onError);
}

CGroupClient::~CGroupClient()
{
    if (timer)
        delete timer;
    socket.disconnectFromHost();
    qDebug() << "Destructed CGroupClient" << socket.socketDescriptor();
}

void CGroupClient::connectToHost()
{
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        socket.abort();
    }
    setConnectionState(ConnectionStates::Connecting);
    setProtocolState(ProtocolStates::AwaitingLogin);

    const auto &groupConfig = getConfig().groupManager;
    const auto remoteHost = groupConfig.host;
    const auto remotePort = static_cast<quint16>(groupConfig.remotePort);
    socket.connectToHost(remoteHost, remotePort);
    timer->start();
}

void CGroupClient::disconnectFromHost()
{
    socket.close();
}

void CGroupClient::setSocket(qintptr socketDescriptor)
{
    if (!socket.setSocketDescriptor(socketDescriptor)) {
        qWarning("Connection failed. Native socket not recognized.");
        onError(QAbstractSocket::SocketAccessError);
        return;
    }
    socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    setConnectionState(ConnectionStates::Connected);
    // REVISIT: Do we need to start the timer?
    timer->start();
}

void CGroupClient::setConnectionState(const ConnectionStates val)
{
    if (DEBUG)
        qInfo() << "Connection state:" << static_cast<int>(val);
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
    timer->stop();
    setConnectionState(ConnectionStates::Quiting);
    emit errorInConnection(this, socket.errorString());
}

void CGroupClient::onTimeout()
{
    switch (socket.state()) {
    case QAbstractSocket::ConnectedState:
        // Race condition? Connection was successfully connected
        break;
    case QAbstractSocket::HostLookupState:
        socket.abort();
        emit errorInConnection(this, "Host not found");
        return;
    default:
        socket.abort();
        emit errorInConnection(this, "Connection timed out");
        break;
    }
}

void CGroupClient::onReadyRead()
{
    io::readAllAvailable(socket, ioBuffer, [this](const QByteArray &byteArray) {
        buffer += byteArray;
        while (currentMessageLen < buffer.size()) {
            cutMessageFromBuffer();
        }
    });
}

void CGroupClient::cutMessageFromBuffer()
{
    // REVISIT: Turn this into a state machine
    if (currentMessageLen == 0) {
        // Find the next message length
        int spaceIndex = buffer.indexOf(' ');
        QString messageLengthStr = buffer.left(spaceIndex + 1);
        currentMessageLen = messageLengthStr.toInt();

        // Update buffer with remainder
        buffer = buffer.right(buffer.size() - spaceIndex - 1);

        // Do we have enough for a message?
        if (currentMessageLen == buffer.size())
            cutMessageFromBuffer();
        return;
    }

    // Cut message from buffer
    QByteArray message = buffer.left(currentMessageLen);
    if (DEBUG)
        qDebug() << "Incoming message:" << message;
    emit incomingData(this, message);
    buffer = buffer.right(buffer.size() - currentMessageLen);
    currentMessageLen = 0;
}

/*
 * Protocol is <message length as string> <space> <message XML>
 */
void CGroupClient::sendData(const QByteArray &data)
{
    QByteArray buff;
    QString len = QString("%1 ").arg(data.size());
    buff = len.toLatin1();
    buff += data;
    if (DEBUG)
        qDebug() << "Sending message:" << buff;
    socket.write(buff);
}
