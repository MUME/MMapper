#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../observer/gameobserver.h"

#include <memory>
#include <vector>

#include <QHostAddress>
#include <QPointer>
#include <QString>
#include <QTcpServer>
#include <QtCore>
#include <QtGlobal>

class ConnectionListener;
class MapCanvas;
class MapData;
class Mmapper2Group;
class Mmapper2PathMachine;
class MumeClock;
class PrespammedPath;
class Proxy;
class QObject;
class RoomManager;

class ConnectionListenerTcpServer final : public QTcpServer
{
    Q_OBJECT

public:
    explicit ConnectionListenerTcpServer(ConnectionListener *parent);
    ~ConnectionListenerTcpServer() final;

protected:
    void incomingConnection(qintptr socketDescriptor) override;

signals:
    void signal_incomingConnection(qintptr socketDescriptor);
};

class ConnectionListener final : public QObject
{
    Q_OBJECT

private:
    MapData &m_mapData;
    Mmapper2PathMachine &m_pathMachine;
    PrespammedPath &m_prespammedPath;
    Mmapper2Group &m_groupManager;
    MumeClock &m_mumeClock;
    MapCanvas &m_mapCanvas;
    GameObserver &m_gameOberver;
    using ServerList = std::vector<QPointer<ConnectionListenerTcpServer>>;
    ServerList m_servers;
    QPointer<Proxy> m_proxy;

    bool m_accept = true;

public:
    explicit ConnectionListener(MapData &,
                                Mmapper2PathMachine &,
                                PrespammedPath &,
                                Mmapper2Group &,
                                MumeClock &,
                                MapCanvas &,
                                GameObserver &,
                                QObject *parent);
    ~ConnectionListener() final;

public:
    void listen();

private:
    void log(const QString &msg) { emit sig_log("Listener", msg); }

signals:
    void sig_log(const QString &, const QString &);
    void sig_clientSuccessfullyConnected();

protected slots:
    void slot_onIncomingConnection(qintptr socketDescriptor);
};
