// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "connectionlistener.h"

#include <memory>
#include <QTcpSocket>
#include <QThread>

#include "../configuration/configuration.h"
#include "../global/TextUtils.h"
#include "proxy.h"

ConnectionListenerTcpServer::ConnectionListenerTcpServer(ConnectionListener *const parent)
    : QTcpServer(parent)
{}

ConnectionListenerTcpServer::~ConnectionListenerTcpServer() = default;

void ConnectionListenerTcpServer::incomingConnection(qintptr socketDescriptor)
{
    emit signal_incomingConnection(socketDescriptor);
}

ConnectionListener::ConnectionListener(MapData *const md,
                                       Mmapper2PathMachine *const pm,
                                       PrespammedPath *const pp,
                                       Mmapper2Group *const gm,
                                       MumeClock *const mc,
                                       MapCanvas *const mca,
                                       QObject *const parent)
    : QObject(parent)
{
    m_mapData = md;
    m_pathMachine = pm;
    m_prespammedPath = pp;
    m_groupManager = gm;
    m_mumeClock = mc;
    m_mapCanvas = mca;
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

void ConnectionListener::listen()
{
    const auto &settings = getConfig().connection;
    const auto hostAdress = settings.proxyListensOnAnyInterface ? QHostAddress::Any
                                                                : QHostAddress::LocalHost;
    const auto port = settings.localPort;

    const auto createServer = [this]() {
        QPointer<ConnectionListenerTcpServer> server(new ConnectionListenerTcpServer(this));
        server->setMaxPendingConnections(1);
        connect(server, &ConnectionListenerTcpServer::acceptError, this, [this, server]() {
            emit log("Listener", QString("Encountered an error: %1").arg(server->errorString()));
        });
        connect(server,
                &ConnectionListenerTcpServer::signal_incomingConnection,
                this,
                &ConnectionListener::onIncomingConnection);
        return server;
    };

    // Construct the first server
    m_servers.emplace_back(createServer());
    auto &server = m_servers.back();
    if (!server->listen(hostAdress, port))
        throw std::runtime_error(::toStdStringLatin1(server->errorString()));

    // Construct a second server for localhost IPv6 if we're only listening on localhost IPv4
    if (!settings.proxyListensOnAnyInterface) {
        m_servers.emplace_back(createServer());
        auto &serverIPv6 = m_servers.back();
        if (!serverIPv6->listen(QHostAddress::LocalHostIPv6, port))
            throw std::runtime_error(::toStdStringLatin1(serverIPv6->errorString()));
    }
}

void ConnectionListener::onIncomingConnection(qintptr socketDescriptor)
{
    if (m_accept) {
        emit log("Listener", "New connection: accepted.");
        m_accept = false;
        emit clientSuccessfullyConnected();

        m_proxy = std::make_unique<Proxy>(m_mapData,
                                          m_pathMachine,
                                          m_prespammedPath,
                                          m_groupManager,
                                          m_mumeClock,
                                          m_mapCanvas,
                                          socketDescriptor,
                                          this);

        if (getConfig().connection.proxyThreaded) {
            m_thread = std::make_unique<QThread>();
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
            QByteArray ba("\033[1;37;41mYou can't connect to MMapper more than once!\033[0m\r\n"
                          "\r\n"
                          "\033[1;37;41mPlease close the existing connection.\033[0m\r\n");
            tcpSocket.write(ba);
            tcpSocket.flush();
            tcpSocket.disconnectFromHost();
            tcpSocket.waitForDisconnected();
        }
    }
}
