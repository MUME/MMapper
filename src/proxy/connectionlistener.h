#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <memory>
#include <vector>
#include <QHostAddress>
#include <QPointer>
#include <QString>
#include <QTcpServer>
#include <QtCore>
#include <QtGlobal>

class AutoLogger;
class ConnectionListener;
class MapCanvas;
class MapData;
class Mmapper2Group;
class Mmapper2PathMachine;
class MumeClock;
class PrespammedPath;
class Proxy;
class QObject;

class ConnectionListenerTcpServer final : public QTcpServer
{
public:
    explicit ConnectionListenerTcpServer(ConnectionListener *parent);
    ~ConnectionListenerTcpServer() final;

private:
    Q_OBJECT

protected:
    void incomingConnection(qintptr socketDescriptor) override;

signals:
    void signal_incomingConnection(qintptr socketDescriptor);
};

class ConnectionListener final : public QObject
{
public:
    explicit ConnectionListener(MapData &,
                                Mmapper2PathMachine &,
                                PrespammedPath &,
                                Mmapper2Group &,
                                MumeClock &,
                                AutoLogger &,
                                MapCanvas &,
                                QObject *parent);
    ~ConnectionListener() final;

private:
    Q_OBJECT

public:
    void listen();

private:
    void log(const QString &msg) { emit sig_log("Listener", msg); }

signals:
    void sig_log(const QString &, const QString &);
    void sig_clientSuccessfullyConnected();

protected slots:
    void slot_onIncomingConnection(qintptr socketDescriptor);

private:
    MapData &m_mapData;
    Mmapper2PathMachine &m_pathMachine;
    PrespammedPath &m_prespammedPath;
    Mmapper2Group &m_groupManager;
    MumeClock &m_mumeClock;
    AutoLogger &m_autoLogger;
    MapCanvas &m_mapCanvas;
    using ServerList = std::vector<QPointer<ConnectionListenerTcpServer>>;
    ServerList m_servers;
    std::unique_ptr<Proxy> m_proxy;
    std::unique_ptr<QThread> m_thread;

    bool m_accept = true;
};
