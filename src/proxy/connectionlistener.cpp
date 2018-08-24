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

#include "connectionlistener.h"

#include <QTcpSocket>

#include "../configuration/configuration.h"
#include "proxy.h"

ConnectionListener::ConnectionListener(MapData *md,
                                       Mmapper2PathMachine *pm,
                                       CommandEvaluator *ce,
                                       PrespammedPath *pp,
                                       Mmapper2Group *gm,
                                       MumeClock *mc,
                                       QObject *parent)
    : QTcpServer(parent)
{
    m_mapData = md;
    m_pathMachine = pm;
    m_commandEvaluator = ce;
    m_prespammedPath = pp;
    m_groupManager = gm;
    m_mumeClock = mc;

    connect(this,
            SIGNAL(log(const QString &, const QString &)),
            parent,
            SLOT(log(const QString &, const QString &)));
}

void ConnectionListener::incomingConnection(qintptr socketDescriptor)
{
    if (m_accept) {
        emit log("Listener", "New connection: accepted.");
        doNotAcceptNewConnections();
        auto *proxy = new Proxy(m_mapData,
                                m_pathMachine,
                                m_commandEvaluator,
                                m_prespammedPath,
                                m_groupManager,
                                m_mumeClock,
                                socketDescriptor,
                                getConfig().connection.proxyThreaded,
                                this);
        proxy->start();
    } else {
        emit log("Listener", "New connection: rejected.");
        QTcpSocket tcpSocket;
        if (tcpSocket.setSocketDescriptor(socketDescriptor)) {
            QByteArray ba("\033[1;37;41mYou can't connect to MMapper more than once!\r\n"
                          "Please close the existing connection.\033[0m\r\n");
            tcpSocket.write(ba);
            tcpSocket.flush();
            tcpSocket.disconnectFromHost();
            tcpSocket.waitForDisconnected();
        }
    }
}

void ConnectionListener::doNotAcceptNewConnections()
{
    emit clientSuccessfullyConnected();
    m_accept = false;
}

void ConnectionListener::doAcceptNewConnections()
{
    m_accept = true;
}
