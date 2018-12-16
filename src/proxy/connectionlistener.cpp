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
#include <QThread>

#include "../configuration/configuration.h"
#include "proxy.h"

ConnectionListener::ConnectionListener(MapData *md,
                                       Mmapper2PathMachine *pm,
                                       PrespammedPath *pp,
                                       Mmapper2Group *gm,
                                       MumeClock *mc,
                                       QObject *parent)
    : QTcpServer(parent)
{
    m_mapData = md;
    m_pathMachine = pm;
    m_prespammedPath = pp;
    m_groupManager = gm;
    m_mumeClock = mc;

    connect(this,
            SIGNAL(log(const QString &, const QString &)),
            parent,
            SLOT(log(const QString &, const QString &)));
}

ConnectionListener::~ConnectionListener()
{
    if (m_proxy) {
        m_proxy.release(); // thread will delete the proxy
    }
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
        m_thread.release(); // finished() and destroyed() signals will destruct the thread
    }
}

void ConnectionListener::incomingConnection(qintptr socketDescriptor)
{
    if (m_accept) {
        emit log("Listener", "New connection: accepted.");
        m_accept = false;
        emit clientSuccessfullyConnected();

        m_proxy.reset(new Proxy(m_mapData,
                                m_pathMachine,
                                m_prespammedPath,
                                m_groupManager,
                                m_mumeClock,
                                socketDescriptor,
                                this));

        if (getConfig().connection.proxyThreaded) {
            m_thread.reset(new QThread);
            m_proxy->moveToThread(m_thread.get());

            // Proxy destruction stops the thread which then destroys itself on completion
            connect(m_proxy.get(), &QObject::destroyed, m_thread.get(), &QThread::quit);
            connect(m_thread.get(), &QThread::finished, m_thread.get(), &QObject::deleteLater);
            connect(m_thread.get(), &QObject::destroyed, this, [this]() {
                m_accept = true;
                m_proxy.release();
                m_thread.release();
            });

            // Make sure if the thread is interrupted that we kill the proxy
            connect(m_thread.get(), &QThread::finished, m_proxy.get(), &QObject::deleteLater);

            // Start the proxy when the thread starts
            connect(m_thread.get(), &QThread::started, m_proxy.get(), &Proxy::start);
            m_thread->start();

        } else {
            connect(m_proxy.get(), &QObject::destroyed, this, [this]() {
                m_accept = true;
                m_proxy.release();
            });
            m_proxy->start();
        }

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
