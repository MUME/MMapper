// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "enums.h"

#include <array>
#include <vector>

#include "../global/enums.h"
#include "DoorFlags.h"
#include "ExitFlags.h"
#include "mmapper2room.h"

#define DEFINE_GETTER(E, N, name) \
    const std::array<E, N> &name() \
    { \
        static const auto things = ::enums::genEnumValues<E, N>(); \
        return things; \
    }
#define DEFINE_GETTER_DEFINED(E, N, name) \
    const std::vector<E> &name() \
    { \
        static const auto things = []() { \
            std::vector<E> result; \
            for (auto x : ::enums::genEnumValues<E, N>()) \
                if (x != E::UNDEFINED) \
                    result.emplace_back(x); \
            return result; \
        }(); \
        return things; \
    }

namespace enums {
DEFINE_GETTER_DEFINED(RoomLightType, NUM_LIGHT_TYPES, getDefinedRoomLightTypes)
DEFINE_GETTER_DEFINED(RoomSundeathType, NUM_SUNDEATH_TYPES, getDefinedRoomSundeathTypes)
DEFINE_GETTER_DEFINED(RoomPortableType, NUM_PORTABLE_TYPES, getDefinedRoomPortbleTypes)
DEFINE_GETTER_DEFINED(RoomRidableType, NUM_RIDABLE_TYPES, getDefinedRoomRidableTypes)
DEFINE_GETTER_DEFINED(RoomAlignType, NUM_ALIGN_TYPES, getDefinedRoomAlignTypes)
DEFINE_GETTER(RoomTerrainType, NUM_ROOM_TERRAIN_TYPES, getAllTerrainTypes)
DEFINE_GETTER(RoomMobFlag, NUM_ROOM_MOB_FLAGS, getAllMobFlags)
DEFINE_GETTER(RoomLoadFlag, NUM_ROOM_LOAD_FLAGS, getAllLoadFlags)
DEFINE_GETTER(DoorFlag, NUM_DOOR_FLAGS, getAllDoorFlags)
DEFINE_GETTER(ExitFlag, NUM_EXIT_FLAGS, getAllExitFlags)
} // namespace enums

#undef DEFINE_GETTER
#undef DEFINE_GETTER_DEFINED
