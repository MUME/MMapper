#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/Flags.h"
#include "../global/TaggedString.h"

#include <QtGlobal>

namespace tags {
struct NODISCARD RoomNameTag final
{
    NODISCARD static bool isValid(std::string_view sv);
};
struct NODISCARD RoomDescTag final
{
    NODISCARD static bool isValid(std::string_view sv);
};
struct NODISCARD RoomContentsTag final
{
    NODISCARD static bool isValid(std::string_view sv);
};
struct NODISCARD RoomNoteTag final
{
    NODISCARD static bool isValid(std::string_view sv);
};
} // namespace tags

using RoomName = TaggedBoxedStringUtf8<tags::RoomNameTag>;
using RoomDesc = TaggedBoxedStringUtf8<tags::RoomDescTag>;
using RoomContents = TaggedBoxedStringUtf8<tags::RoomContentsTag>;
using RoomNote = TaggedBoxedStringUtf8<tags::RoomNoteTag>;

#define XFOREACH_RoomTerrainEnum(X) \
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
    X(CAVERN)

#define X_DECL(X) X,
enum class NODISCARD RoomTerrainEnum : uint8_t { XFOREACH_RoomTerrainEnum(X_DECL) };
#undef X_DECL
static_assert(RoomTerrainEnum::UNDEFINED == RoomTerrainEnum{0});
#define X_ADD(X) +1
static constexpr const size_t NUM_ROOM_TERRAIN_TYPES = (XFOREACH_RoomTerrainEnum(X_ADD));
#undef X_ADD
static_assert(RoomTerrainEnum::CAVERN == RoomTerrainEnum{NUM_ROOM_TERRAIN_TYPES - 1});
static_assert(NUM_ROOM_TERRAIN_TYPES == 15);
DEFINE_ENUM_COUNT(RoomTerrainEnum, NUM_ROOM_TERRAIN_TYPES)

#define XFOREACH_RoomAlignEnum(X) \
    X(UNDEFINED) \
    X(GOOD) \
    X(NEUTRAL) \
    X(EVIL)
#define X_DECL(X) X,
enum class NODISCARD RoomAlignEnum : uint8_t { XFOREACH_RoomAlignEnum(X_DECL) };
#undef X_DECL
#define X_ADD(X) +1
static constexpr const int NUM_ALIGN_TYPES = (XFOREACH_RoomAlignEnum(X_ADD));
#undef X_ADD
static_assert(RoomAlignEnum::UNDEFINED == RoomAlignEnum{0});
static_assert(NUM_ALIGN_TYPES == 4);

#define XFOREACH_RoomLightEnum(X) \
    X(UNDEFINED) \
    X(DARK) \
    X(LIT)
#define XFOREACH_RoomPortableEnum(X) \
    X(UNDEFINED) \
    X(PORTABLE) \
    X(NOT_PORTABLE)
#define XFOREACH_RoomRidableEnum(X) \
    X(UNDEFINED) \
    X(RIDABLE) \
    X(NOT_RIDABLE)
#define XFOREACH_RoomSundeathEnum(X) \
    X(UNDEFINED) \
    X(SUNDEATH) \
    X(NO_SUNDEATH)

// REVISIT: Consider just using a single tri-state bool enums for these?
#define X_DECL(X) X,
enum class NODISCARD RoomLightEnum : uint8_t { XFOREACH_RoomLightEnum(X_DECL) };
enum class NODISCARD RoomPortableEnum : uint8_t { XFOREACH_RoomPortableEnum(X_DECL) };
enum class NODISCARD RoomRidableEnum : uint8_t { XFOREACH_RoomRidableEnum(X_DECL) };
enum class NODISCARD RoomSundeathEnum : uint8_t { XFOREACH_RoomSundeathEnum(X_DECL) };
#undef X_DECL

#define X_ADD(X) +1
static constexpr const int NUM_LIGHT_TYPES = XFOREACH_RoomLightEnum(X_ADD);
static constexpr const int NUM_PORTABLE_TYPES = XFOREACH_RoomPortableEnum(X_ADD);
static constexpr const int NUM_RIDABLE_TYPES = XFOREACH_RoomRidableEnum(X_ADD);
static constexpr const int NUM_SUNDEATH_TYPES = XFOREACH_RoomSundeathEnum(X_ADD);
#undef X_ADD

DEFINE_ENUM_COUNT(RoomAlignEnum, NUM_ALIGN_TYPES)
DEFINE_ENUM_COUNT(RoomLightEnum, NUM_LIGHT_TYPES)
DEFINE_ENUM_COUNT(RoomPortableEnum, NUM_PORTABLE_TYPES)
DEFINE_ENUM_COUNT(RoomRidableEnum, NUM_RIDABLE_TYPES)
DEFINE_ENUM_COUNT(RoomSundeathEnum, NUM_SUNDEATH_TYPES)

#define CHECK3(ALL_CAPS, CamelCase) \
    static_assert(Room##CamelCase##Enum::UNDEFINED == Room##CamelCase##Enum{0}); \
    static_assert(NUM_##ALL_CAPS##_TYPES == 3);
CHECK3(LIGHT, Light)
CHECK3(PORTABLE, Portable)
CHECK3(RIDABLE, Ridable)
CHECK3(SUNDEATH, Sundeath)
#undef CHECK3

#define XFOREACH_ROOM_MOB_FLAG(X) \
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
    X(SUPER_MOB) \
    X(MILKABLE) \
    X(RATTLESNAKE)

enum class NODISCARD RoomMobFlagEnum : uint8_t {
#define X_DECL(X) X,
    XFOREACH_ROOM_MOB_FLAG(X_DECL)
#undef X_DECL
};
#define X_ADD(X) +1
static constexpr const int NUM_ROOM_MOB_FLAGS = (XFOREACH_ROOM_MOB_FLAG(X_ADD));
#undef X_ADD
static_assert(NUM_ROOM_MOB_FLAGS == 19);
DEFINE_ENUM_COUNT(RoomMobFlagEnum, NUM_ROOM_MOB_FLAGS)

class NODISCARD RoomMobFlags final : public enums::Flags<RoomMobFlags, RoomMobFlagEnum, uint32_t>
{
    using Flags::Flags;
};

#define XFOREACH_ROOM_LOAD_FLAG(X) \
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
    X(FERRY) \
    X(DEATHTRAP)

enum class NODISCARD RoomLoadFlagEnum : uint8_t {
#define X_DECL(X) X,
    XFOREACH_ROOM_LOAD_FLAG(X_DECL)
#undef X_DECL
};
static constexpr const int NUM_ROOM_LOAD_FLAGS = static_cast<int>(RoomLoadFlagEnum::DEATHTRAP) + 1;
static_assert(NUM_ROOM_LOAD_FLAGS == 25);
DEFINE_ENUM_COUNT(RoomLoadFlagEnum, NUM_ROOM_LOAD_FLAGS)

class NODISCARD RoomLoadFlags final : public enums::Flags<RoomLoadFlags, RoomLoadFlagEnum, uint32_t>
{
    using Flags::Flags;
};

// REVISIT: Why are these in a different order than the other XMACRO(), and why don't we just use that one?
#define XFOREACH_ROOM_FIELD_ENUM(X) \
    X(NAME) \
    /* Note: DESC could also be called STATIC_DESC */ \
    X(DESC) \
    X(TERRAIN_TYPE) \
    X(CONTENTS) \
    X(NOTE) \
    X(MOB_FLAGS) \
    X(LOAD_FLAGS) \
    X(PORTABLE_TYPE) \
    X(LIGHT_TYPE) \
    X(ALIGN_TYPE) \
    X(RIDABLE_TYPE) \
    X(SUNDEATH_TYPE) \
    X(RESERVED)

#define X_DECL(X) X,
enum class NODISCARD RoomFieldEnum { XFOREACH_ROOM_FIELD_ENUM(X_DECL) };
#undef X_DECL

#define X_ADD(X) +1
static constexpr const int NUM_ROOM_FIELDS = (XFOREACH_ROOM_FIELD_ENUM(X_ADD));
#undef X_ADD

static_assert(NUM_ROOM_FIELDS == static_cast<int>(RoomFieldEnum::RESERVED) + 1);
static_assert(NUM_ROOM_FIELDS == 13);
DEFINE_ENUM_COUNT(RoomFieldEnum, NUM_ROOM_FIELDS)

class NODISCARD RoomFieldFlags final : public enums::Flags<RoomFieldFlags, RoomFieldEnum, uint16_t>
{
    using Flags::Flags;
};

NODISCARD inline constexpr RoomFieldFlags operator|(const RoomFieldEnum lhs,
                                                    const RoomFieldEnum rhs) noexcept
{
    return RoomFieldFlags{lhs} | RoomFieldFlags{rhs};
}

template<>
struct std::hash<RoomName>
{
    NODISCARD std::size_t operator()(const RoomName &name) const noexcept
    {
        return std::hash<std::string_view>()(name.getStdStringViewUtf8());
    }
};
template<>
struct std::hash<RoomDesc>
{
    NODISCARD std::size_t operator()(const RoomDesc &desc) const noexcept
    {
        return std::hash<std::string_view>()(desc.getStdStringViewUtf8());
    }
};

NODISCARD extern std::string_view to_string_view(RoomAlignEnum);
NODISCARD extern std::string_view to_string_view(RoomLightEnum);
NODISCARD extern std::string_view to_string_view(RoomLoadFlagEnum);
NODISCARD extern std::string_view to_string_view(RoomMobFlagEnum);
NODISCARD extern std::string_view to_string_view(RoomPortableEnum);
NODISCARD extern std::string_view to_string_view(RoomRidableEnum);
NODISCARD extern std::string_view to_string_view(RoomSundeathEnum);
NODISCARD extern std::string_view to_string_view(RoomTerrainEnum);

/* REVISIT: merge these with human-readable names used in parser output? */
NODISCARD extern QString getName(RoomLoadFlagEnum flag);
NODISCARD extern QString getName(RoomMobFlagEnum flag);
NODISCARD extern QString getName(RoomTerrainEnum terrain);

void sanitizeRoomName(std::string &name);
void sanitizeRoomDesc(std::string &desc);
void sanitizeRoomContents(std::string &contents);
void sanitizeRoomNote(std::string &note);

NODISCARD extern RoomName makeRoomName(std::string name);
NODISCARD extern RoomDesc makeRoomDesc(std::string desc);
NODISCARD extern RoomContents makeRoomContents(std::string desc);
NODISCARD extern RoomNote makeRoomNote(std::string note);

namespace mmqt {
NODISCARD extern RoomName makeRoomName(QString name);
NODISCARD extern RoomDesc makeRoomDesc(QString desc);
NODISCARD extern RoomContents makeRoomContents(QString desc);
NODISCARD extern RoomNote makeRoomNote(QString note);
} // namespace mmqt

namespace test {
extern void test_mmapper2room();
} // namespace test
