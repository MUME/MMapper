#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
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

#ifndef MMAPPER_ROADINDEX_H
#define MMAPPER_ROADINDEX_H

#include <cstdint>

#include "../global/Flags.h"
#include "../mapdata/ExitDirection.h"

class Room;

static constexpr const int NUM_COMPASS_DIRS = 4;
static constexpr const int NUM_ROAD_INDICES = 1 << NUM_COMPASS_DIRS;

enum class RoadIndex : uint32_t {
#define BIT_FROM_EXDIR(x) x = (1 << static_cast<int>(ExitDirection::x))
    NONE = 0,
    BIT_FROM_EXDIR(NORTH),
    BIT_FROM_EXDIR(SOUTH),
    BIT_FROM_EXDIR(EAST),
    BIT_FROM_EXDIR(WEST),
    ALL = NUM_ROAD_INDICES - 1
#undef BIT_FROM_EXDIR
};
static_assert(static_cast<int>(RoadIndex::ALL) == 15, "");
DEFINE_ENUM_COUNT(RoadIndex, NUM_ROAD_INDICES)

inline constexpr RoadIndex operator|(RoadIndex lhs, RoadIndex rhs) noexcept
{
    return static_cast<RoadIndex>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}
inline constexpr RoadIndex &operator|=(RoadIndex &lhs, RoadIndex rhs) noexcept
{
    return lhs = (lhs | rhs);
}
inline constexpr RoadIndex operator&(RoadIndex lhs, RoadIndex rhs)
{
    return static_cast<RoadIndex>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}
inline constexpr RoadIndex operator~(RoadIndex x)
{
    return static_cast<RoadIndex>(static_cast<uint32_t>(x) ^ static_cast<uint32_t>(RoadIndex::ALL));
}
static_assert(~RoadIndex::ALL == RoadIndex::NONE, "");
static_assert(~RoadIndex::NONE == RoadIndex::ALL, "");

RoadIndex getRoadIndex(const ExitDirection dir);
RoadIndex getRoadIndex(const Room &room);

enum class RoadIndexType { ROAD, TRAIL };
template<RoadIndexType Tag>
struct TaggedRoadIndex final
{
    static constexpr const RoadIndexType tag_type = Tag;
    RoadIndex index;
    explicit constexpr TaggedRoadIndex(const RoadIndex i)
        : index{i}
    {}
};

using TaggedRoad = TaggedRoadIndex<RoadIndexType::ROAD>;
using TaggedTrail = TaggedRoadIndex<RoadIndexType::TRAIL>;

#endif
