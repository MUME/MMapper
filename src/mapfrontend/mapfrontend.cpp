// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mapfrontend.h"

#include <cassert>
#include <memory>
#include <set>
#include <utility>
#include <QMutex>

#include "../expandoracommon/RoomRecipient.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "ParseTree.h"
#include "map.h"
#include "mapaction.h"
#include "roomcollection.h"
#include "roomlocker.h"

MapFrontend::MapFrontend(QObject *const parent)
    : QObject(parent)
{}

MapFrontend::~MapFrontend()
{
    QMutexLocker locker(&mapLock);
    emit sig_clearingMap();
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
    emit sig_mapSizeChanged(getMin(), getMax());
}

void MapFrontend::scheduleAction(const std::shared_ptr<MapAction> &action)
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
        executeAction(action.get());
        removeAction(action);
    }
}

void MapFrontend::executeAction(MapAction *const action)
{
    action->exec();
}

void MapFrontend::removeAction(const std::shared_ptr<MapAction> &action)
{
    for (auto roomId : action->getAffectedRooms()) {
        actionSchedule[roomId].erase(action);
    }
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
    std::set<std::shared_ptr<MapAction>> executedActions;
    for (const auto &action : actionSchedule[roomId]) {
        if (isExecutable(action.get())) {
            executeAction(action.get());
            executedActions.insert(action);
        }
    }
    for (const auto &action : executedActions) {
        removeAction(action);
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
    emit sig_clearingMap();

    for (size_t i = 0, size = roomIndex.size(); i < size; ++i) {
        const auto roomId = RoomId{static_cast<uint32_t>(i)};

        auto &ref = roomIndex[roomId];
        if (ref == nullptr)
            continue;

        std::exchange(ref, nullptr)->setAboutToDie();

        if (SharedRoomCollection h = std::exchange(roomHomes[roomId], nullptr)) {
            h->clear();
        }
        locks[roomId].clear();
    }

    map.clear();

    while (!unusedIds.empty()) {
        unusedIds.pop();
    }
    greatestUsedId = INVALID_ROOMID;
    m_bounds.reset();
    checkSize(); // called for side effect of sending signal
}

void MapFrontend::lookingForRooms(RoomRecipient &recipient, const RoomId id)
{
    QMutexLocker locker(&mapLock);
    if (greatestUsedId >= id) {
        if (const SharedRoom &r = roomIndex[id]) {
            locks[id].insert(&recipient);
            recipient.receiveRoom(this, r.get());
        }
    }
}

RoomId MapFrontend::assignId(const SharedRoom &room, const SharedRoomCollection &roomHome)
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
                                  const Coordinate &input_min,
                                  const Coordinate &input_max)
{
    QMutexLocker locker(&mapLock);
    RoomLocker ret(recipient, *this);
    map.getRooms(ret, input_min, input_max);
}

void MapFrontend::insertPredefinedRoom(const SharedRoom &sharedRoom)
{
    Room &room = deref(sharedRoom);

    QMutexLocker locker(&mapLock);
    assert(signalsBlocked());
    const auto id = room.getId();
    const Coordinate &c = room.getPosition();
    auto event = Room::getEvent(&room);

    assert(roomIndex.size() <= id.asUint32() || roomIndex[id] == nullptr);

    auto roomHome = parseTree.insertRoom(*event);
    map.setNearest(c, room);
    checkSize(room.getPosition());
    unusedIds.push(id);
    MAYBE_UNUSED const auto ignored = assignId(sharedRoom, roomHome);
    if (roomHome != nullptr) {
        roomHome->addRoom(sharedRoom);
    }
}

RoomId MapFrontend::createEmptyRoom(const Coordinate &c)
{
    QMutexLocker locker(&mapLock);
    SharedRoom room = Room::createPermanentRoom(*this);
    map.setNearest(c, *room);
    checkSize(room->getPosition());
    return assignId(room, nullptr);
}

void MapFrontend::checkSize(const Coordinate &c)
{
    if (!m_bounds) {
        m_bounds.emplace();
        m_bounds->min = m_bounds->max = c;
        emit sig_mapSizeChanged(c, c);
        return;
    }

#define MINMAX(xyz) \
    do { \
        auto &lo = min.xyz; \
        auto &hi = max.xyz; \
        lo = std::min(lo, c.xyz); \
        hi = std::max(hi, c.xyz); \
    } while (false)

    auto &min = m_bounds->min;
    auto &max = m_bounds->max;

    const Coordinate oldMin{min};
    const Coordinate oldMax{max};

    MINMAX(x);
    MINMAX(y);
    MINMAX(z);

    if (min != oldMin || max != oldMax) {
        emit sig_mapSizeChanged(min, max);
    }

#undef MINMAX
}

void MapFrontend::createRoom(const SigParseEvent &sigParseEvent, const Coordinate &expectedPosition)
{
    const ParseEvent &event = sigParseEvent.deref();

    QMutexLocker locker(&mapLock);
    checkSize(expectedPosition); // still hackish but somewhat better
    if (SharedRoomCollection roomHome = parseTree.insertRoom(event)) {
        SharedRoom room = Room::createTemporaryRoom(*this, event);
        roomHome->addRoom(room);
        map.setNearest(expectedPosition, *room);
        MAYBE_UNUSED const auto ignored = assignId(room, roomHome);
    }
}

void MapFrontend::lookingForRooms(RoomRecipient &recipient, const SigParseEvent &sigParseEvent)
{
    const ParseEvent &event = sigParseEvent.deref();
    QMutexLocker locker(&mapLock);
    if (greatestUsedId == INVALID_ROOMID) {
        Coordinate c(0, 0, 0);
        createRoom(sigParseEvent, c);
        if (greatestUsedId != INVALID_ROOMID) {
            roomIndex[DEFAULT_ROOMID]->setPermanent();
        }
    }

    RoomLocker ret(recipient, *this, &event);
    parseTree.getRooms(ret, event);
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
        if (const SharedRoom &room = roomIndex[id]) {
            // REVISIT: Why do temporary rooms exist?
            // Also, note: After the conversion to SharedRoom, it's no longer necessary
            // to explicitly delete rooms. Just release all references to them.
            if (room->isTemporary()) {
                scheduleAction(std::make_shared<SingleRoomAction>(std::make_unique<Remove>(), id));
            }
        }
    }
}

// REVISIT: This is sent too often. Hunt down and kill the unnecessary cases (probably most of them).
//
// makes a lock on a room permanent and anonymous.
// Like that the room can't be deleted via releaseRoom anymore.
void MapFrontend::keepRoom(RoomRecipient &sender, const RoomId id)
{
    QMutexLocker lock(&mapLock);
    auto &lock_ref = locks[id];
    lock_ref.erase(&sender);
    scheduleAction(std::make_shared<SingleRoomAction>(std::make_unique<MakePermanent>(), id));
    if (lock_ref.empty()) {
        executeActions(id);
    }
}
