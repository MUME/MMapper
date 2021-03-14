#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <QtGlobal>

#include "../global/Flags.h"
#include "../global/TaggedString.h"

class Room;

namespace tags {
struct NODISCARD RoomNameTag final
{};
struct NODISCARD RoomDynamicDescTag final
{};
struct NODISCARD RoomStaticDescTag final
{};
struct NODISCARD RoomNoteTag final
{};
} // namespace tags

using RoomName = TaggedString<tags::RoomNameTag>;
using RoomDynamicDesc = TaggedString<tags::RoomDynamicDescTag>;
using RoomStaticDesc = TaggedString<tags::RoomStaticDescTag>;
using RoomNote = TaggedString<tags::RoomNoteTag>;

#define X_FOREACH_RoomTerrainEnum(X) \
    X(UNDEFINED) \
    X(INDOORS) \
    X(CITY) \
    X(FIELD) \
    X(FOREST) \
    X(HILLS) \
    X(MOUNTAINS) \
    X(SHALLOW) \
    X(WATER) \
    X(RAPIDS) \
    X(UNDERWATER) \
    X(ROAD) \
    X(BRUSH) \
    X(TUNNEL) \
    X(CAVERN) \
    X(DEATHTRAP)

#define DECL(X) X,
enum class NODISCARD RoomTerrainEnum : uint8_t { X_FOREACH_RoomTerrainEnum(DECL) };
#undef DECL
static_assert(RoomTerrainEnum::UNDEFINED == RoomTerrainEnum{0});
#define ADD(X) +1
static constexpr const size_t NUM_ROOM_TERRAIN_TYPES = (X_FOREACH_RoomTerrainEnum(ADD));
#undef ADD
static_assert(RoomTerrainEnum::DEATHTRAP == RoomTerrainEnum{NUM_ROOM_TERRAIN_TYPES - 1});
static_assert(NUM_ROOM_TERRAIN_TYPES == 16);
DEFINE_ENUM_COUNT(RoomTerrainEnum, NUM_ROOM_TERRAIN_TYPES)

#define X_FOREACH_RoomAlignEnum(X) \
    X(UNDEFINED) \
    X(GOOD) \
    X(NEUTRAL) \
    X(EVIL)
#define DECL(X) X,
enum class NODISCARD RoomAlignEnum : uint8_t { X_FOREACH_RoomAlignEnum(DECL) };
#undef DECL
#define ADD(X) +1
static constexpr const int NUM_ALIGN_TYPES = (X_FOREACH_RoomAlignEnum(ADD));
#undef ADD
static_assert(RoomAlignEnum::UNDEFINED == RoomAlignEnum{0});
static_assert(NUM_ALIGN_TYPES == 4);

#define X_FOREACH_RoomLightEnum(X) \
    X(UNDEFINED) \
    X(DARK) \
    X(LIT)
#define X_FOREACH_RoomPortableEnum(X) \
    X(UNDEFINED) \
    X(PORTABLE) \
    X(NOT_PORTABLE)
#define X_FOREACH_RoomRidableEnum(X) \
    X(UNDEFINED) \
    X(RIDABLE) \
    X(NOT_RIDABLE)
#define X_FOREACH_RoomSundeathEnum(X) \
    X(UNDEFINED) \
    X(SUNDEATH) \
    X(NO_SUNDEATH)

// REVISIT: Consider just using a single tri-state bool enums for these?
#define DECL(X) X,
enum class NODISCARD RoomLightEnum : uint8_t { X_FOREACH_RoomLightEnum(DECL) };
enum class NODISCARD RoomPortableEnum : uint8_t { X_FOREACH_RoomPortableEnum(DECL) };
enum class NODISCARD RoomRidableEnum : uint8_t { X_FOREACH_RoomRidableEnum(DECL) };
enum class NODISCARD RoomSundeathEnum : uint8_t { X_FOREACH_RoomSundeathEnum(DECL) };
#undef DECL

#define ADD(X) +1
static constexpr const int NUM_LIGHT_TYPES = X_FOREACH_RoomLightEnum(ADD);
static constexpr const int NUM_PORTABLE_TYPES = X_FOREACH_RoomPortableEnum(ADD);
static constexpr const int NUM_RIDABLE_TYPES = X_FOREACH_RoomRidableEnum(ADD);
static constexpr const int NUM_SUNDEATH_TYPES = X_FOREACH_RoomSundeathEnum(ADD);
#undef ADD

#define CHECK3(ALL_CAPS, CamelCase) \
    static_assert(Room##CamelCase##Enum::UNDEFINED == Room##CamelCase##Enum{0}); \
    static_assert(NUM_##ALL_CAPS##_TYPES == 3);
CHECK3(LIGHT, Light)
CHECK3(PORTABLE, Portable)
CHECK3(RIDABLE, Ridable)
CHECK3(SUNDEATH, Sundeath)
#undef CHECK3

#define X_FOREACH_ROOM_MOB_FLAG(X) \
    X(RENT) \
    X(SHOP) \
    X(WEAPON_SHOP) \
    X(ARMOUR_SHOP) \
    X(FOOD_SHOP) \
    X(PET_SHOP) \
    X(GUILD) \
    X(SCOUT_GUILD) \
    X(MAGE_GUILD) \
    X(CLERIC_GUILD) \
    X(WARRIOR_GUILD) \
    X(RANGER_GUILD) \
    X(AGGRESSIVE_MOB) \
    X(QUEST_MOB) \
    X(PASSIVE_MOB) \
    X(ELITE_MOB) \
    X(SUPER_MOB)

enum class NODISCARD RoomMobFlagEnum {
#define DECL(X) X,
    X_FOREACH_ROOM_MOB_FLAG(DECL)
#undef DECL
};
#define ADD(X) +1
static constexpr const int NUM_ROOM_MOB_FLAGS = (X_FOREACH_ROOM_MOB_FLAG(ADD));
#undef ADD
static_assert(NUM_ROOM_MOB_FLAGS == 17);
DEFINE_ENUM_COUNT(RoomMobFlagEnum, NUM_ROOM_MOB_FLAGS)

class NODISCARD RoomMobFlags final : public enums::Flags<RoomMobFlags, RoomMobFlagEnum, uint32_t>
{
    using Flags::Flags;
};

#define X_FOREACH_ROOM_LOAD_FLAG(X) \
    X(TREASURE) \
    X(ARMOUR) \
    X(WEAPON) \
    X(WATER) \
    X(FOOD) \
    X(HERB) \
    X(KEY) \
    X(MULE) \
    X(HORSE) \
    X(PACK_HORSE) \
    X(TRAINED_HORSE) \
    X(ROHIRRIM) \
    X(WARG) \
    X(BOAT) \
    X(ATTENTION) \
    X(TOWER) \
    X(CLOCK) \
    X(MAIL) \
    X(STABLE) \
    X(WHITE_WORD) \
    X(DARK_WORD) \
    X(EQUIPMENT) \
    X(COACH) \
    X(FERRY)

enum class NODISCARD RoomLoadFlagEnum {
#define DECL(X) X,
    X_FOREACH_ROOM_LOAD_FLAG(DECL)
#undef DECL
};
static constexpr const int NUM_ROOM_LOAD_FLAGS = static_cast<int>(RoomLoadFlagEnum::FERRY) + 1;
static_assert(NUM_ROOM_LOAD_FLAGS == 24);
DEFINE_ENUM_COUNT(RoomLoadFlagEnum, NUM_ROOM_LOAD_FLAGS)

class NODISCARD RoomLoadFlags final : public enums::Flags<RoomLoadFlags, RoomLoadFlagEnum, uint32_t>
{
    using Flags::Flags;
};

#define X_FOREACH_ROOM_FIELD_ENUM(X) \
    X(NAME) \
    /* Note: DESC could also be called STATIC_DESC */ \
    X(DESC) \
    X(TERRAIN_TYPE) \
    X(DYNAMIC_DESC) \
    X(NOTE) \
    X(MOB_FLAGS) \
    X(LOAD_FLAGS) \
    X(PORTABLE_TYPE) \
    X(LIGHT_TYPE) \
    X(ALIGN_TYPE) \
    X(RIDABLE_TYPE) \
    X(SUNDEATH_TYPE) \
    X(RESERVED)

#define DECL(X) X,
enum class NODISCARD RoomFieldEnum { X_FOREACH_ROOM_FIELD_ENUM(DECL) };
#undef DECL

#define ADD(X) +1
static constexpr const int NUM_ROOM_FIELDS = (X_FOREACH_ROOM_FIELD_ENUM(ADD));
#undef ADD

static_assert(NUM_ROOM_FIELDS == static_cast<int>(RoomFieldEnum::RESERVED) + 1);
static_assert(NUM_ROOM_FIELDS == 13);
DEFINE_ENUM_COUNT(RoomFieldEnum, NUM_ROOM_FIELDS)
class NODISCARD RoomFields final : public enums::Flags<RoomFields, RoomFieldEnum, uint16_t>
{
    using Flags::Flags;
};

NODISCARD inline constexpr RoomFields operator|(const RoomFieldEnum lhs,
                                                const RoomFieldEnum rhs) noexcept
{
    return RoomFields{lhs} | RoomFields{rhs};
}
