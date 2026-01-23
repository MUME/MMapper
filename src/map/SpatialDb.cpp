// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "SpatialDb.h"

#include "../global/AnsiOstream.h"
#include "../global/progresscounter.h"

const RoomId *SpatialDb::findUnique(const Coordinate &key) const
{
    m_cachedFindResult = m_index.findFirst(key);
    if (m_cachedFindResult.has_value()) {
        return &m_cachedFindResult.value();
    }
    return nullptr;
}

TinyRoomIdSet SpatialDb::findRooms(const Coordinate &key) const
{
    return m_index.findAt(key);
}

std::optional<RoomId> SpatialDb::findFirst(const Coordinate &key) const
{
    return m_index.findFirst(key);
}

bool SpatialDb::hasRoomAt(const Coordinate &key) const
{
    return m_index.hasRoomAt(key);
}

TinyRoomIdSet SpatialDb::findInBounds(const Bounds &bounds) const
{
    return m_index.findInBounds(bounds);
}

TinyRoomIdSet SpatialDb::findInRadius(const Coordinate &center, const int radius) const
{
    return m_index.findInRadius(center, radius);
}

void SpatialDb::remove(const RoomId id, const Coordinate &coord)
{
    m_index.remove(id, coord);
}

void SpatialDb::add(const RoomId id, const Coordinate &coord)
{
    m_index.insert(id, coord);
}

void SpatialDb::move(const RoomId id, const Coordinate &from, const Coordinate &to)
{
    m_index.move(id, from, to);
}

void SpatialDb::updateBounds(ProgressCounter &pc)
{
    m_index.updateBounds(pc);
}

void SpatialDb::printStats(ProgressCounter &pc, AnsiOstream &os) const
{
    m_index.printStats(pc, os);
}
