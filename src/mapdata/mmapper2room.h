#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef MMAPPER2ROOM_H
#define MMAPPER2ROOM_H

#include <QtGlobal>

#include "../global/Flags.h"

class Room;

using RoomName = QString;
using RoomDescription = QString;
using RoomNote = QString;

enum class RoomTerrainType {
    UNDEFINED = 0,
    INDOORS,
    CITY,
    FIELD,
    FOREST,
    HILLS,
    MOUNTAINS,
    SHALLOW,
    WATER,
    RAPIDS,
    UNDERWATER,
    ROAD,
    BRUSH,
    TUNNEL,
    CAVERN,
    DEATHTRAP
};
static constexpr const size_t NUM_ROOM_TERRAIN_TYPES = static_cast<size_t>(
                                                           RoomTerrainType::DEATHTRAP)
                                                       + 1u;
static_assert(NUM_ROOM_TERRAIN_TYPES == 16);
DEFINE_ENUM_COUNT(RoomTerrainType, NUM_ROOM_TERRAIN_TYPES)

enum class RoomAlignType { UNDEFINED = 0, GOOD, NEUTRAL, EVIL };
static constexpr const int NUM_ALIGN_TYPES = 4;

// REVISIT: Consider just using a single tri-state bool enums for these?
enum class RoomLightType { UNDEFINED = 0, DARK, LIT };
enum class RoomPortableType { UNDEFINED = 0, PORTABLE, NOT_PORTABLE };
enum class RoomRidableType { UNDEFINED = 0, RIDABLE, NOT_RIDABLE };
enum class RoomSundeathType { UNDEFINED = 0, SUNDEATH, NO_SUNDEATH };
static constexpr const int NUM_LIGHT_TYPES = 3;
static constexpr const int NUM_SUNDEATH_TYPES = 3;
static constexpr const int NUM_PORTABLE_TYPES = 3;
static constexpr const int NUM_RIDABLE_TYPES = 3;

enum class RoomMobFlag {
    RENT,
    SHOP,
    WEAPON_SHOP,
    ARMOUR_SHOP,
    FOOD_SHOP,
    PET_SHOP,
    GUILD,
    SCOUT_GUILD,
    MAGE_GUILD,
    CLERIC_GUILD,
    WARRIOR_GUILD,
    RANGER_GUILD,
    AGGRESSIVE_MOB,
    QUEST_MOB,
    PASSIVE_MOB,
    ELITE_MOB,
    SUPER_MOB,
};
static constexpr const int NUM_ROOM_MOB_FLAGS = static_cast<int>(RoomMobFlag::SUPER_MOB) + 1;
static_assert(NUM_ROOM_MOB_FLAGS == 17);
DEFINE_ENUM_COUNT(RoomMobFlag, NUM_ROOM_MOB_FLAGS)

class RoomMobFlags final : public enums::Flags<RoomMobFlags, RoomMobFlag, uint32_t>
{
    using Flags::Flags;
};

enum class RoomLoadFlag {
    TREASURE,
    ARMOUR,
    WEAPON,
    WATER,
    FOOD,
    HERB,
    KEY,
    MULE,
    HORSE,
    PACK_HORSE,
    TRAINED_HORSE,
    ROHIRRIM,
    WARG,
    BOAT,
    ATTENTION,
    TOWER,
    CLOCK,
    MAIL,
    STABLE,
    WHITE_WORD,
    DARK_WORD,
    EQUIPMENT,
    COACH,
    FERRY
};
static constexpr const int NUM_ROOM_LOAD_FLAGS = static_cast<int>(RoomLoadFlag::FERRY) + 1;
static_assert(NUM_ROOM_LOAD_FLAGS == 24);
DEFINE_ENUM_COUNT(RoomLoadFlag, NUM_ROOM_LOAD_FLAGS)

class RoomLoadFlags final : public enums::Flags<RoomLoadFlags, RoomLoadFlag, uint32_t>
{
    using Flags::Flags;
};

enum class RoomField {
    NAME,
    /** STATIC_DESC */
    DESC,
    TERRAIN_TYPE,
    DYNAMIC_DESC,
    NOTE,
    MOB_FLAGS,
    LOAD_FLAGS,
    PORTABLE_TYPE,
    LIGHT_TYPE,
    ALIGN_TYPE,
    RIDABLE_TYPE,
    SUNDEATH_TYPE,
    RESERVED,
    LAST
};

static constexpr const int NUM_ROOM_FIELDS = static_cast<int>(RoomField::RESERVED) + 1;
static_assert(NUM_ROOM_FIELDS == static_cast<int>(RoomField::LAST));
static_assert(NUM_ROOM_FIELDS == 13);
static constexpr const int NUM_ROOM_PROPS = NUM_ROOM_FIELDS;
DEFINE_ENUM_COUNT(RoomField, NUM_ROOM_FIELDS)
class RoomFields : public enums::Flags<RoomFields, RoomField, uint16_t>
{
    using Flags::Flags;
};

inline constexpr RoomFields operator|(const RoomField lhs, const RoomField rhs) noexcept
{
    return RoomFields{lhs} | RoomFields{rhs};
}

#endif
