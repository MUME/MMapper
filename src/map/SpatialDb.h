#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "SpatialIndex.h"
#include "TinyRoomIdSet.h"
#include "coordinate.h"
#include "roomid.h"

#include <optional>

class AnsiOstream;
class ProgressCounter;

/// SpatialDb provides spatial indexing for rooms.
/// Now backed by SpatialIndex (quadtree) which supports multiple rooms per coordinate.
struct NODISCARD SpatialDb final
{
private:
    SpatialIndex m_index;

public:
    NODISCARD bool needsBoundsUpdate() const { return m_index.needsBoundsUpdate(); }
    NODISCARD std::optional<Bounds> getBounds() const { return m_index.getBounds(); }

public:
    /// Find all rooms at coordinate
    NODISCARD TinyRoomIdSet findRooms(const Coordinate &key) const;

    /// Find first room at coordinate (new interface, cleaner than findUnique)
    NODISCARD std::optional<RoomId> findFirst(const Coordinate &key) const;

    /// Check if any room exists at coordinate
    NODISCARD bool hasRoomAt(const Coordinate &key) const;

public:
    /// Find all rooms within bounding box
    NODISCARD TinyRoomIdSet findInBounds(const Bounds &bounds) const;

    /// Find all rooms within radius of center
    NODISCARD TinyRoomIdSet findInRadius(const Coordinate &center, int radius) const;

public:
    void remove(RoomId id, const Coordinate &coord);
    void add(RoomId id, const Coordinate &coord);
    void move(RoomId id, const Coordinate &from, const Coordinate &to);
    void updateBounds(ProgressCounter &pc);
    void printStats(ProgressCounter &pc, AnsiOstream &os) const;

public:
    // Callback = void(const Coordinate&, RoomId);
    template<typename Callback>
    void for_each(Callback &&callback) const
    {
        static_assert(std::is_invocable_r_v<void, Callback, const Coordinate &, RoomId>);
        m_index.forEach([&callback](const RoomId id, const Coordinate &coord) {
            callback(coord, id);
        });
    }

    NODISCARD size_t size() const { return m_index.size(); }

public:
    NODISCARD bool operator==(const SpatialDb &rhs) const { return m_index == rhs.m_index; }
    NODISCARD bool operator!=(const SpatialDb &rhs) const { return !(rhs == *this); }
};
