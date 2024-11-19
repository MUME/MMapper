#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Flags.h"
#include "../map/ExitDirection.h"
#include "../map/RoomHandle.h"

#include <cstdint>

static constexpr const int NUM_COMPASS_DIRS = 4;
static constexpr const int NUM_ROAD_INDICES = 1 << NUM_COMPASS_DIRS;

enum class NODISCARD RoadIndexMaskEnum : uint32_t {
#define BIT_FROM_EXDIR(x) x = (1 << static_cast<int>(ExitDirEnum::x))
    NONE = 0,
    BIT_FROM_EXDIR(NORTH),
    BIT_FROM_EXDIR(SOUTH),
    BIT_FROM_EXDIR(EAST),
    BIT_FROM_EXDIR(WEST),
    ALL = NUM_ROAD_INDICES - 1
#undef BIT_FROM_EXDIR
};
static_assert(static_cast<int>(RoadIndexMaskEnum::ALL) == 15);
DEFINE_ENUM_COUNT(RoadIndexMaskEnum, NUM_ROAD_INDICES)

NODISCARD inline constexpr RoadIndexMaskEnum operator|(const RoadIndexMaskEnum lhs,
                                                       const RoadIndexMaskEnum rhs) noexcept
{
    return static_cast<RoadIndexMaskEnum>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}
inline constexpr RoadIndexMaskEnum &operator|=(RoadIndexMaskEnum &lhs,
                                               const RoadIndexMaskEnum rhs) noexcept
{
    return lhs = (lhs | rhs);
}
NODISCARD inline constexpr RoadIndexMaskEnum operator&(const RoadIndexMaskEnum lhs,
                                                       const RoadIndexMaskEnum rhs)
{
    return static_cast<RoadIndexMaskEnum>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}
NODISCARD inline constexpr RoadIndexMaskEnum operator~(const RoadIndexMaskEnum x)
{
    return static_cast<RoadIndexMaskEnum>(static_cast<uint32_t>(x)
                                          ^ static_cast<uint32_t>(RoadIndexMaskEnum::ALL));
}
static_assert(~RoadIndexMaskEnum::ALL == RoadIndexMaskEnum::NONE);
static_assert(~RoadIndexMaskEnum::NONE == RoadIndexMaskEnum::ALL);

NODISCARD RoadIndexMaskEnum getRoadIndex(ExitDirEnum dir);
NODISCARD RoadIndexMaskEnum getRoadIndex(const RawRoom &room);

enum class NODISCARD RoadTagEnum { ROAD, TRAIL };
template<RoadTagEnum Tag>
struct NODISCARD TaggedRoadIndex final
{
    static constexpr const RoadTagEnum tag_type = Tag;
    const RoadIndexMaskEnum index;
    explicit constexpr TaggedRoadIndex(const RoadIndexMaskEnum i)
        : index{i}
    {}
};

using TaggedRoad = TaggedRoadIndex<RoadTagEnum::ROAD>;
using TaggedTrail = TaggedRoadIndex<RoadTagEnum::TRAIL>;
