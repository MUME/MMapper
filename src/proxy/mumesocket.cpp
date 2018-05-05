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

#include "mumesocket.h"
#include "configuration/configuration.h"

#include <QTcpSocket>
#include <QWebSocket>
#include <QNetworkRequest>
#include <QDebug>

MumeSocket::MumeSocket(QObject *parent) : QObject(parent)
{

}

void MumeSocket::onConnect()
{
    emit connected();
}

void MumeSocket::onDisconnect()
{
    emit disconnected();
}

void MumeSocket::onError(QAbstractSocket::SocketError e)
{
    if (e != QAbstractSocket::RemoteHostClosedError) {
        emit socketError(e);
    }
}

MumeMudSocket::MumeMudSocket(QObject *parent) : MumeSocket(parent)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(onDisconnect()));
    connect(m_socket, SIGNAL(connected()), this, SLOT(onConnect()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this,
            SLOT(onError(QAbstractSocket::SocketError)));
}

MumeMudSocket::~MumeMudSocket()
{
    if (m_socket) {
        m_socket->disconnectFromHost();
        m_socket->deleteLater();
    }
    delete m_socket;
    m_socket = NULL;
}

void MumeMudSocket::connectToHost()
{
    m_socket->connectToHost(Config().m_remoteServerName, Config().m_remotePort, QIODevice::ReadWrite);
}

void MumeMudSocket::disconnectFromHost()
{
    m_socket->disconnectFromHost();
}

void MumeMudSocket::onConnect()
{
    m_socket->setSocketOption(QAbstractSocket::KeepAliveOption, true);
    MumeSocket::onConnect();
}

void MumeMudSocket::onReadyRead()
{
    int read;
    while (m_socket->bytesAvailable()) {
        read = m_socket->read(m_buffer, 8191);
        if (read != -1) {
            m_buffer[read] = 0;
            QByteArray ba = QByteArray::fromRawData(m_buffer, read);
            emit processMudStream(ba);
        }
    }
}

void MumeMudSocket::sendToMud(const QByteArray &ba)
{
    m_socket->write(ba.data(), ba.size());
    m_socket->flush();
}


MumeWebSocket::MumeWebSocket(QObject *parent) : MumeSocket(parent)
{
    m_socket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);
    connect(m_socket, SIGNAL(binaryMessageReceived(const QByteArray &)),
            this, SLOT(onBinaryMessageReceived(const QByteArray &)));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(onDisconnect()));
    connect(m_socket, SIGNAL(connected()), this, SLOT(onConnect()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this,
            SLOT(onError(QAbstractSocket::SocketError)));
}

MumeWebSocket::~MumeWebSocket()
{
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
    }
    delete m_socket;
    m_socket = NULL;
}

void MumeWebSocket::connectToHost()
{
    QUrl url;
    url.setScheme("wss");
    url.setHost(Config().m_remoteServerName);
    url.setPort(Config().m_remotePort);
    url.setPath("/mume/play/websocket");
    qDebug() << url;

    QNetworkRequest req(url);
    req.setRawHeader("Sec-WebSocket-Protocol", "binary");
    m_socket->open(req);
}

void MumeWebSocket::disconnectFromHost()
{
    m_socket->close();
}

void MumeWebSocket::onBinaryMessageReceived(const QByteArray &ba)
{
    emit processMudStream(ba);
}

void MumeWebSocket::sendToMud(const QByteArray &ba)
{
    m_socket->sendBinaryMessage(ba);
    m_socket->flush();
}

