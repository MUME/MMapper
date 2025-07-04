#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/ImmUnorderedMap.h"
#include "../global/macros.h"
#include "coordinate.h"
#include "roomid.h"

#include <optional>

class AnsiOstream;
class ProgressCounter;

struct NODISCARD SpatialDb final
{
private:
    /// Value is the last room assigned to the coordinate.
    ImmUnorderedMap<Coordinate, RoomId> m_unique;

private:
    std::optional<Bounds> m_bounds;
    bool m_needsBoundsUpdate = true;

public:
    NODISCARD bool needsBoundsUpdate() const
    {
        return m_needsBoundsUpdate || !m_bounds.has_value();
    }
    NODISCARD const std::optional<Bounds> &getBounds() const { return m_bounds; }

public:
    NODISCARD const RoomId *findUnique(const Coordinate &key) const;

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
        m_unique.for_each([&callback](const auto &p) { callback(p.first, p.second); });
    }
    NODISCARD auto size() const { return m_unique.size(); }

public:
    NODISCARD bool operator==(const SpatialDb &rhs) const { return m_unique == rhs.m_unique; }
    NODISCARD bool operator!=(const SpatialDb &rhs) const { return !(rhs == *this); }
};
