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

#include "roomselection.h"

#include <cassert>
#include <memory>

#include "../expandoracommon/room.h"
#include "mapdata.h"

RoomSelection::RoomSelection(MapData &admin)
    : m_mapData(admin)
{}

RoomSelection::RoomSelection(MapData &admin, const Coordinate &c)
    : RoomSelection(admin)
{
    m_mapData.lookingForRooms(*this, c);
}

RoomSelection::RoomSelection(MapData &admin, const Coordinate &ulf, const Coordinate &lrb)
    : RoomSelection(admin)
{
    m_mapData.lookingForRooms(*this, ulf, lrb);
}

void RoomSelection::receiveRoom(RoomAdmin *admin, const Room *aRoom)
{
    assert(admin == &m_mapData);
    insert(aRoom->getId(), aRoom);
}

RoomSelection::~RoomSelection()
{
    // Remove the lock within the map
    for (auto i = cbegin(); i != cend(); i++) {
        m_mapData.keepRoom(*this, i.key());
    }
}

const Room *RoomSelection::getFirstRoom() const noexcept(false)
{
    if (isEmpty())
        throw std::runtime_error("empty");
    return first();
}

RoomId RoomSelection::getFirstRoomId() const noexcept(false)
{
    return getFirstRoom()->getId();
}

const Room *RoomSelection::getRoom(const RoomId targetId)
{
    return m_mapData.getRoom(targetId, *this);
}

const Room *RoomSelection::getRoom(const Coordinate &coord)
{
    return m_mapData.getRoom(coord, *this);
}

void RoomSelection::unselect(const RoomId targetId)
{
    m_mapData.keepRoom(*this, targetId);
    remove(targetId);
}

bool RoomSelection::isMovable(const Coordinate &offset) const
{
    for (auto i = cbegin(); i != cend(); i++) {
        const Coordinate target = i.value()->getPosition() + offset;
        RoomSelection tmpSel = RoomSelection(m_mapData);
        if (const Room *const other = tmpSel.getRoom(target)) {
            if (!contains(other->getId())) {
                return false;
            }
        }
    }
    return true;
}

void RoomSelection::genericSearch(const RoomFilter &f)
{
    m_mapData.genericSearch(dynamic_cast<RoomRecipient *>(this), f);
}

SharedRoomSelection RoomSelection::createSelection(MapData &mapData)
{
    return std::make_shared<RoomSelection>(mapData);
}

SharedRoomSelection RoomSelection::createSelection(MapData &mapData, const Coordinate &c)
{
    return std::make_shared<RoomSelection>(mapData, c);
}

SharedRoomSelection RoomSelection::createSelection(MapData &mapData,
                                                   const Coordinate &ulf,
                                                   const Coordinate &lrb)
{
    return std::make_shared<RoomSelection>(mapData, ulf, lrb);
}
