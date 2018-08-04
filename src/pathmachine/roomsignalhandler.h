#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef ROOMSIGNALHANDLER
#define ROOMSIGNALHANDLER

#include <map>
#include <set>
#include <QObject>
#include <QString>
#include <QtCore>

#include "../global/roomid.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/mmapper2exit.h"

class MapAction;
class Room;
class RoomAdmin;
class RoomRecipient;
struct RoomId;

class RoomSignalHandler : public QObject
{
    Q_OBJECT
private:
    std::map<const Room *, RoomAdmin *> owners{};
    std::map<const Room *, std::set<RoomRecipient *>> lockers{};
    std::map<const Room *, int> holdCount{};

public:
    RoomSignalHandler() = delete;
    RoomSignalHandler(QObject *parent)
        : QObject(parent)
    {}
    /* receiving from our clients: */
    // hold the room, we don't know yet what to do, overrides release, re-caches if room was un-cached
    void hold(const Room *room, RoomAdmin *owner, RoomRecipient *locker);
    // room isn't needed anymore and can be deleted if no one else is holding it and no one else uncached it
    void release(const Room *room);
    // keep the room but un-cache it - overrides both hold and release
    // toId is negative if no exit should be added, else it's the id of
    // the room where the exit should lead
    void keep(const Room *room, ExitDirection dir, RoomId fromId);

    /* Sending to the rooms' owners:
       keepRoom: keep the room, but we don't need it anymore for now
       releaseRoom: delete the room, if you like */

    int getNumLockers(const Room *room) { return lockers[room].size(); }

signals:
    void scheduleAction(MapAction *);
};

#endif
