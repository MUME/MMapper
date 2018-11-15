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
#include <QTcpSocket>
#include <QTimer>

#include "../configuration/configuration.h"
#include "../global/io.h"

static constexpr const bool DEBUG = false;
static constexpr const auto FIVE_SECOND_TIMEOUT = 5000;

CGroupClient::CGroupClient(QObject *parent)
    : QObject(parent)
    , timer(new QTimer(this))
{
    timer->setInterval(FIVE_SECOND_TIMEOUT);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &CGroupClient::onTimeout);

    connect(&socket, &QAbstractSocket::hostFound, this, [this]() { emit sendLog("Host found."); });
    connect(&socket, &QAbstractSocket::connected, this, [this]() {
        socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
        setProtocolState(ProtocolStates::AwaitingLogin);
        emit sendLog("Connection established.");
        emit connectionEstablished(this);
    });
    connect(&socket, &QAbstractSocket::disconnected, this, [this]() {
        timer->stop();
        emit sendLog("Connection closed.");
        emit connectionClosed(this);
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
    timer->start();
    const auto &groupConfig = getConfig().groupManager;
    const auto remoteHost = groupConfig.host;
    const auto remotePort = static_cast<quint16>(groupConfig.remotePort);
    emit sendLog(
        QString("Connecting to remote host %1:%2").arg(remoteHost.constData()).arg(remotePort));
    socket.connectToHost(remoteHost, remotePort);
}

void CGroupClient::disconnectFromHost()
{
    timer->stop();
    emit sendLog("Closing the socket. Quitting.");
    socket.disconnectFromHost();
}

void CGroupClient::setSocket(qintptr socketDescriptor)
{
    if (!socket.setSocketDescriptor(socketDescriptor)) {
        qWarning() << "Connection failed. Native socket not recognized.";
        onError(QAbstractSocket::SocketAccessError);
        return;
    }
    socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    setProtocolState(ProtocolStates::AwaitingLogin);
    emit connectionEstablished(this);
}

void CGroupClient::setProtocolState(const ProtocolStates val)
{
    if (DEBUG)
        qInfo() << "Protocol state:" << static_cast<int>(val);
    protocolState = val;
    switch (val) {
    case ProtocolStates::AwaitingLogin:
        // Restart timer to verify that info was sent
        timer->start();
        break;
    case ProtocolStates::AwaitingInfo:
        // Restart timer to verify that login occurred
        emit sendLog("Login accepted.");
        timer->start();
        break;
    case ProtocolStates::Logged:
        emit sendLog("Group information received.");
        timer->stop();
        break;
    default:
        abort();
    }
}

void CGroupClient::onError(QAbstractSocket::SocketError /*socketError*/)
{
    timer->stop();
    emit errorInConnection(this, socket.errorString());
}

void CGroupClient::onTimeout()
{
    switch (socket.state()) {
    case QAbstractSocket::ConnectedState:
        switch (protocolState) {
        case ProtocolStates::Unconnected:
        case ProtocolStates::AwaitingLogin:
        case ProtocolStates::AwaitingInfo:
            socket.disconnectFromHost();
            emit errorInConnection(this, "Login timed out");
            break;
        case ProtocolStates::Logged:
            // Race condition? Protocol was successfully logged
            break;
        }
        break;
    case QAbstractSocket::HostLookupState:
        socket.abort();
        emit errorInConnection(this, "Host not found");
        return;
    case QAbstractSocket::UnconnectedState:
    case QAbstractSocket::ConnectingState:
    case QAbstractSocket::BoundState:
    case QAbstractSocket::ListeningState:
    case QAbstractSocket::ClosingState:
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
