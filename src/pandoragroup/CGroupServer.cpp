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

#include <QHostAddress>
#include <QDebug>

#include "CGroupServer.h"

CGroupServer::CGroupServer(int localPort, QObject *parent) :
    QTcpServer(parent)
{
    connect(this, SIGNAL(sendLog(const QString &)), parent, SLOT(relayLog(const QString &)));
    connect(this, SIGNAL(serverStartupFailed()), parent, SLOT(serverStartupFailed()));
    connect(this, SIGNAL(connectionClosed(CGroupClient *)),
            parent, SLOT(connectionClosed(CGroupClient *)));

    if (listen(QHostAddress::Any, localPort) != true) {
        emit sendLog("Failed to start a group Manager server");
        emit serverStartupFailed();
    } else {
        emit sendLog(QString("Listening on port %1").arg(localPort));
    }
}

CGroupServer::~CGroupServer()
{
    closeAll();
}

void CGroupServer::incomingConnection(qintptr socketDescriptor)
{
//    qInfo() << "Adding incomming client to the connections list" << socketDescriptor;

    // connect the client straight to the Communicator, as he handles all the state changes
    // data transfers and similar.
    CGroupClient *client = new CGroupClient(this);
    connections.append(client);

    connect(client, SIGNAL(incomingData(CGroupClient *, QByteArray)),
            parent(), SLOT(incomingData(CGroupClient *, QByteArray)));
    connect(client, SIGNAL(connectionEstablished(CGroupClient *)),
            parent(), SLOT(connectionEstablished(CGroupClient *)));

    client->setSocket(socketDescriptor);
}

void CGroupServer::errorInConnection(CGroupClient *connection, const QString &errorMessage)
{
    emit connectionClosed(connection);
//    qInfo() << "Removing and deleting the connection completely" << connection->socketDescriptor();
    connections.removeAll(connection);
    connection->close();
    connection->deleteLater();
}

void CGroupServer::sendToAll(QByteArray message)
{
    sendToAllExceptOne(NULL, message);
}

void CGroupServer::sendToAllExceptOne(CGroupClient *conn, QByteArray message)
{
    for (int i = 0; i < connections.size(); i++) {
        if (connections[i] != conn) {
            connections[i]->sendData(message);
        }
    }
}

void CGroupServer::closeAll()
{
//    qInfo() << "Closing connections";
    for (int i = 0; i < connections.size(); i++) {
        if (connections[i]) {
            connections[i]->disconnectFromHost();
            connections[i]->deleteLater();
        }
    }
    connections.clear();
}

void CGroupServer::relayLog(const QString &str)
{
//    qInfo() << "Client logged" << str;
}
