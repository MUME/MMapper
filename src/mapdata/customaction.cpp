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

#include "customaction.h"

#include <memory>
#include <set>
#include <stdexcept>
#include <utility>

#include "../expandoracommon/abstractroomfactory.h"
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

GroupMapAction::GroupMapAction(AbstractAction *const action, const RoomSelection *const selection)
    : executor(action)
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
    if (room2Dir == ExitDirection::UNKNOWN) {
        room2Dir = opposite(dir);
    }
    AddOneWayExit::exec();
    std::swap(to, from);
    dir = room2Dir;
    AddOneWayExit::exec();
}

void RemoveTwoWayExit::exec()
{
    if (room2Dir == ExitDirection::UNKNOWN) {
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
        factory()->update(target, source);
        auto oid = target->getId();
        const SharedRoomCollection newHome = [this, &target]() {
            auto props = factory()->getEvent(target);
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
        for (const auto dir : enums::makeCountingIterator<ExitDirection>(exits)) {
            const Exit &e = exits[dir];
            for (const auto oeid : e.inRange()) {
                if (Room *const oe = roomIndex(oeid)) {
                    oe->exit(opposite(dir)).addOut(oid);
                    target->exit(dir).addIn(oeid);
                }
            }
            for (const auto oeid : e.outRange()) {
                if (Room *const oe = roomIndex(oeid)) {
                    oe->exit(opposite(dir)).addIn(oid);
                    target->exit(dir).addOut(oeid);
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
        connectRooms(center, other, ExitDirection::NORTH, cid);
        other.y += 2;
        connectRooms(center, other, ExitDirection::SOUTH, cid);
        other.y--;
        other.x--;
        connectRooms(center, other, ExitDirection::WEST, cid);
        other.x += 2;
        connectRooms(center, other, ExitDirection::EAST, cid);
    }
}

void ConnectToNeighbours::connectRooms(Room *const center,
                                       Coordinate &otherPos,
                                       const ExitDirection dir,
                                       const RoomId cid)
{
    if (Room *const room = map().get(otherPos)) {
        const auto oid = room->getId();
        Exit &oexit = room->exit(opposite(dir));
        oexit.addIn(cid);
        oexit.addOut(cid);
        Exit &cexit = center->exit(dir);
        cexit.addIn(oid);
        cexit.addOut(oid);
    }
}

void DisconnectFromNeighbours::exec(const RoomId id)
{
    if (Room *room = roomIndex(id)) {
        ExitsList &exits = room->getExitsList();

        for (auto dir : enums::makeCountingIterator<ExitDirection>(exits)) {
            Exit &e = exits[dir];
            for (auto idx : e.inRange()) {
                if (Room *other = roomIndex(idx)) {
                    other->exit(opposite(dir)).removeOut(id);
                }
            }
            for (auto idx : e.outRange()) {
                if (Room *other = roomIndex(idx)) {
                    other->exit(opposite(dir)).removeIn(id);
                }
            }
            e.removeAll();
        }
    }
}

ModifyRoomFlags::ModifyRoomFlags(const uint in_flags,
                                 const RoomField in_fieldNum,
                                 const FlagModifyMode in_mode)
    : flags(in_flags)
    , fieldNum(in_fieldNum)
    , mode(in_mode)
{}

template<typename Flags, typename Flag>
static Flags modifyFlags(const Flags flags, const Flag x, const FlagModifyMode mode)
{
    switch (mode) {
    case FlagModifyMode::SET:
        return flags | x;
    case FlagModifyMode::UNSET:
        return flags & (~x);
    case FlagModifyMode::TOGGLE:
        return flags ^ x;
    }
    return flags;
}

void ModifyRoomFlags::exec(const RoomId id)
{
    if (Room *room = roomIndex(id)) {
        /* REVISIT: should there be an error here if the field really contains a string? */
        const auto field = this->fieldNum;
        room->modifyUint(field, [this](uint roomFlags) {
            return modifyFlags(roomFlags, this->flags, this->mode);
        });
    }
}

UpdateExitField::UpdateExitField(const ExitFieldVariant &in_update, const ExitDirection in_dir)
    : update(in_update)
    , dir(in_dir)
{}
UpdateExitField::UpdateExitField(const DoorName &update, const ExitDirection dir)
    : update{update}
    , dir{dir}
{}

void UpdateExitField::exec(const RoomId id)
{
    if (Room *room = roomIndex(id)) {
        Exit &ex = room->exit(dir);
        ex.setField(update);
    }
}

ModifyExitFlags::ModifyExitFlags(const ExitFieldVariant in_flags,
                                 const ExitDirection in_dir,
                                 const FlagModifyMode in_mode)
    : var{in_flags}
    , mode(in_mode)
    , dir(in_dir)
{
    // The DoorName version is actually implemented, but it has never been used.
    // Test it if you use it!
    if (in_flags.getType() == ExitField::DOOR_NAME)
        throw std::runtime_error("not implemented");
}

ModifyExitFlags::ModifyExitFlags(const ExitFlags in_flags,
                                 const ExitDirection in_dir,
                                 const FlagModifyMode in_mode)
    : var{in_flags}
    , mode(in_mode)
    , dir(in_dir)
{}

ModifyExitFlags::ModifyExitFlags(const DoorFlags in_flags,
                                 const ExitDirection in_dir,
                                 const FlagModifyMode in_mode)
    : var{in_flags}
    , mode(in_mode)
    , dir(in_dir)
{}

static void modifyDoorName(Exit &ex, const ExitFieldVariant var, const FlagModifyMode mode)
{
    switch (mode) {
    case FlagModifyMode::SET:
        ex.setDoorName(var.getDoorName());
        break;
    case FlagModifyMode::UNSET:
        // this doesn't really make sense either.
        ex.clearDoorName();
        break;
    case FlagModifyMode::TOGGLE:
        // the idea of toggling a door name doesn't make sense,
        // so this implementation is as good as any.
        if (ex.hasDoorName())
            ex.setDoorName(var.getDoorName());
        else
            ex.clearDoorName();
        break;
    default:
        /* this can't happen */
        throw std::runtime_error("impossible");
    }
}

void ModifyExitFlags::exec(const RoomId id)
{
    if (Room *const room = roomIndex(id)) {
        Exit &ex = room->exit(dir);

        switch (var.getType()) {
        case ExitField::DOOR_NAME:
            modifyDoorName(ex, var, mode);
            break;
        case ExitField::EXIT_FLAGS:
            ex.setExitFlags(modifyFlags(ex.getExitFlags(), var.getExitFlags(), mode));
            break;
        case ExitField::DOOR_FLAGS:
            ex.setDoorFlags(modifyFlags(ex.getDoorFlags(), var.getDoorFlags(), mode));
            break;
        default:
            /* this can't happen */
            throw std::runtime_error("impossible");
        }
    }
}
