#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
static_assert(static_cast<int>(RoadIndex::ALL) == 15);
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
static_assert(~RoadIndex::ALL == RoadIndex::NONE);
static_assert(~RoadIndex::NONE == RoadIndex::ALL);

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
