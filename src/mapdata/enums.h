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

#ifndef MMAPPER_MAPDATA_ENUMS_H
#define MMAPPER_MAPDATA_ENUMS_H

#include <array>
#include <vector>

#include "DoorFlags.h"
#include "ExitFlags.h"
#include "mmapper2room.h"

#define DECL_GETTER(E, N, name) const std::array<E, N> &name();
#define DECL_GETTER_DEFINED(E, N, name) const std::vector<E> &name();

namespace enums {
DECL_GETTER_DEFINED(RoomLightType, NUM_LIGHT_TYPES, getDefinedRoomLightTypes)
DECL_GETTER_DEFINED(RoomSundeathType, NUM_SUNDEATH_TYPES, getDefinedRoomSundeathTypes)
DECL_GETTER_DEFINED(RoomPortableType, NUM_PORTABLE_TYPES, getDefinedRoomPortbleTypes)
DECL_GETTER_DEFINED(RoomRidableType, NUM_RIDABLE_TYPES, getDefinedRoomRidableTypes)
DECL_GETTER_DEFINED(RoomAlignType, NUM_ALIGN_TYPES, getDefinedRoomAlignTypes)
DECL_GETTER(RoomMobFlag, NUM_ROOM_MOB_FLAGS, getAllMobFlags)
DECL_GETTER(RoomLoadFlag, NUM_ROOM_LOAD_FLAGS, getAllLoadFlags)
DECL_GETTER(DoorFlag, NUM_DOOR_FLAGS, getAllDoorFlags)
DECL_GETTER(ExitFlag, NUM_EXIT_FLAGS, getAllExitFlags)
} // namespace enums

#undef DECL_GETTER
#undef DECL_GETTER_DEFINED

#define ALL_DOOR_FLAGS ::enums::getAllDoorFlags()
#define ALL_EXIT_FLAGS ::enums::getAllExitFlags()
#define ALL_MOB_FLAGS ::enums::getAllMobFlags()
#define ALL_LOAD_FLAGS ::enums::getAllLoadFlags()

/* NOTE: These are actually used; but they're hidden in macros as DEFINED_ROOM_##X##_TYPES */
#define DEFINED_ROOM_LIGHT_TYPES ::enums::getDefinedRoomLightTypes()
#define DEFINED_ROOM_SUNDEATH_TYPES ::enums::getDefinedRoomSundeathTypes()
#define DEFINED_ROOM_PORTABLE_TYPES ::enums::getDefinedRoomPortbleTypes()
#define DEFINED_ROOM_RIDABLE_TYPES ::enums::getDefinedRoomRidableTypes()
#define DEFINED_ROOM_ALIGN_TYPES ::enums::getDefinedRoomAlignTypes()

#endif // MMAPPER_MAPDATA_ENUMS_H
