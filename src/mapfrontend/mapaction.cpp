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

#include "mapaction.h"
#include "abstractroomfactory.h"
#include "intermediatenode.h"
#include "map.h"
#include "mapfrontend.h"
#include "room.h"
#include "roomcollection.h"
#include <cassert>

#include <utility>

SingleRoomAction::SingleRoomAction(AbstractAction *ex, uint in_id)
    : id(in_id)
    , executor(ex)
{}

const std::set<uint> &SingleRoomAction::getAffectedRooms()
{
    executor->insertAffected(id, affectedRooms);
    return affectedRooms;
}

AddExit::AddExit(uint in_from, uint in_to, uint in_dir)
    : from(in_from)
    , to(in_to)
    , dir(in_dir)
{
    affectedRooms.insert(from);
    affectedRooms.insert(to);
}

void AddExit::exec()
{
    tryExec();
}

Room *AddExit::tryExec()
{
    auto &rooms = roomIndex();
    Room *rfrom = rooms[from];
    if (rfrom == nullptr) {
        return nullptr;
    }
    Room *rto = rooms[to];
    if (rto == nullptr) {
        return nullptr;
    }

    rfrom->exit(dir).addOut(to);
    rto->exit(factory()->opposite(dir)).addIn(from);
    return rfrom;
}

RemoveExit::RemoveExit(uint in_from, uint in_to, uint in_dir)
    : from(in_from)
    , to(in_to)
    , dir(in_dir)
{
    affectedRooms.insert(from);
    affectedRooms.insert(to);
}

void RemoveExit::exec()
{
    tryExec();
}

Room *RemoveExit::tryExec()
{
    auto &rooms = roomIndex();
    Room *rfrom = rooms[from];
    if (rfrom != nullptr) {
        rfrom->exit(dir).removeOut(to);
    }
    Room *rto = rooms[to];
    if (rto != nullptr) {
        rto->exit(factory()->opposite(dir)).removeIn(from);
    }
    return rfrom;
}

void MakePermanent::exec(uint id)
{
    Room *room = roomIndex()[id];
    if (room != nullptr) {
        room->setPermanent();
    }
}

UpdateRoomField::UpdateRoomField(const QVariant &in_update, uint in_fieldNum)
    : update(std::move(in_update))
    , fieldNum(in_fieldNum)
{}

void UpdateRoomField::exec(uint id)
{
    if (Room *room = roomIndex()[id]) {
        room->replace(static_cast<RoomField>(fieldNum), update);
    }
}

Update::Update()
    : props(0)
{
    props.reset();
}

Update::Update(ParseEvent &in_props)
    : props(in_props)
{
    //assert(props.getNumSkipped() == 0);
}

UpdatePartial::UpdatePartial(const QVariant &in_val, uint in_pos)
    : Update()
    , UpdateRoomField(in_val, in_pos)
{
    // This function is never called,
    // as proven by the fact that it was calling Update(nullptr)
    std::abort();
}

void UpdatePartial::exec(uint id)
{
    Room *room = roomIndex()[id];
    if (room != nullptr) {
        UpdateRoomField::exec(id);
        props = *(factory()->getEvent(room));
        Update::exec(id);
    }
}

void Update::exec(uint id)
{
    Room *room = roomIndex()[id];
    if (room != nullptr) {
        factory()->update(*room, props);
        RoomCollection *newHome = treeRoot().insertRoom(props);
        auto &homes = roomHomes();
        RoomCollection *oldHome = homes[id];
        if (oldHome != nullptr) {
            oldHome->erase(room);
        }
        homes[id] = newHome;
        newHome->insert(room);
    }
}

void ExitsAffecter::insertAffected(uint id, std::set<uint> &affected)
{
    if (Room *room = roomIndex()[id]) {
        affected.insert(id);
        const ExitsList &exits = room->getExitsList();
        ExitsList::const_iterator exitIter = exits.begin();
        while (exitIter != exits.end()) {
            const Exit &e = *exitIter;
            for (auto idx : e.inRange()) {
                affected.insert(idx);
            }
            for (auto idx : e.outRange()) {
                affected.insert(idx);
            }
            ++exitIter;
        }
    }
}

void Remove::exec(uint id)
{
    auto &rooms = roomIndex();
    Room *room = std::exchange(rooms[id], nullptr);
    if (room == nullptr) {
        return;
    }

    map().remove(room->getPosition());
    if (RoomCollection *home = roomHomes()[id]) {
        home->erase(room);
    }
    // don't return previously used ids for now
    // unusedIds().push(id);
    const ExitsList &exits = room->getExitsList();
    for (int dir = 0; dir < exits.size(); ++dir) {
        const Exit &e = exits[dir];
        for (auto idx : e.inRange()) {
            if (Room *other = rooms[idx]) {
                other->exit(factory()->opposite(dir)).removeOut(id);
            }
        }
        for (auto idx : e.outRange()) {
            if (Room *other = rooms[idx]) {
                other->exit(factory()->opposite(dir)).removeIn(id);
            }
        }
    }
    delete room;
}

void FrontendAccessor::setFrontend(MapFrontend *in)
{
    m_frontend = in;
}

Map &FrontendAccessor::map()
{
    return m_frontend->map;
}

IntermediateNode &FrontendAccessor::treeRoot()
{
    return m_frontend->treeRoot;
}

std::vector<Room *> &FrontendAccessor::roomIndex()
{
    return m_frontend->roomIndex;
}

std::vector<RoomCollection *> &FrontendAccessor::roomHomes()
{
    return m_frontend->roomHomes;
}

std::stack<uint> &FrontendAccessor::unusedIds()
{
    return m_frontend->unusedIds;
}

AbstractRoomFactory *FrontendAccessor::factory()
{
    return m_frontend->factory;
}
