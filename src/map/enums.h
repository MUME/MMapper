#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Array.h"
#include "DoorFlags.h"
#include "ExitFlags.h"
#include "infomark.h"
#include "mmapper2room.h"

#include <vector>

#define DECL_GETTER(E, N, name) const MMapper::Array<E, N> &name();
#define DECL_GETTER_DEFINED(E, N, name) const std::vector<E> &name();

namespace enums {
DECL_GETTER_DEFINED(RoomLightEnum, NUM_LIGHT_TYPES, getDefinedRoomLightTypes)
DECL_GETTER_DEFINED(RoomSundeathEnum, NUM_SUNDEATH_TYPES, getDefinedRoomSundeathTypes)
DECL_GETTER_DEFINED(RoomPortableEnum, NUM_PORTABLE_TYPES, getDefinedRoomPortbleTypes)
DECL_GETTER_DEFINED(RoomRidableEnum, NUM_RIDABLE_TYPES, getDefinedRoomRidableTypes)
DECL_GETTER_DEFINED(RoomAlignEnum, NUM_ALIGN_TYPES, getDefinedRoomAlignTypes)
DECL_GETTER(RoomTerrainEnum, NUM_ROOM_TERRAIN_TYPES, getAllTerrainTypes)
DECL_GETTER(RoomMobFlagEnum, NUM_ROOM_MOB_FLAGS, getAllMobFlags)
DECL_GETTER(RoomLoadFlagEnum, NUM_ROOM_LOAD_FLAGS, getAllLoadFlags)
DECL_GETTER(DoorFlagEnum, NUM_DOOR_FLAGS, getAllDoorFlags)
DECL_GETTER(ExitFlagEnum, NUM_EXIT_FLAGS, getAllExitFlags)
DECL_GETTER(InfoMarkClassEnum, NUM_INFOMARK_CLASSES, getAllInfoMarkClasses)
DECL_GETTER(InfoMarkTypeEnum, NUM_INFOMARK_TYPES, getAllInfoMarkTypes)
} // namespace enums

#undef DECL_GETTER
#undef DECL_GETTER_DEFINED

#define ALL_TERRAIN_TYPES ::enums::getAllTerrainTypes()
#define ALL_DOOR_FLAGS ::enums::getAllDoorFlags()
#define ALL_EXIT_FLAGS ::enums::getAllExitFlags()
#define ALL_INFOMARK_CLASSES ::enums::getAllInfoMarkClasses()
#define ALL_INFOMARK_TYPES ::enums::getAllInfoMarkTypes()
#define ALL_MOB_FLAGS ::enums::getAllMobFlags()
#define ALL_LOAD_FLAGS ::enums::getAllLoadFlags()

/* NOTE: These are actually used; but they're hidden in macros as DEFINED_ROOM_##X##_TYPES */
#define DEFINED_ROOM_LIGHT_TYPES ::enums::getDefinedRoomLightTypes()
#define DEFINED_ROOM_SUNDEATH_TYPES ::enums::getDefinedRoomSundeathTypes()
#define DEFINED_ROOM_PORTABLE_TYPES ::enums::getDefinedRoomPortbleTypes()
#define DEFINED_ROOM_RIDABLE_TYPES ::enums::getDefinedRoomRidableTypes()
#define DEFINED_ROOM_ALIGN_TYPES ::enums::getDefinedRoomAlignTypes()
