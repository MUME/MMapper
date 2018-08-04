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

#include "enums.h"

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
DEFINE_GETTER(RoomMobFlag, NUM_ROOM_MOB_FLAGS, getAllMobFlags)
DEFINE_GETTER(RoomLoadFlag, NUM_ROOM_LOAD_FLAGS, getAllLoadFlags)
DEFINE_GETTER(DoorFlag, NUM_DOOR_FLAGS, getAllDoorFlags)
DEFINE_GETTER(ExitFlag, NUM_EXIT_FLAGS, getAllExitFlags)
} // namespace enums

#undef DEFINE_GETTER
#undef DEFINE_GETTER_DEFINED
