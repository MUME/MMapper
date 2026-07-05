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

ENUM_ALLOWS_BIT_OPS_WITH_WIDTH(RoadIndexMaskEnum, 4)

NODISCARD RoadIndexMaskEnum getRoadIndex(ExitDirEnum dir);
NODISCARD RoadIndexMaskEnum getRoadIndex(const RawRoom &room);

enum class NODISCARD RoadTagEnum : uint8_t { ROAD, TRAIL };
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
