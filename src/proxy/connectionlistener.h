#pragma once
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

#ifndef CONNECTIONLISTENER
#define CONNECTIONLISTENER

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

#endif
