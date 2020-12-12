// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "customaction.h"

#include <memory>
#include <set>
#include <stdexcept>
#include <utility>

#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/Flags.h"
#include "../global/enums.h"
#include "../mapfrontend/ParseTree.h"
#include "../mapfrontend/map.h"
#include "../mapfrontend/mapaction.h"
#include "../mapfrontend/roomcollection.h"
#include "DoorFlags.h"
#include "ExitDirection.h"
#include "ExitFlags.h"
#include "roomselection.h"

template<typename T>
static inline TaggedString<T> modifyField(const TaggedString<T> &prev,
                                          const TaggedString<T> &next,
                                          const FlagModifyModeEnum mode)
{
    switch (mode) {
    case FlagModifyModeEnum::SET:
        return next;
    case FlagModifyModeEnum::UNSET:
        // this doesn't really make sense either.
        return TaggedString<T>{};
    case FlagModifyModeEnum::TOGGLE:
        // the idea of toggling a string like a door name doesn't make sense,
        // so this implementation is as good as any.
        if (prev.empty())
            return next;
        return TaggedString<T>{};
    }
    std::abort();
}

template<typename Flags, typename Flag>
static inline Flags modifyField(const Flags flags, const Flag x, const FlagModifyModeEnum mode)
{
    static_assert(std::is_same_v<Flags, Flag>);
    switch (mode) {
    case FlagModifyModeEnum::SET:
        return flags | x;
    case FlagModifyModeEnum::UNSET:
        return flags & (~x);
    case FlagModifyModeEnum::TOGGLE:
        return flags ^ x;
    }
    std::abort();
}

GroupMapAction::GroupMapAction(std::unique_ptr<AbstractAction> action,
                               const SharedRoomSelection &selection)
    : executor(std::move(action))
{
    for (const auto &[rid, room] : *selection) {
        affectedRooms.insert(rid);
        selectedRooms.push_back(rid);
    }
}

void AddTwoWayExit::exec()
{
    if (room2Dir == ExitDirEnum::UNKNOWN) {
        room2Dir = opposite(dir);
    }
    AddOneWayExit::exec();
    std::swap(to, from);
    dir = room2Dir;
    AddOneWayExit::exec();
}

void RemoveTwoWayExit::exec()
{
    if (room2Dir == ExitDirEnum::UNKNOWN) {
        room2Dir = opposite(dir);
    }
    RemoveOneWayExit::exec();
    std::swap(to, from);
    dir = room2Dir;
    RemoveOneWayExit::exec();
}

const RoomIdSet &GroupMapAction::getAffectedRooms()
{
    for (auto &selectedRoom : selectedRooms) {
        executor->insertAffected(selectedRoom, affectedRooms);
    }
    return affectedRooms;
}

void GroupMapAction::exec()
{
    for (auto &selectedRoom : selectedRooms) {
        executor->preExec(selectedRoom);
    }
    for (auto &selectedRoom : selectedRooms) {
        executor->exec(selectedRoom);
    }
}

MoveRelative::MoveRelative(const Coordinate &in_move)
    : move(in_move)
{}

void MoveRelative::exec(const RoomId id)
{
    if (Room *room = roomIndex(id)) {
        const Coordinate &c = room->getPosition();
        Coordinate newPos = c + move;
        map().setNearest(newPos, *room);
    }
}

void MoveRelative::preExec(const RoomId id)
{
    if (Room *const room = roomIndex(id)) {
        map().remove(room->getPosition());
    }
}

MergeRelative::MergeRelative(const Coordinate &in_move)
    : move(in_move)
{}

void MergeRelative::insertAffected(const RoomId id, RoomIdSet &affected)
{
    if (Room *const room = roomIndex(id)) {
        ExitsAffecter::insertAffected(id, affected);
        const Coordinate &c = room->getPosition();
        const Coordinate newPos = c + move;
        Room *const other = map().get(newPos);
        if (other != nullptr) {
            affected.insert(other->getId());
        }
    }
}

void MergeRelative::preExec(const RoomId id)
{
    if (Room *const room = roomIndex(id)) {
        map().remove(room->getPosition());
    }
}

void MergeRelative::exec(const RoomId id)
{
    Room *const source = roomIndex(id);
    if (source == nullptr) {
        return;
    }
    Map &roomMap = map();

    const Coordinate &c = source->getPosition();
    Room *const target = roomMap.get(c + move);
    if (target != nullptr) {
        Room::update(target, source);
        auto oid = target->getId();
        const SharedRoomCollection newHome = [this, &target]() {
            auto props = Room::getEvent(target);
            return getParseTree().insertRoom(*props);
        }();

        RoomHomes &homes = roomHomes();

        auto &home_ref = homes[oid];
        if (SharedRoomCollection &oldHome = home_ref) {
            oldHome->removeRoom(target);
        }

        home_ref = newHome;
        if (newHome != nullptr) {
            newHome->addRoom(target);
        }
        const ExitsList &exits = source->getExitsList();
        for (const auto dir : enums::makeCountingIterator<ExitDirEnum>(exits)) {
            const Exit &e = exits[dir];
            for (const auto &oeid : e.inRange()) {
                if (Room *const oe = roomIndex(oeid)) {
                    oe->addOutExit(opposite(dir), oid);
                    target->addInExit(dir, oeid);
                }
            }
            for (const auto &oeid : e.outRange()) {
                if (Room *const oe = roomIndex(oeid)) {
                    oe->addInExit(opposite(dir), oid);
                    target->addOutExit(dir, oeid);
                }
            }
        }
        Remove::exec(source->getId());
    } else {
        const Coordinate newPos = c + move;
        roomMap.setNearest(newPos, *source);
    }
}

void ConnectToNeighbours::insertAffected(const RoomId id, RoomIdSet &affected)
{
    if (Room *const center = roomIndex(id)) {
        Coordinate other(0, -1, 0);
        other += center->getPosition();
        Room *room = map().get(other);
        if (room != nullptr) {
            affected.insert(room->getId());
        }
        other.y += 2;
        room = map().get(other);
        if (room != nullptr) {
            affected.insert(room->getId());
        }
        other.y--;
        other.x--;
        room = map().get(other);
        if (room != nullptr) {
            affected.insert(room->getId());
        }
        other.x += 2;
        room = map().get(other);
        if (room != nullptr) {
            affected.insert(room->getId());
        }
    }
}

void ConnectToNeighbours::exec(const RoomId cid)
{
    if (Room *const center = roomIndex(cid)) {
        Coordinate other(0, -1, 0);
        other += center->getPosition();
        connectRooms(center, other, ExitDirEnum::SOUTH, cid);
        other.y += 2;
        connectRooms(center, other, ExitDirEnum::NORTH, cid);
        other.y--;
        other.x--;
        connectRooms(center, other, ExitDirEnum::WEST, cid);
        other.x += 2;
        connectRooms(center, other, ExitDirEnum::EAST, cid);
        other.x--;
        other.z--;
        connectRooms(center, other, ExitDirEnum::DOWN, cid);
        other.z += 2;
        connectRooms(center, other, ExitDirEnum::UP, cid);
    }
}

void ConnectToNeighbours::connectRooms(Room *const center,
                                       Coordinate &otherPos,
                                       const ExitDirEnum dir,
                                       const RoomId cid)
{
    if (Room *const room = map().get(otherPos)) {
        const auto &cExit = center->getExitsList()[dir];
        const auto cFirstOut = cExit.isExit() && cExit.outIsEmpty();
        const ExitDirEnum oDir = opposite(dir);
        const auto &oExit = room->getExitsList()[oDir];
        const auto oFirstOut = oExit.isExit() && oExit.outIsEmpty();
        if (cFirstOut && oFirstOut) { // Add a two way exit
            center->addInOutExit(dir, room->getId());
            room->addInOutExit(oDir, cid);
        } else if (cFirstOut && !oFirstOut) { // Add a one way exit
            center->addOutExit(dir, room->getId());
            room->addInExit(oDir, cid);
        }
    }
}

ModifyRoomFlags::ModifyRoomFlags(const RoomFieldVariant in_flags, const FlagModifyModeEnum in_mode)
    : var{in_flags}
    , mode(in_mode)
{}

#define X_FOREACH_ROOM_CLASS_FIELD(X) \
    X(NOTE, Note, RoomNote) \
    X(MOB_FLAGS, MobFlags, RoomMobFlags) \
    X(LOAD_FLAGS, LoadFlags, RoomLoadFlags)

#define X_FOREACH_ROOM_ENUM_FIELD(X) \
    X(PORTABLE_TYPE, PortableType, RoomPortableEnum, NUM_PORTABLE_TYPES) \
    X(LIGHT_TYPE, LightType, RoomLightEnum, NUM_LIGHT_TYPES) \
    X(ALIGN_TYPE, AlignType, RoomAlignEnum, NUM_ALIGN_TYPES) \
    X(RIDABLE_TYPE, RidableType, RoomRidableEnum, NUM_RIDABLE_TYPES) \
    X(SUNDEATH_TYPE, SundeathType, RoomSundeathEnum, NUM_SUNDEATH_TYPES) \
    X(TERRAIN_TYPE, TerrainType, RoomTerrainEnum, NUM_ROOM_TERRAIN_TYPES)

void ModifyRoomFlags::exec(const RoomId id)
{
    if (Room *const room = roomIndex(id)) {
        switch (var.getType()) {
            // Room field classes
#define X_CASE(UPPER_CASE, CamelCase, Type) \
    { \
    case RoomFieldEnum::UPPER_CASE: \
        return room->set##CamelCase( \
            modifyField(room->get##CamelCase(), var.get##CamelCase(), mode)); \
    }
            X_FOREACH_ROOM_CLASS_FIELD(X_CASE)
#undef X_CASE
            // Room field enums
            // NOTE: TOGGLE assumes that the user never wants to toggle UNDEFINED enums
#define X_CASE(UPPER_CASE, CamelCase, Type, Size) \
    { \
    case RoomFieldEnum::UPPER_CASE: \
        switch (mode) { \
        case FlagModifyModeEnum::UNSET: \
            return room->set##CamelCase(static_cast<Type>(0)); \
        case FlagModifyModeEnum::SET: \
            return room->set##CamelCase(var.get##CamelCase()); \
        case FlagModifyModeEnum::TOGGLE: { \
            static_assert(Size >= 1); \
            const int next = static_cast<int>(room->get##CamelCase()) + 1; \
            if (next == Size) \
                return room->set##CamelCase(static_cast<Type>(1)); \
            return room->set##CamelCase(static_cast<Type>(next)); \
        } \
        default: \
            std::abort(); \
        } \
    }
            X_FOREACH_ROOM_ENUM_FIELD(X_CASE)
#undef X_CASE

            // REVISIT: RoomName requires that we enhance RoomFieldVariant
        case RoomFieldEnum::NAME:
        case RoomFieldEnum::DESC:
        case RoomFieldEnum::DYNAMIC_DESC:
        case RoomFieldEnum::LAST:
        case RoomFieldEnum::RESERVED:
        default:
            /* this can't happen */
            throw std::runtime_error("impossible");
        }
    }
}
#undef X_FOREACH_ROOM_CLASS_FIELD
#undef X_FOREACH_ROOM_ENUM_FIELD

ModifyRoomUpToDate::ModifyRoomUpToDate(const bool in_checked)
    : checked(in_checked)
{}

void ModifyRoomUpToDate::exec(const RoomId id)
{
    if (Room *room = roomIndex(id)) {
        if (checked)
            room->setUpToDate();
        else
            room->setOutDated();
    }
}

ModifyExitFlags::ModifyExitFlags(const ExitFieldVariant in_flags,
                                 const ExitDirEnum in_dir,
                                 const FlagModifyModeEnum in_mode)
    : var{in_flags}
    , mode(in_mode)
    , dir(in_dir)
{}

void ModifyExitFlags::exec(const RoomId id)
{
#define NOP()
#define X_CASE(UPPER_CASE, CamelCase) \
    { \
    case ExitFieldEnum::UPPER_CASE: \
        return room->set##CamelCase(dir, \
                                    modifyField(room->get##CamelCase(dir), \
                                                var.get##CamelCase(), \
                                                mode)); \
    }
    if (Room *const room = roomIndex(id)) {
        switch (var.getType()) {
            X_FOREACH_EXIT_FIELD(X_CASE, NOP)
#undef X_CASE
#undef NOP
        default:
            /* this can't happen */
            throw std::runtime_error("impossible");
        }
    }
}
