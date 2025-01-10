// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "enums.h"

#include "../global/enums.h"
#include "DoorFlags.h"
#include "ExitFlags.h"
#include "mmapper2room.h"

#include <array>
#include <vector>

#define DEFINE_GETTER(E, N, name) \
    const MMapper::Array<E, N> &name() \
    { \
        static const auto things = ::enums::genEnumValues<E, N>(); \
        return things; \
    }
#define DEFINE_GETTER_DEFINED(E, N, name) \
    const std::vector<E> &name() \
    { \
        static_assert(std::is_enum_v<E>); \
        static const auto things = []() { \
            std::vector<E> result; \
            for (const E x : ::enums::genEnumValues<E, N>()) { \
                if (x != E::UNDEFINED) { \
                    result.emplace_back(x); \
                } \
            } \
            return result; \
        }(); \
        return things; \
    }

namespace enums {
DEFINE_GETTER_DEFINED(RoomLightEnum, NUM_LIGHT_TYPES, getDefinedRoomLightTypes)
DEFINE_GETTER_DEFINED(RoomSundeathEnum, NUM_SUNDEATH_TYPES, getDefinedRoomSundeathTypes)
DEFINE_GETTER_DEFINED(RoomPortableEnum, NUM_PORTABLE_TYPES, getDefinedRoomPortbleTypes)
DEFINE_GETTER_DEFINED(RoomRidableEnum, NUM_RIDABLE_TYPES, getDefinedRoomRidableTypes)
DEFINE_GETTER_DEFINED(RoomAlignEnum, NUM_ALIGN_TYPES, getDefinedRoomAlignTypes)
DEFINE_GETTER(RoomTerrainEnum, NUM_ROOM_TERRAIN_TYPES, getAllTerrainTypes)
DEFINE_GETTER(RoomMobFlagEnum, NUM_ROOM_MOB_FLAGS, getAllMobFlags)
DEFINE_GETTER(RoomLoadFlagEnum, NUM_ROOM_LOAD_FLAGS, getAllLoadFlags)
DEFINE_GETTER(DoorFlagEnum, NUM_DOOR_FLAGS, getAllDoorFlags)
DEFINE_GETTER(ExitFlagEnum, NUM_EXIT_FLAGS, getAllExitFlags)
DEFINE_GETTER(InfoMarkClassEnum, NUM_INFOMARK_CLASSES, getAllInfoMarkClasses)
DEFINE_GETTER(InfoMarkTypeEnum, NUM_INFOMARK_TYPES, getAllInfoMarkTypes)
} // namespace enums

#undef DEFINE_GETTER
#undef DEFINE_GETTER_DEFINED
