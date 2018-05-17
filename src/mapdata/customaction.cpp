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

#include "abstractroomfactory.h"
#include "intermediatenode.h"
#include "map.h"
#include "mmapper2exit.h"
#include "room.h"
#include "roomcollection.h"
#include "roomselection.h"
#include <utility>

using namespace std;

GroupAction::GroupAction(AbstractAction *action, const RoomSelection *selection) : executor(action)
{
    QMapIterator<uint, const Room *> roomIter(*selection);
    while (roomIter.hasNext()) {
        uint rid = roomIter.next().key();
        affectedRooms.insert(rid);
        selectedRooms.push_back(rid);
    }
}

void AddTwoWayExit::exec()
{
    if (room2Dir == UINT_MAX) {
        room2Dir = Mmapper2Exit::opposite(dir);
    }
    AddOneWayExit::exec();
    uint temp = to;
    to = from;
    from = temp;
    dir = room2Dir;
    AddOneWayExit::exec();
}



void RemoveTwoWayExit::exec()
{
    if (room2Dir == UINT_MAX) {
        room2Dir = Mmapper2Exit::opposite(dir);
    }
    RemoveOneWayExit::exec();
    uint temp = to;
    to = from;
    from = temp;
    dir = room2Dir;
    RemoveOneWayExit::exec();
}


const std::set<uint> &GroupAction::getAffectedRooms()
{
    for (unsigned int &selectedRoom : selectedRooms) {
        executor->insertAffected(selectedRoom, affectedRooms);
    }
    return affectedRooms;
}

void GroupAction::exec()
{
    for (unsigned int &selectedRoom : selectedRooms) {
        executor->preExec(selectedRoom);
    }
    for (unsigned int &selectedRoom : selectedRooms) {
        executor->exec(selectedRoom);
    }
}

MoveRelative::MoveRelative(const Coordinate &in_move) :
    move(in_move)
{}

void MoveRelative::exec(uint id)
{
    Room *room = roomIndex()[id];
    if (room != nullptr) {
        const Coordinate &c = room->getPosition();
        Coordinate newPos = c + move;
        map().setNearest(newPos, room);
    }
}

void MoveRelative::preExec(uint id)
{
    Room *room = roomIndex()[id];
    if (room != nullptr) {
        map().remove(room->getPosition());
    }
}


MergeRelative::MergeRelative(const Coordinate &in_move) :
    move(in_move)
{}

void MergeRelative::insertAffected(uint id, set<uint> &affected)
{
    Room *room = roomIndex()[id];
    if (room != nullptr) {
        ExitsAffecter::insertAffected(id, affected);
        const Coordinate &c = room->getPosition();
        Coordinate newPos = c + move;
        Room *other = map().get(newPos);
        if (other != nullptr) {
            affected.insert(other->getId());
        }
    }
}

void MergeRelative::preExec(uint id)
{
    Room *room = roomIndex()[id];
    if (room != nullptr) {
        map().remove(room->getPosition());
    }
}

void MergeRelative::exec(uint id)
{
    Room *source = roomIndex()[id];
    if (source == nullptr) {
        return;
    }
    Map &roomMap = map();
    vector<Room *> &rooms = roomIndex();
    const Coordinate &c = source->getPosition();
    Coordinate newPos = c + move;
    Room *target = roomMap.get(newPos);
    if (target != nullptr) {
        factory()->update(target, source);
        ParseEvent *props = factory()->getEvent(target);
        uint oid = target->getId();
        RoomCollection *newHome = treeRoot().insertRoom(props);
        vector<RoomCollection *> &homes = roomHomes();
        RoomCollection *oldHome = homes[oid];
        if (oldHome != nullptr) {
            oldHome->erase(target);
        }

        homes[oid] = newHome;
        if (newHome != nullptr) {
            newHome->insert(target);
        }
        const ExitsList &exits = source->getExitsList();
        for (int dir = 0; dir < exits.size(); ++dir) {
            const Exit &e = exits[dir];
            for (auto i = e.inBegin(); i != e.inEnd(); ++i) {
                uint oeid = *i;
                Room *oe = rooms[oeid];
                if (oe != nullptr) {
                    oe->exit(Mmapper2Exit::opposite(dir)).addOut(oid);
                    target->exit(dir).addIn(oeid);
                }
            }
            for (auto i = e.outBegin(); i != e.outEnd(); ++i) {
                uint oeid = *i;
                Room *oe = rooms[oeid];
                if (oe != nullptr) {
                    oe->exit(Mmapper2Exit::opposite(dir)).addIn(oid);
                    target->exit(dir).addOut(oeid);
                }
            }
        }
        Remove::exec(source->getId());
    } else {
        Coordinate newPos = c + move;
        roomMap.setNearest(newPos, source);
    }
}


void ConnectToNeighbours::insertAffected(uint id, set<uint> &affected)
{
    Room *center = roomIndex()[id];
    if (center != nullptr) {
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

void ConnectToNeighbours::exec(uint cid)
{
    Room *center = roomIndex()[cid];
    if (center != nullptr) {
        Coordinate other(0, -1, 0);
        other += center->getPosition();
        connectRooms(center, other, ED_NORTH, cid);
        other.y += 2;
        connectRooms(center, other, ED_SOUTH, cid);
        other.y--;
        other.x--;
        connectRooms(center, other, ED_WEST, cid);
        other.x += 2;
        connectRooms(center, other, ED_EAST, cid);
    }
}

void ConnectToNeighbours::connectRooms(Room *center, Coordinate &otherPos, uint dir, uint cid)
{
    Room *room = map().get(otherPos);
    if (room != nullptr) {
        uint oid = room->getId();
        Exit &oexit = room->exit(Mmapper2Exit::opposite(dir));
        oexit.addIn(cid);
        oexit.addOut(cid);
        Exit &cexit = center->exit(dir);
        cexit.addIn(oid);
        cexit.addOut(oid);
    }
}

void DisconnectFromNeighbours::exec(uint id)
{
    Room *room = roomIndex()[id];
    if (room != nullptr) {
        ExitsList &exits = room->getExitsList();
        for (int dir = 0; dir < exits.size(); ++dir) {
            Exit &e = exits[dir];
            for (auto in = e.inBegin(); in != e.inEnd(); ++in) {
                Room *other = roomIndex()[*in];
                if (other != nullptr) {
                    other->exit(Mmapper2Exit::opposite(dir)).removeOut(id);
                }
            }
            for (auto out = e.outBegin(); out != e.outEnd(); ++out) {
                Room *other = roomIndex()[*out];
                if (other != nullptr) {
                    other->exit(Mmapper2Exit::opposite(dir)).removeIn(id);
                }
            }
            e.removeAll();
        }
    }
}

ModifyRoomFlags::ModifyRoomFlags(uint in_flags, uint in_fieldNum, FlagModifyMode in_mode) :
    flags(in_flags), fieldNum(in_fieldNum), mode(in_mode) {}

void modifyFlags(uint &roomFlags, uint flags, FlagModifyMode mode)
{
    switch (mode) {
    case FMM_SET:
        roomFlags |= flags;
        break;
    case FMM_UNSET:
        roomFlags &= (~flags);
        break;
    case FMM_TOGGLE:
        roomFlags ^= flags;
        break;
    }
}

void ModifyRoomFlags::exec(uint id)
{
    Room *room = roomIndex()[id];
    if (room != nullptr) {
        uint roomFlags = (*room)[fieldNum].toUInt();
        modifyFlags(roomFlags, flags, mode);
        (*room)[fieldNum] = roomFlags;
    }
}

UpdateExitField::UpdateExitField(const QVariant &in_update, uint in_dir, uint in_fieldNum) :
    update(std::move(in_update)), fieldNum(in_fieldNum), dir(in_dir) {}

void UpdateExitField::exec(uint id)
{
    Room *room = roomIndex()[id];
    if (room != nullptr) {
        room->exit(dir)[fieldNum] = update;
    }
}

ModifyExitFlags::ModifyExitFlags(uint in_flags, uint in_dir, uint in_fieldNum,
                                 FlagModifyMode in_mode) :
    flags(in_flags), fieldNum(in_fieldNum), mode(in_mode), dir(in_dir) {}

void ModifyExitFlags::exec(uint id)
{
    Room *room = roomIndex()[id];
    if (room != nullptr) {
        uint exitFlags = room->exit(dir)[fieldNum].toUInt();
        modifyFlags(exitFlags, flags, mode);
        room->exit(dir)[fieldNum] = exitFlags;
    }
}
