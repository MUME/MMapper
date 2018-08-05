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

#include "mapfrontend.h"

#include <cassert>
#include <memory>
#include <utility>
#include <QMutex>

#include "../expandoracommon/abstractroomfactory.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/frustum.h"
#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/room.h"
#include "../expandoracommon/roomrecipient.h"
#include "../global/roomid.h"
#include "ParseTree.h"
#include "map.h"
#include "mapaction.h"
#include "roomcollection.h"
#include "roomlocker.h"

MapFrontend::MapFrontend(AbstractRoomFactory *const in_factory)
    : mapLock(QMutex::Recursive)
    , factory(in_factory)
{}

MapFrontend::~MapFrontend()
{
    QMutexLocker locker(&mapLock);
    emit clearingMap();

    for (uint i = 0; i <= greatestUsedId.asUint32(); ++i) {
        /* NOTE: you're allowed to delete nullptr, so no conditional is required */
        delete std::exchange(roomIndex[RoomId{i}], nullptr);
    }
}

void MapFrontend::block()
{
    mapLock.lock();
    blockSignals(true);
}

void MapFrontend::unblock()
{
    mapLock.unlock();
    blockSignals(false);
}

void MapFrontend::checkSize()
{
    emit mapSizeChanged(ulf, lrb);
}

void MapFrontend::scheduleAction(MapAction *const action)
{
    QMutexLocker locker(&mapLock);
    action->schedule(this);

    bool executable = true;
    for (auto roomId : action->getAffectedRooms()) {
        actionSchedule[roomId].insert(action);
        if (!locks[roomId].empty()) {
            executable = false;
        }
    }
    if (executable) {
        executeAction(action);
        removeAction(action);
    }
}

void MapFrontend::executeAction(MapAction *const action)
{
    action->exec();
}

void MapFrontend::removeAction(MapAction *const action)
{
    for (auto roomId : action->getAffectedRooms()) {
        actionSchedule[roomId].erase(action);
    }
    delete action;
}

bool MapFrontend::isExecutable(MapAction *const action)
{
    for (auto roomId : action->getAffectedRooms()) {
        if (!locks[roomId].empty()) {
            return false;
        }
    }
    return true;
}

void MapFrontend::executeActions(const RoomId roomId)
{
    std::set<MapAction *> executedActions;
    for (auto action : actionSchedule[roomId]) {
        if (isExecutable(action)) {
            executeAction(action);
            executedActions.insert(action);
        }
    }
    for (auto action : executedActions) {
        removeAction(action);
    }
}

void MapFrontend::lookingForRooms(RoomRecipient &recipient, const Frustum &frustum)
{
    QMutexLocker locker(&mapLock);
    for (auto &room : roomIndex) {
        if (room != nullptr) {
            const Coordinate rc = room->getPosition();
            if (frustum.pointInFrustum(rc)) {
                locks[room->getId()].insert(&recipient);
                recipient.receiveRoom(this, room);
            }
        }
    }
}

void MapFrontend::lookingForRooms(RoomRecipient &recipient, const Coordinate &pos)
{
    QMutexLocker locker(&mapLock);
    if (Room *const r = map.get(pos)) {
        locks[r->getId()].insert(&recipient);
        recipient.receiveRoom(this, r);
    }
}

void MapFrontend::clear()
{
    QMutexLocker locker(&mapLock);
    emit clearingMap();

    for (uint i = 0; i < roomIndex.size(); ++i) {
        const auto roomId = RoomId{i};
        if (roomIndex[roomId] != nullptr) {
            delete std::exchange(roomIndex[roomId], nullptr);
            if (auto h = std::exchange(roomHomes[roomId], nullptr)) {
                h->clear();
            }
            locks[roomId].clear();
        }
    }

    map.clear();

    while (!unusedIds.empty()) {
        unusedIds.pop();
    }
    greatestUsedId = INVALID_ROOMID;
    ulf.clear();
    lrb.clear();
    emit mapSizeChanged(ulf, lrb);
}

void MapFrontend::lookingForRooms(RoomRecipient &recipient, const RoomId id)
{
    QMutexLocker locker(&mapLock);
    if (greatestUsedId >= id) {
        if (Room *r = roomIndex[id]) {
            locks[id].insert(&recipient);
            recipient.receiveRoom(this, r);
        }
    }
}

RoomId MapFrontend::assignId(Room *const room, const SharedRoomCollection &roomHome)
{
    /* REVISIT: move all of the objects modified in this function to a sub-object? */

    const RoomId id = [this]() {
        if (unusedIds.empty()) {
            /* avoid adding operator++ to RoomId, to help avoid mistakes */
            return greatestUsedId = RoomId{greatestUsedId.asUint32() + 1u};
        } else {
            auto id = unusedIds.top();
            unusedIds.pop();
            if (id > greatestUsedId || greatestUsedId == INVALID_ROOMID) {
                greatestUsedId = id;
            }
            return id;
        }
    }();

    room->setId(id);

    if (roomIndex.size() <= id.asUint32()) {
        const auto bigger = id.asUint32() * 2u + 1u;
        roomIndex.resize(bigger, nullptr);
        locks.resize(bigger);
        roomHomes.resize(bigger, nullptr);
    }
    roomIndex[id] = room;
    roomHomes[id] = roomHome;
    return id;
}

void MapFrontend::lookingForRooms(RoomRecipient &recipient,
                                  const Coordinate &ulf,
                                  const Coordinate &lrb)
{
    QMutexLocker locker(&mapLock);
    RoomLocker ret(recipient, *this);
    map.getRooms(ret, ulf, lrb);
}

void MapFrontend::insertPredefinedRoom(Room &room)
{
    QMutexLocker locker(&mapLock);
    assert(signalsBlocked());
    const auto id = room.getId();
    const Coordinate &c = room.getPosition();
    auto event = factory->getEvent(&room);

    assert(roomIndex.size() <= id.asUint32() || roomIndex[id] == nullptr);

    auto roomHome = parseTree.insertRoom(*event);
    map.setNearest(c, room);
    checkSize(room.getPosition());
    unusedIds.push(id);
    assignId(&room, roomHome);
    if (roomHome != nullptr) {
        roomHome->addRoom(&room);
    }
}

RoomId MapFrontend::createEmptyRoom(const Coordinate &c)
{
    QMutexLocker locker(&mapLock);
    Room *const room = factory->createRoom();
    room->setPermanent();
    map.setNearest(c, *room);
    checkSize(room->getPosition());
    const RoomId id = assignId(room, nullptr);
    return id;
}

void MapFrontend::checkSize(const Coordinate &c)
{
    // lrb = lower right back, and ulf = upper left front.
    Coordinate lrbBackup(lrb);
    Coordinate ulfBackup(ulf);

    if (c.x < ulf.x) {
        ulf.x = c.x; // left(-X) == west
    } else if (c.x > lrb.x) {
        lrb.x = c.x; // right(+X) == east
    }
    if (c.y < ulf.y) {
        ulf.y = c.y; // upper(-Y) == north
    } else if (c.y > lrb.y) {
        lrb.y = c.y; // lower(+Y) == south
    }
    if (c.z < ulf.z) {
        ulf.z = c.z; // front(-Z) == up
    } else if (c.z > lrb.z) {
        lrb.z = c.z; // back(+Z) == down
    }

    if (ulf != ulfBackup || lrb != lrbBackup) {
        emit mapSizeChanged(ulf, lrb);
    }
}

void MapFrontend::createEmptyRooms(const Coordinate &ulf, const Coordinate &lrb)
{
    map.fillArea(factory, ulf, lrb);
}

void MapFrontend::createRoom(const SigParseEvent &sigParseEvent, const Coordinate &expectedPosition)
{
    ParseEvent &event = sigParseEvent.deref();

    QMutexLocker locker(&mapLock);
    checkSize(expectedPosition); // still hackish but somewhat better
    if (SharedRoomCollection roomHome = parseTree.insertRoom(event)) {
        Room *const room = factory->createRoom(event);
        roomHome->addRoom(room);
        map.setNearest(expectedPosition, *room);
        assignId(room, roomHome);
    }
    event.reset();
}

void MapFrontend::lookingForRooms(RoomRecipient &recipient, const SigParseEvent &sigParseEvent)
{
    ParseEvent &event = sigParseEvent.deref();
    QMutexLocker locker(&mapLock);
    if (greatestUsedId == INVALID_ROOMID) {
        Coordinate c(0, 0, 0);
        createRoom(sigParseEvent, c);
        if (greatestUsedId != INVALID_ROOMID) {
            roomIndex[DEFAULT_ROOMID]->setPermanent();
        }
    }

    RoomLocker ret(recipient, *this, factory, &event);
    parseTree.getRooms(ret, event);
    event.reset();
}

void MapFrontend::lockRoom(RoomRecipient *const recipient, const RoomId id)
{
    locks[id].insert(recipient);
}

// removes the lock on a room
// after the last lock is removed, the room is deleted
void MapFrontend::releaseRoom(RoomRecipient &sender, const RoomId id)
{
    QMutexLocker lock(&mapLock);
    auto &room_locks_ref = locks[id];
    room_locks_ref.erase(&sender);
    if (room_locks_ref.empty()) {
        executeActions(id);
        if (Room *const room = roomIndex[id]) {
            if (room->isTemporary()) {
                MapAction *const r = new SingleRoomAction(new Remove, id);
                scheduleAction(r);
            }
        }
    }
}

// makes a lock on a room permanent and anonymous.
// Like that the room can't be deleted via releaseRoom anymore.
void MapFrontend::keepRoom(RoomRecipient &sender, const RoomId id)
{
    QMutexLocker lock(&mapLock);
    auto &lock_ref = locks[id];
    lock_ref.erase(&sender);
    MapAction *const mp = new SingleRoomAction(new MakePermanent, id);
    scheduleAction(mp);
    if (lock_ref.empty()) {
        executeActions(id);
    }
}
