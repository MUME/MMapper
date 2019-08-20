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

GroupMapAction::GroupMapAction(std::unique_ptr<AbstractAction> action,
                               const SharedRoomSelection &selection)
    : executor(std::move(action))
{
    QMapIterator<RoomId, const Room *> roomIter(*selection);
    while (roomIter.hasNext()) {
        RoomId rid = roomIter.next().key();
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
        connectRooms(center, other, ExitDirEnum::NORTH, cid);
        other.y += 2;
        connectRooms(center, other, ExitDirEnum::SOUTH, cid);
        other.y--;
        other.x--;
        connectRooms(center, other, ExitDirEnum::WEST, cid);
        other.x += 2;
        connectRooms(center, other, ExitDirEnum::EAST, cid);
    }
}

void ConnectToNeighbours::connectRooms(Room *const center,
                                       Coordinate &otherPos,
                                       const ExitDirEnum dir,
                                       const RoomId cid)
{
    if (Room *const room = map().get(otherPos)) {
        const ExitDirEnum oDir = opposite(dir);
        const auto &oExit = room->getExitsList()[oDir];
        // REVISIT: Should this logic happen within addInOutExit() ?
        if (oExit.isExit())
            room->addInOutExit(oDir, cid);
        const auto &cExit = center->getExitsList()[dir];
        if (cExit.isExit())
            center->addInOutExit(dir, room->getId());
    }
}

ModifyRoomFlags::ModifyRoomFlags(const RoomFieldVariant in_flags, const FlagModifyModeEnum in_mode)
    : var{in_flags}
    , mode(in_mode)
{
    // TODO: Assert non-supported
}

ModifyRoomFlags::ModifyRoomFlags(const RoomMobFlags in_flags, const FlagModifyModeEnum in_mode)
    : var{in_flags}
    , mode(in_mode)
{}

ModifyRoomFlags::ModifyRoomFlags(const RoomLoadFlags in_flags, const FlagModifyModeEnum in_mode)
    : var{in_flags}
    , mode(in_mode)
{}

void ModifyRoomFlags::exec(const RoomId id)
{
    if (Room *const room = roomIndex(id)) {
        switch (var.getType()) {
        case RoomFieldEnum::MOB_FLAGS:
            room->setMobFlags(modifyFlags(room->getMobFlags(), var.getMobFlags(), mode));
            break;
        case RoomFieldEnum::LOAD_FLAGS:
            room->setLoadFlags(modifyFlags(room->getLoadFlags(), var.getLoadFlags(), mode));
            break;
        case RoomFieldEnum::NOTE:
            // TODO: Re-use logic from modifyDoorName(ex, var, mode);
        case RoomFieldEnum::TERRAIN_TYPE:
        case RoomFieldEnum::PORTABLE_TYPE:
        case RoomFieldEnum::LIGHT_TYPE:
        case RoomFieldEnum::RIDABLE_TYPE:
        case RoomFieldEnum::SUNDEATH_TYPE:
        case RoomFieldEnum::ALIGN_TYPE:
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

UpdateExitField::UpdateExitField(const DoorName &update, const ExitDirEnum dir)
    : update{update}
    , dir{dir}
{}

void UpdateExitField::exec(const RoomId id)
{
    if (Room *const room = roomIndex(id)) {
        room->updateExitField(dir, update);
    }
}

ModifyExitFlags::ModifyExitFlags(const ExitFieldVariant in_flags,
                                 const ExitDirEnum in_dir,
                                 const FlagModifyModeEnum in_mode)
    : var{in_flags}
    , mode(in_mode)
    , dir(in_dir)
{
    // The DoorName version is actually implemented, but it has never been used.
    // Test it if you use it!
    if (in_flags.getType() == ExitFieldEnum::DOOR_NAME)
        throw std::runtime_error("not implemented");
}

ModifyExitFlags::ModifyExitFlags(const ExitFlags in_flags,
                                 const ExitDirEnum in_dir,
                                 const FlagModifyModeEnum in_mode)
    : var{in_flags}
    , mode(in_mode)
    , dir(in_dir)
{}

ModifyExitFlags::ModifyExitFlags(const DoorFlags in_flags,
                                 const ExitDirEnum in_dir,
                                 const FlagModifyModeEnum in_mode)
    : var{in_flags}
    , mode(in_mode)
    , dir(in_dir)
{}

void ModifyExitFlags::exec(const RoomId id)
{
    if (Room *const room = roomIndex(id)) {
        room->modifyExitFlags(dir, mode, var);
    }
}
