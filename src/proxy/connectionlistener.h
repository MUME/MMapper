#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <memory>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtCore>
#include <QtGlobal>

class MapCanvas;
class MapData;
class Mmapper2Group;
class Mmapper2PathMachine;
class MumeClock;
class PrespammedPath;
class Proxy;
class QObject;

class ConnectionListener : public QTcpServer
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

signals:
    void log(const QString &, const QString &);
    void clientSuccessfullyConnected();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    Q_OBJECT

private:
    MapData *m_mapData = nullptr;
    Mmapper2PathMachine *m_pathMachine = nullptr;
    PrespammedPath *m_prespammedPath = nullptr;
    Mmapper2Group *m_groupManager = nullptr;
    MumeClock *m_mumeClock = nullptr;
    MapCanvas *m_mapCanvas = nullptr;
    std::unique_ptr<Proxy> m_proxy;
    std::unique_ptr<QThread> m_thread;

    bool m_accept = true;
};
