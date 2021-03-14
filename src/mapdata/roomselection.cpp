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

NODISCARD static Coordinate toCoordinate(const glm::ivec3 &c)
{
    return Coordinate(c.x, c.y, c.z);
}

NODISCARD static Coordinate min(const Coordinate &a, const Coordinate &b)
{
    return toCoordinate(glm::min(a.to_ivec3(), b.to_ivec3()));
}

NODISCARD static Coordinate max(const Coordinate &a, const Coordinate &b)
{
    return toCoordinate(glm::max(a.to_ivec3(), b.to_ivec3()));
}

RoomSelection::RoomSelection(MapData &admin, const Coordinate &a, const Coordinate &b)
    : RoomSelection(admin)
{
    m_mapData.lookingForRooms(*this, min(a, b), max(a, b));
}

void RoomSelection::virt_receiveRoom(RoomAdmin *const admin, const Room *const aRoom)
{
    assert(admin == &m_mapData);
    emplace(aRoom->getId(), aRoom);
}

RoomSelection::~RoomSelection()
{
    // Remove the lock within the map
    for (auto i = cbegin(); i != cend(); i++) {
        m_mapData.releaseRoom(*this, i->first);
    }
}

const Room *RoomSelection::getFirstRoom() const noexcept(false)
{
    if (empty())
        throw std::runtime_error("empty");
    return cbegin()->second;
}

RoomId RoomSelection::getFirstRoomId() const noexcept(false)
{
    return getFirstRoom()->getId();
}

bool RoomSelection::contains(RoomId targetId) const
{
    return find(targetId) != end();
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
    m_mapData.releaseRoom(*this, targetId);
    erase(find(targetId));
}

bool RoomSelection::isMovable(const Coordinate &offset) const
{
    for (auto i = cbegin(); i != cend(); i++) {
        const Coordinate target = i->second->getPosition() + offset;
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
