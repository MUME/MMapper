// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

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

RoomSelection::RoomSelection(MapData &admin, const Coordinate &min, const Coordinate &max)
    : RoomSelection(admin)
{
    m_mapData.lookingForRooms(*this, min, max);
}

void RoomSelection::receiveRoom(RoomAdmin *const admin, const Room *const aRoom)
{
    assert(admin == &m_mapData);
    insert(aRoom->getId(), aRoom);
}

RoomSelection::~RoomSelection()
{
    // Remove the lock within the map
    for (auto i = cbegin(); i != cend(); i++) {
        m_mapData.releaseRoom(*this, i.key());
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
    m_mapData.genericSearch(checked_static_upcast<RoomRecipient *>(this), f);
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
                                                   const Coordinate &min,
                                                   const Coordinate &max)
{
    return std::make_shared<RoomSelection>(mapData, min, max);
}
