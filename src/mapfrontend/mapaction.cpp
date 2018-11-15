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

#include <memory>
#include <utility>
#include <QMutableVectorIterator>

#include "../expandoracommon/abstractroomfactory.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/room.h"
#include "../global/enums.h"
#include "../global/roomid.h"
#include "../mapdata/ExitDirection.h"
#include "../parser/CommandId.h"
#include "ParseTree.h"
#include "map.h"
#include "mapfrontend.h"
#include "roomcollection.h"

SingleRoomAction::SingleRoomAction(AbstractAction *const ex, const RoomId in_id)
    : id(in_id)
    , executor(ex)
{}

const RoomIdSet &SingleRoomAction::getAffectedRooms()
{
    executor->insertAffected(id, affectedRooms);
    return affectedRooms;
}

AddExit::AddExit(const RoomId in_from, const RoomId in_to, const ExitDirection in_dir)
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
    Room *rfrom = roomIndex(from);
    if (rfrom == nullptr) {
        return nullptr;
    }
    Room *rto = roomIndex(to);
    if (rto == nullptr) {
        return nullptr;
    }

    rfrom->exit(dir).addOut(to);
    rto->exit(opposite(dir)).addIn(from);
    return rfrom;
}

RemoveExit::RemoveExit(const RoomId in_from, const RoomId in_to, const ExitDirection in_dir)
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
    Room *rfrom = roomIndex(from);
    if (rfrom != nullptr) {
        rfrom->exit(dir).removeOut(to);
    }
    if (Room *rto = roomIndex(to)) {
        rto->exit(opposite(dir)).removeIn(from);
    }
    return rfrom;
}

void MakePermanent::exec(const RoomId id)
{
    if (Room *room = roomIndex(id)) {
        room->setPermanent();
    }
}

UpdateRoomField::UpdateRoomField(const QVariant &in_update, const uint in_fieldNum)
    : update(std::move(in_update))
    , fieldNum(in_fieldNum)
{}

void UpdateRoomField::exec(const RoomId id)
{
    /* TODO: update directly */
    if (Room *const room = roomIndex(id)) {
        room->replace(static_cast<RoomField>(fieldNum), update);
    }
}

Update::Update()
    : props(CommandIdType::NONE /* was effectively CommandIdType::NORTH */)
{
    props.reset();
}

Update::Update(const SigParseEvent &sigParseEvent)
    : props{CommandIdType::NONE}
{
    props = sigParseEvent.deref().clone();
    // assert(props.getNumSkipped() == 0);
}

void Update::exec(const RoomId id)
{
    if (Room *const room = roomIndex(id)) {
        factory()->update(*room, props);
        auto newHome = getParseTree().insertRoom(props);

        // NOTE: This requires a reference.
        // Consider removing roomHomes() and adding roomHomeRef(id).
        auto &homes = roomHomes();
        auto &home_ref = homes[id];

        if (auto &oldHome = home_ref) {
            oldHome->removeRoom(room);
        }
        home_ref = newHome;

        // REVISIT: newHome can be nullptr here!
        // Compare with other uses of insertRoom() and factor out common code.
        if (newHome != nullptr) {
            newHome->addRoom(room);
        }
    }
}

void ExitsAffecter::insertAffected(const RoomId id, RoomIdSet &affected)
{
    if (Room *const room = roomIndex(id)) {
        affected.insert(id);
        const ExitsList &exits = room->getExitsList();
        for (const Exit &e : exits) {
            for (auto idx : e.inRange()) {
                affected.insert(idx);
            }
            for (auto idx : e.outRange()) {
                affected.insert(idx);
            }
        }
    }
}

void Remove::exec(const RoomId id)
{
    // This still needs a reference so it can set it to nullptr
    auto &rooms = roomIndex();
    Room *const room = std::exchange(rooms[id], nullptr);
    if (room == nullptr) {
        return;
    }

    map().remove(room->getPosition());
    if (SharedRoomCollection home = roomHomes(id)) {
        home->removeRoom(room);
    }
    // don't return previously used ids for now
    // unusedIds().push(id);
    const ExitsList &exits = room->getExitsList();
    for (const auto dir : enums::makeCountingIterator<ExitDirection>(exits)) {
        const Exit &e = exits[dir];
        for (const auto idx : e.inRange()) {
            if (Room *const other = rooms[idx]) {
                other->exit(opposite(dir)).removeOut(id);
            }
        }
        for (const auto idx : e.outRange()) {
            if (Room *const other = rooms[idx]) {
                other->exit(opposite(dir)).removeIn(id);
            }
        }
    }
    delete room;
}

void FrontendAccessor::setFrontend(MapFrontend *const in)
{
    m_frontend = in;
}

Map &FrontendAccessor::map()
{
    return m_frontend->map;
}

ParseTree &FrontendAccessor::getParseTree()
{
    return m_frontend->parseTree;
}

RoomIndex &FrontendAccessor::roomIndex()
{
    return m_frontend->roomIndex;
}
Room *FrontendAccessor::roomIndex(const RoomId id) const
{
    return m_frontend->roomIndex[id];
}

RoomHomes &FrontendAccessor::roomHomes()
{
    return m_frontend->roomHomes;
}
const SharedRoomCollection &FrontendAccessor::roomHomes(const RoomId id) const
{
    return m_frontend->roomHomes[id];
}

std::stack<RoomId> &FrontendAccessor::unusedIds()
{
    return m_frontend->unusedIds;
}

AbstractRoomFactory *FrontendAccessor::factory()
{
    return m_frontend->factory;
}
