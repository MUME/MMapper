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
    virtual ~ConnectionListenerTcpServer() override;

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
    explicit ConnectionListener(MapData *,
                                Mmapper2PathMachine *,
                                PrespammedPath *,
                                Mmapper2Group *,
                                MumeClock *,
                                MapCanvas *,
                                QObject *parent);
    virtual ~ConnectionListener() override;

private:
    Q_OBJECT

public:
    void listen();

signals:
    void log(const QString &, const QString &);
    void clientSuccessfullyConnected();

protected slots:
    void onIncomingConnection(qintptr socketDescriptor);

private:
    MapData *m_mapData = nullptr;
    Mmapper2PathMachine *m_pathMachine = nullptr;
    PrespammedPath *m_prespammedPath = nullptr;
    Mmapper2Group *m_groupManager = nullptr;
    MumeClock *m_mumeClock = nullptr;
    MapCanvas *m_mapCanvas = nullptr;
    using ServerList = std::vector<QPointer<ConnectionListenerTcpServer>>;
    ServerList m_servers;
    std::unique_ptr<Proxy> m_proxy;
    std::unique_ptr<QThread> m_thread;

    bool m_accept = true;
};
