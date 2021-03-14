#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <QtGlobal>

#include "../global/Flags.h"
#include "../global/TaggedString.h"

class Room;

struct RoomNameTag final
{};
struct RoomDynamicDescTag final
{};
struct RoomStaticDescTag final
{};
struct RoomNoteTag final
{};

using RoomName = TaggedString<RoomNameTag>;
using RoomDynamicDesc = TaggedString<RoomDynamicDescTag>;
using RoomStaticDesc = TaggedString<RoomStaticDescTag>;
using RoomNote = TaggedString<RoomNoteTag>;

enum class RoomTerrainEnum {
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
                                                           RoomTerrainEnum::DEATHTRAP)
                                                       + 1u;
static_assert(NUM_ROOM_TERRAIN_TYPES == 16);
DEFINE_ENUM_COUNT(RoomTerrainEnum, NUM_ROOM_TERRAIN_TYPES)

enum class RoomAlignEnum { UNDEFINED = 0, GOOD, NEUTRAL, EVIL };
static constexpr const int NUM_ALIGN_TYPES = 4;

// REVISIT: Consider just using a single tri-state bool enums for these?
enum class RoomLightEnum { UNDEFINED = 0, DARK, LIT };
enum class RoomPortableEnum { UNDEFINED = 0, PORTABLE, NOT_PORTABLE };
enum class RoomRidableEnum { UNDEFINED = 0, RIDABLE, NOT_RIDABLE };
enum class RoomSundeathEnum { UNDEFINED = 0, SUNDEATH, NO_SUNDEATH };
static constexpr const int NUM_LIGHT_TYPES = 3;
static constexpr const int NUM_SUNDEATH_TYPES = 3;
static constexpr const int NUM_PORTABLE_TYPES = 3;
static constexpr const int NUM_RIDABLE_TYPES = 3;

enum class RoomMobFlagEnum {
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
static constexpr const int NUM_ROOM_MOB_FLAGS = static_cast<int>(RoomMobFlagEnum::SUPER_MOB) + 1;
static_assert(NUM_ROOM_MOB_FLAGS == 17);
DEFINE_ENUM_COUNT(RoomMobFlagEnum, NUM_ROOM_MOB_FLAGS)

class RoomMobFlags final : public enums::Flags<RoomMobFlags, RoomMobFlagEnum, uint32_t>
{
    using Flags::Flags;
};

enum class RoomLoadFlagEnum {
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
static constexpr const int NUM_ROOM_LOAD_FLAGS = static_cast<int>(RoomLoadFlagEnum::FERRY) + 1;
static_assert(NUM_ROOM_LOAD_FLAGS == 24);
DEFINE_ENUM_COUNT(RoomLoadFlagEnum, NUM_ROOM_LOAD_FLAGS)

class RoomLoadFlags final : public enums::Flags<RoomLoadFlags, RoomLoadFlagEnum, uint32_t>
{
    using Flags::Flags;
};

enum class RoomFieldEnum {
    NAME,
    /* Note: DESC could also be called STATIC_DESC */
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

static constexpr const int NUM_ROOM_FIELDS = static_cast<int>(RoomFieldEnum::RESERVED) + 1;
static_assert(NUM_ROOM_FIELDS == static_cast<int>(RoomFieldEnum::LAST));
static_assert(NUM_ROOM_FIELDS == 13);
static constexpr const int NUM_ROOM_PROPS = NUM_ROOM_FIELDS;
DEFINE_ENUM_COUNT(RoomFieldEnum, NUM_ROOM_FIELDS)
class RoomFields final : public enums::Flags<RoomFields, RoomFieldEnum, uint16_t>
{
    using Flags::Flags;
};

inline constexpr RoomFields operator|(const RoomFieldEnum lhs, const RoomFieldEnum rhs) noexcept
{
    return RoomFields{lhs} | RoomFields{rhs};
}
