// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "connectionlistener.h"

#include "../configuration/configuration.h"
#include "../global/AnsiOstream.h"
#include "../global/Charset.h"
#include "../global/MakeQPointer.h"
#include "../global/TextUtils.h"
#include "TcpSocket.h"
#include "proxy.h"

#include <memory>
#include <sstream>

#include <QTcpSocket>
#include <QThread>

ConnectionListenerTcpServer::ConnectionListenerTcpServer(ConnectionListener *const parent)
    : QTcpServer(parent)
{}

ConnectionListenerTcpServer::~ConnectionListenerTcpServer() = default;

void ConnectionListenerTcpServer::incomingConnection(qintptr socketDescriptor)
{
    emit signal_incomingConnection(socketDescriptor);
}

ConnectionListener::ConnectionListener(MapData &md,
                                       Mmapper2PathMachine &pm,
                                       PrespammedPath &pp,
                                       Mmapper2Group &gm,
                                       MumeClock &mc,
                                       CTimers &ct,
                                       MapCanvas &mca,
                                       GameObserver &go,
                                       QObject *const parent)
    : QObject(parent)
    , m_mapData{md}
    , m_pathMachine{pm}
    , m_prespammedPath{pp}
    , m_groupManager{gm}
    , m_mumeClock{mc}
    , m_timers{ct}
    , m_mapCanvas{mca}
    , m_gameOberver{go}
{}

ConnectionListener::~ConnectionListener()
{
    // Note: We're the owner of the proxy, so we're allowed to delete it.
    // Deleting the proxy here somehow prevents the proxy's destructor from triggering
    // use-after-free undefined behavior when the proxy tries to send a goodbye message
    // to the user.
    //
    // Another option would be to make m_proxy an owning pointer (e.g. unique_ptr or QScopedPointer),
    // and then make sure it's declared last so it'll be deleted first, and then make this dtor
    // a default dtor again.
    if (Proxy *const pProxy = m_proxy) {
        MMLOG() << "ConnectionListener is deleting the proxy...";
        delete pProxy;
    }
}

void ConnectionListener::listen()
{
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        return;
    }
    const auto &settings = getConfig().connection;
    const auto hostAdress = settings.proxyListensOnAnyInterface ? QHostAddress::Any
                                                                : QHostAddress::LocalHost;
    const auto port = settings.localPort;

    const auto createServer = [this]() {
        auto server = mmqt::makeQPointer<ConnectionListenerTcpServer>(this);
        server->setMaxPendingConnections(1);
        connect(server, &ConnectionListenerTcpServer::acceptError, this, [this, server]() {
            log(QString("Encountered an error: %1").arg(server->errorString()));
        });
        connect(server,
                &ConnectionListenerTcpServer::signal_incomingConnection,
                this,
                &ConnectionListener::slot_onIncomingConnection);
        return server;
    };

    // Construct the first server
    m_servers.emplace_back(createServer());
    auto &server = m_servers.back();
    if (!server->listen(hostAdress, port)) {
        throw std::runtime_error(mmqt::toStdStringUtf8(server->errorString()));
    }

    // Construct a second server for localhost IPv6 if we're only listening on localhost IPv4
    if (!settings.proxyListensOnAnyInterface) {
        m_servers.emplace_back(createServer());
        auto &serverIPv6 = m_servers.back();
        if (!serverIPv6->listen(QHostAddress::LocalHostIPv6, port)) {
            throw std::runtime_error(mmqt::toStdStringUtf8(serverIPv6->errorString()));
        }
    }
}

void ConnectionListener::slot_onIncomingConnection(qintptr socketDescriptor)
{
    auto socket = std::make_unique<TcpSocket>(socketDescriptor, this);
    emit sig_clientSuccessfullyConnected();
    startClient(std::move(socket));
}

void ConnectionListener::startClient(std::unique_ptr<AbstractSocket> socket)
{
    if (m_proxy == nullptr) {
        log("New connection: accepted.");
        m_proxy = Proxy::allocInit(m_mapData,
                                   m_pathMachine,
                                   m_prespammedPath,
                                   m_groupManager,
                                   m_mumeClock,
                                   m_timers,
                                   m_mapCanvas,
                                   m_gameOberver,
                                   std::move(socket),
                                   *this);
    } else {
        log("New connection: rejected.");
        const auto msg = std::invoke([]() -> QByteArray {
            constexpr const auto whiteOnRed = getRawAnsi(AnsiColor16Enum::white,
                                                         AnsiColor16Enum::red);
            std::stringstream oss;
            AnsiOstream aos{oss};
            aos.writeWithColor(whiteOnRed, "You can't connect to MMapper more than once!\n");
            aos.write("\n");
            aos.writeWithColor(whiteOnRed, "Please close the existing connection.\n");
            return mmqt::toQByteArrayUtf8(oss.str());
        });

        socket->write(msg);
        socket->flush();
        socket->disconnectFromHost();
    }
}
