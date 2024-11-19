// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mapfrontend.h"

#include "../global/Timer.h"
#include "../global/logging.h"
#include "../global/progresscounter.h"
#include "../map/ChangeTypes.h"
#include "../map/RoomRecipient.h"
#include "../map/coordinate.h"
#include "../map/parseevent.h"
#include "../map/room.h"
#include "../map/roomid.h"

#include <cassert>
#include <memory>
#include <set>
#include <tuple>
#include <utility>

MapFrontend::MapFrontend(QObject *const parent)
    : QObject(parent)
{}

MapFrontend::~MapFrontend()
{
    emit sig_clearingMap();
}

void MapFrontend::block()
{
    blockSignals(true);
}

void MapFrontend::unblock()
{
    blockSignals(false);
}

void MapFrontend::checkSize()
{
    const auto bounds = getCurrentMap().getBounds().value_or(Bounds{});
    emit sig_mapSizeChanged(bounds.min, bounds.max);
}

void MapFrontend::scheduleAction(const Change &change)
{
    std::ignore = applySingleChange(change);
}

void MapFrontend::revert()
{
    emit sig_clearingMap();
    setCurrentMarks(m_saved.marks);
    setCurrentMap(m_saved.map);
}

void MapFrontend::clear()
{
    emit sig_clearingMap();
    setCurrentMap(Map{});
    setCurrentMarks(InfomarkDb{});
    currentHasBeenSaved();
    checkSize(); // called for side effect of sending signal
    virt_clear();
}

bool MapFrontend::createEmptyRoom(const Coordinate &c)
{
    const auto &map = getCurrentMap();
    if (map.findRoomHandle(c)) {
        qWarning() << "A room already exists at the chosen position.";
        return false;
    }

    return applySingleChange(Change{room_change_types::AddPermanentRoom{c}});
}

void MapFrontend::slot_createRoom(const SigParseEvent &sigParseEvent,
                                  const Coordinate &expectedPosition)
{
    const ParseEvent &event = sigParseEvent.deref();

    MMLOG() << "[mapfrontend] Adding new room from parseEvent";

    const bool success = applySingleChange(
        Change{room_change_types::AddRoom2{expectedPosition, event}});

    if (success) {
        MMLOG() << "[mapfrontend] Added new room.";
    } else {
        MMLOG() << "[mapfrontend] Failed to add new room.";
    }
}

RoomPtr MapFrontend::findRoomHandle(RoomId id) const
{
    return getCurrentMap().findRoomHandle(id);
}

RoomPtr MapFrontend::findRoomHandle(const Coordinate &coord) const
{
    return getCurrentMap().findRoomHandle(coord);
}

RoomPtr MapFrontend::findRoomHandle(const ExternalRoomId id) const
{
    return getCurrentMap().findRoomHandle(id);
}

RoomPtr MapFrontend::findRoomHandle(const ServerRoomId id) const
{
    return getCurrentMap().findRoomHandle(id);
}

RoomHandle MapFrontend::getRoomHandle(const RoomId id) const
{
    return getCurrentMap().getRoomHandle(id);
}

const RawRoom &MapFrontend::getRawRoom(const RoomId id) const
{
    return getCurrentMap().getRawRoom(id);
}

RoomIdSet MapFrontend::findAllRooms(const Coordinate &coord) const
{
    if (auto pRoom = findRoomHandle(coord)) {
        return RoomIdSet{deref(pRoom).getId()};
    }
    return RoomIdSet{};
}

RoomIdSet MapFrontend::findAllRooms(const SigParseEvent &event) const
{
    if (!event.isValid()) {
        return RoomIdSet{};
    }

    return getCurrentMap().findAllRooms(event.deref());
}

RoomIdSet MapFrontend::findAllRooms(const Coordinate &input_min, const Coordinate &input_max) const
{
    Bounds bounds{input_min, input_max};
    RoomIdSet result;
    const auto &map = getCurrentMap();
    for (const RoomId id : map.getRooms()) {
        const auto &r = map.getRoomHandle(id);
        if (bounds.contains(r.getPosition())) {
            result.insert(r.getId());
        }
    }
    return result;
}

void MapFrontend::lookingForRooms(RoomRecipient &recipient, const SigParseEvent &sigParseEvent)
{
    const ParseEvent &event = sigParseEvent.deref();
    if (getCurrentMap().empty()) {
        if ((false)) {
            // what was going on in this case?
            Coordinate c(0, 0, 0);
            slot_createRoom(sigParseEvent, c);
            std::abort();
        }
        return;
    }

    getCurrentMap().getRooms(recipient, event);
}

void MapFrontend::lookingForRooms(RoomRecipient &recipient, const RoomId id)
{
    if (const auto &pRoom = findRoomHandle(id)) {
        recipient.receiveRoom(*pRoom);
    }
}

void MapFrontend::lookingForRooms(RoomRecipient &recipient, const Coordinate &pos)
{
    if (const auto &pRoom = findRoomHandle(pos)) {
        recipient.receiveRoom(*pRoom);
    }
}

void MapFrontend::setSavedMap(Map map)
{
    m_saved.map = map;
}

void MapFrontend::setSavedMarks(InfomarkDb marks)
{
    m_saved.marks = marks;
}

// REVISIT: we probably don't want to take the caller's word for it about what changed?
void MapFrontend::setCurrentMarks(InfomarkDb marks, const InfoMarkUpdateFlags modified)
{
    m_current.marks = marks;
    this->InfoMarkModificationTracker::notifyModified(modified);
}

void MapFrontend::setCurrentMap(const MapApplyResult &result)
{
    // NOTE: This is very important: it's where the map is actually changed!
    m_current.map = result.map;
    const auto roomUpdateFlags = result.roomUpdateFlags;

    this->RoomModificationTracker::notifyModified(roomUpdateFlags);
    // TODO: move checkSize() into the notifyModified().
    if (roomUpdateFlags.contains(RoomUpdateEnum::BoundsChanged)) {
        checkSize();
    }
}

void MapFrontend::setCurrentMap(Map map)
{
    // Always update everything when the map is changed like this.
    setCurrentMap(MapApplyResult{map, ~RoomUpdateFlags{}});
}

bool MapFrontend::applySingleChange(ProgressCounter &pc, const Change &change)
{
    MapApplyResult result;
    try {
        result = m_current.map.applySingleChange(pc, change);
    } catch (const std::exception &e) {
        MMLOG_ERROR() << "Exception: " << e.what();
        return false;
    }

    setCurrentMap(result);
    return true;
}

bool MapFrontend::applySingleChange(const Change &change)
{
    ProgressCounter dummyPc;
    return applySingleChange(dummyPc, change);
}

bool MapFrontend::applyChanges(ProgressCounter &pc, const ChangeList &changes)
{
    MapApplyResult result;
    try {
        result = m_current.map.apply(pc, changes);
    } catch (const std::exception &e) {
        MMLOG_ERROR() << "Exception: " << e.what();
        return false;
    }

    setCurrentMap(result);
    return true;
}

bool MapFrontend::applyChanges(const ChangeList &changes)
{
    ProgressCounter dummyPc;
    return applyChanges(dummyPc, changes);
}

void MapFrontend::keepRoom(RoomRecipient &, const RoomId id)
{
    if (const auto &room = findRoomHandle(id); room.has_value() && room->isTemporary()) {
        applySingleChange(Change{room_change_types::MakePermanent{id}});
    }
}

void MapFrontend::releaseRoom(RoomRecipient &, const RoomId id)
{
    if (const auto &room = findRoomHandle(id); room.has_value() && room->isTemporary()) {
        applySingleChange(Change{room_change_types::RemoveRoom{id}});
    }
}
