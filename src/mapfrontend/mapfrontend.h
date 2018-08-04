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

#ifndef MAPFRONTEND_H
#define MAPFRONTEND_H

#include <map>
#include <set>
#include <stack>
#include <QMutex>
#include <QString>
#include <QtCore>

#include "../expandoracommon/component.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/roomadmin.h"
#include "../global/roomid.h"
#include "ParseTree.h"
#include "map.h"

class AbstractRoomFactory;
class Frustum;
class MapAction;
class ParseEvent;
class QObject;
class Room;
class RoomCollection;
class RoomRecipient;
class SigParseEvent;

/**
 * The MapFrontend organizes rooms and their relations to each other.
 */
class MapFrontend : public Component, public RoomAdmin
{
    Q_OBJECT
    friend class FrontendAccessor;

protected:
    ParseTree parseTree{};
    Map map{};
    RoomIndex roomIndex{};
    std::stack<RoomId> unusedIds{};
    std::map<RoomId, std::set<MapAction *>> actionSchedule{};
    RoomHomes roomHomes{};
    RoomLocks locks{};

    RoomId greatestUsedId = INVALID_ROOMID;
    QMutex mapLock{};
    Coordinate ulf{};
    Coordinate lrb{};
    AbstractRoomFactory *factory = nullptr;

    void executeActions(RoomId roomId);
    void executeAction(MapAction *action);
    bool isExecutable(MapAction *action);
    void removeAction(MapAction *action);

    virtual RoomId assignId(Room *room, const SharedRoomCollection &roomHome);
    virtual void checkSize(const Coordinate &);

public:
    explicit MapFrontend(AbstractRoomFactory *factory);
    virtual ~MapFrontend();
    virtual void clear();
    void block();
    void unblock();
    virtual void checkSize();

    // removes the lock on a room
    // after the last lock is removed, the room is deleted
    virtual void releaseRoom(RoomRecipient &, RoomId) override;

    // makes a lock on a room permanent and anonymous.
    // Like that the room can't be deleted via releaseRoom anymore.
    virtual void keepRoom(RoomRecipient &, RoomId) override;

    virtual void lockRoom(RoomRecipient *, RoomId);

    virtual RoomId createEmptyRoom(const Coordinate &);

    virtual void insertPredefinedRoom(Room &);

    virtual RoomId getMaxId() { return greatestUsedId; }

    virtual const Coordinate &getUlf() const { return ulf; }
    virtual const Coordinate &getLrb() const { return lrb; }
public slots:
    // looking for rooms leads to a bunch of foundRoom() signals
    virtual void lookingForRooms(RoomRecipient &, const SigParseEvent &);
    virtual void lookingForRooms(RoomRecipient &, RoomId); // by id
    virtual void lookingForRooms(RoomRecipient &, const Coordinate &);
    virtual void lookingForRooms(RoomRecipient &, const Frustum &);
    virtual void lookingForRooms(RoomRecipient &,
                                 const Coordinate &,
                                 const Coordinate &); // by bounding box

    // createRoom creates a room without a lock
    // it will get deleted if no one looks for it for a certain time
    virtual void createRoom(const SigParseEvent &, const Coordinate &);
    virtual void createEmptyRooms(const Coordinate &, const Coordinate &);

    virtual void scheduleAction(MapAction *action) override;

signals:

    // this signal is sent out if a room is deleted. So any clients still
    // working on this room can start some emergency action.
    void mapSizeChanged(const Coordinate &, const Coordinate &);
    void clearingMap();
};

#endif
