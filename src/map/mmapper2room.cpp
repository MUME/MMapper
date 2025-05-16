// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "mmapper2room.h"

#include "../global/Charset.h"
#include "../global/LineUtils.h"
#include "ParseTree.h"
#include "sanitizer.h"

#include <optional>
#include <sstream>

std::string_view to_string_view(const RoomAlignEnum e)
{
#define X_CASE(X) \
    case RoomAlignEnum::X: \
        return #X;
    switch (e) {
        XFOREACH_RoomAlignEnum(X_CASE)
    }
#undef X_CASE
    std::abort();
}

std::string_view to_string_view(const RoomLightEnum e)
{
#define X_CASE(X) \
    case RoomLightEnum::X: \
        return #X;
    switch (e) {
        XFOREACH_RoomLightEnum(X_CASE)
    }
#undef X_CASE
    std::abort();
}

std::string_view to_string_view(const RoomLoadFlagEnum e)
{
#define X_CASE(X) \
    case RoomLoadFlagEnum::X: \
        return #X;
    switch (e) {
        XFOREACH_ROOM_LOAD_FLAG(X_CASE)
    }
#undef X_CASE
    std::abort();
}

std::string_view to_string_view(const RoomMobFlagEnum e)
{
#define X_CASE(X) \
    case RoomMobFlagEnum::X: \
        return #X;
    switch (e) {
        XFOREACH_ROOM_MOB_FLAG(X_CASE)
    }
#undef X_CASE
    std::abort();
}

std::string_view to_string_view(const RoomPortableEnum e)
{
#define X_CASE(X) \
    case RoomPortableEnum::X: \
        return #X;
    switch (e) {
        XFOREACH_RoomPortableEnum(X_CASE)
    }
#undef X_CASE
    std::abort();
}

std::string_view to_string_view(const RoomRidableEnum e)
{
#define X_CASE(X) \
    case RoomRidableEnum::X: \
        return #X;
    switch (e) {
        XFOREACH_RoomRidableEnum(X_CASE)
    }
#undef X_CASE
    std::abort();
}

std::string_view to_string_view(const RoomSundeathEnum e)
{
#define X_CASE(X) \
    case RoomSundeathEnum::X: \
        return #X;
    switch (e) {
        XFOREACH_RoomSundeathEnum(X_CASE)
    }
#undef X_CASE
    std::abort();
}

std::string_view to_string_view(const RoomTerrainEnum e)
{
#define X_CASE(X) \
    case RoomTerrainEnum::X: \
        return #X;
    switch (e) {
        XFOREACH_RoomTerrainEnum(X_CASE)
    }
#undef X_CASE
    std::abort();
}

QString getName(const RoomTerrainEnum terrain)
{
#define X_CASE(UPPER) \
    do { \
    case RoomTerrainEnum::UPPER: \
        return #UPPER; \
    } while (false);

    switch (terrain) {
        XFOREACH_RoomTerrainEnum(X_CASE)
    }
    return QString::asprintf("(RoomTerrainEnum)%d", static_cast<int>(terrain));
#undef X_CASE2
}

QString getName(const RoomMobFlagEnum flag)
{
#define X_CASE2(UPPER, desc) \
    do { \
    case RoomMobFlagEnum::UPPER: \
        return desc; \
    } while (false)
    switch (flag) {
        X_CASE2(RENT, "Rent place");
        X_CASE2(SHOP, "Generic shop");
        X_CASE2(WEAPON_SHOP, "Weapon shop");
        X_CASE2(ARMOUR_SHOP, "Armour shop");
        X_CASE2(FOOD_SHOP, "Food shop");
        X_CASE2(PET_SHOP, "Pet shop");
        X_CASE2(GUILD, "Generic guild");
        X_CASE2(SCOUT_GUILD, "Scout guild");
        X_CASE2(MAGE_GUILD, "Mage guild");
        X_CASE2(CLERIC_GUILD, "Cleric guild");
        X_CASE2(WARRIOR_GUILD, "Warrior guild");
        X_CASE2(RANGER_GUILD, "Ranger guild");
        X_CASE2(AGGRESSIVE_MOB, "Aggressive mob");
        X_CASE2(QUEST_MOB, "Quest mob");
        X_CASE2(PASSIVE_MOB, "Passive mob");
        X_CASE2(ELITE_MOB, "Elite mob");
        X_CASE2(SUPER_MOB, "Super mob");
        X_CASE2(MILKABLE, "Milkable mob");
        X_CASE2(RATTLESNAKE, "Rattlesnake mob");
    }
    return QString::asprintf("(RoomMobFlagEnum)%d", static_cast<int>(flag));
#undef X_CASE2
}

QString getName(const RoomLoadFlagEnum flag)
{
#define X_CASE2(UPPER, desc) \
    do { \
    case RoomLoadFlagEnum::UPPER: \
        return desc; \
    } while (false)
    switch (flag) {
        X_CASE2(TREASURE, "Treasure");
        X_CASE2(ARMOUR, "Armour");
        X_CASE2(WEAPON, "Weapon");
        X_CASE2(WATER, "Water");
        X_CASE2(FOOD, "Food");
        X_CASE2(HERB, "Herb");
        X_CASE2(KEY, "Key");
        X_CASE2(MULE, "Mule");
        X_CASE2(HORSE, "Horse");
        X_CASE2(PACK_HORSE, "Pack horse");
        X_CASE2(TRAINED_HORSE, "Trained horse");
        X_CASE2(ROHIRRIM, "Rohirrim");
        X_CASE2(WARG, "Warg");
        X_CASE2(BOAT, "Boat");
        X_CASE2(ATTENTION, "Attention");
        X_CASE2(TOWER, "Tower");
        X_CASE2(CLOCK, "Clock");
        X_CASE2(MAIL, "Mail");
        X_CASE2(STABLE, "Stable");
        X_CASE2(WHITE_WORD, "White word");
        X_CASE2(DARK_WORD, "Dark word");
        X_CASE2(EQUIPMENT, "Equipment");
        X_CASE2(COACH, "Coach");
        X_CASE2(FERRY, "Ferry");
        X_CASE2(DEATHTRAP, "Deathtrap");
    }
    return QString::asprintf("(RoomLoadFlagEnum)%d", static_cast<int>(flag));
#undef X_CASE2
}

///

NODISCARD static bool isSanitizeRoomName(const std::string_view name)
{
    if (!isValidUtf8(name)) {
        return false;
    }
    return sanitizer::isSanitizedOneLine(name);
}

void sanitizeRoomName(std::string &name)
{
    if (isSanitizeRoomName(name)) {
        return;
    }
    name = sanitizer::sanitizeOneLine(name).getStdStringUtf8();
    assert(isSanitizeRoomName(name));
}

///

static constexpr size_t max_desc_width = 80;

NODISCARD static bool isSanitizedRoomDesc(const std::string_view desc)
{
    if (!isValidUtf8(desc)) {
        return false;
    }
    return sanitizer::isSanitizedWordWraped(desc, max_desc_width);
}

void sanitizeRoomDesc(std::string &desc)
{
    if (isSanitizedRoomDesc(desc)) {
        return;
    }

    desc = sanitizer::sanitizeWordWrapped(std::move(desc), max_desc_width).getStdStringUtf8();
    assert(isSanitizedRoomDesc(desc));
}

///

NODISCARD static bool isSanitizedRoomContents(const std::string_view contents)
{
    if (!isValidUtf8(contents)) {
        return false;
    }
    return sanitizer::isSanitizedMultiline(contents);
}

void sanitizeRoomContents(std::string &contents)
{
    if (isSanitizedRoomContents(contents)) {
        return;
    }
    contents = sanitizer::sanitizeMultiline(contents).getStdStringUtf8();
    assert(isSanitizedRoomContents(contents));
}

///

NODISCARD static bool isSanitizedRoomNote(const std::string_view note)
{
    // Note is permitted to contain any utf8, not just latin1 subset.
    if (!isValidUtf8(note)) {
        return false;
    }
    return sanitizer::isSanitizedUserSupplied(note);
}

void sanitizeRoomNote(std::string &note)
{
    if (isSanitizedRoomNote(note)) {
        return;
    }
    note = sanitizer::sanitizeUserSupplied(note).getStdStringUtf8();
    assert(isSanitizedRoomNote(note));
}

///

RoomName makeRoomName(std::string name)
{
    sanitizeRoomName(name);
    return RoomName{std::move(name)};
}

RoomDesc makeRoomDesc(std::string desc)
{
    sanitizeRoomDesc(desc);
    return RoomDesc{std::move(desc)};
}
RoomContents makeRoomContents(std::string desc)
{
    sanitizeRoomContents(desc);
    return RoomContents{std::move(desc)};
}
RoomNote makeRoomNote(std::string note)
{
    sanitizeRoomNote(note);
    return RoomNote{std::move(note)};
}

namespace mmqt {
RoomName makeRoomName(const QString &name)
{
    return ::makeRoomName(toStdStringUtf8(name));
}
RoomDesc makeRoomDesc(const QString &desc)
{
    return ::makeRoomDesc(toStdStringUtf8(desc));
}
RoomContents makeRoomContents(const QString &desc)
{
    return ::makeRoomContents(toStdStringUtf8(desc));
}
RoomNote makeRoomNote(const QString &note)
{
    return ::makeRoomNote(toStdStringUtf8(note));
}
} // namespace mmqt

namespace tags {
bool RoomNameTag::isValid(const std::string_view sv)
{
    return isSanitizeRoomName(sv);
}
bool RoomDescTag::isValid(const std::string_view sv)
{
    return isSanitizedRoomDesc(sv);
}
bool RoomContentsTag::isValid(const std::string_view sv)
{
    return isSanitizedRoomContents(sv);
}
bool RoomNoteTag::isValid(const std::string_view sv)
{
    return isSanitizedRoomNote(sv);
}
} // namespace tags

namespace { // anonymous

void test_room_descs()
{
    auto make_string = [](const size_t len) -> std::string {
        std::string s(len, static_cast<char>('x'));
        assert(s.size() == len);
        assert(s.front() == 'x');
        assert(s.back() == 'x');
        return s;
    };

    auto wordWrap = [](std::string &s, size_t len) {
        s = sanitizer::sanitizeWordWrapped(std::move(s), len).getStdStringUtf8();
    };

    using char_consts::C_NEWLINE;
    using char_consts::C_SPACE;

    {
        const auto word39 = make_string(39);
        const std::string oneLine = word39 + C_SPACE + word39 + C_NEWLINE;
        const std::string twoLines = word39 + C_NEWLINE + word39 + C_NEWLINE;
        assert(oneLine != twoLines);
        assert(oneLine.size() == twoLines.size());

        std::string longLine = oneLine;
        wordWrap(longLine, 80);
        assert(longLine == oneLine);
    }
    {
        const auto word40 = make_string(40);
        const std::string oneLine = word40 + C_SPACE + word40 + C_NEWLINE;
        const std::string twoLines = word40 + C_NEWLINE + word40 + C_NEWLINE;
        assert(oneLine != twoLines);
        assert(oneLine.size() == twoLines.size());

        std::string longLine = oneLine;
        wordWrap(longLine, 80);
        assert(longLine == twoLines);
    }
    {
        const auto word78 = make_string(78);
        const std::string oneLine = word78 + C_SPACE + 'a' + C_SPACE + word78 + C_NEWLINE;
        const std::string twoLines = word78 + C_SPACE + 'a' + C_NEWLINE + word78 + C_NEWLINE;
        assert(oneLine != twoLines);
        assert(oneLine.size() == twoLines.size());

        std::string longLine = oneLine;
        wordWrap(longLine, 80);
        assert(longLine == twoLines);
    }
    for (size_t len = 79; len <= 81; ++len) {
        const auto word81 = make_string(len);
        const std::string oneLine = word81 + C_SPACE + 'a' + C_SPACE + word81 + C_NEWLINE;
        const std::string threeLines = word81 + C_NEWLINE + 'a' + C_NEWLINE + word81 + C_NEWLINE;
        assert(oneLine != threeLines);
        assert(oneLine.size() == threeLines.size());

        std::string longLine = oneLine;
        wordWrap(longLine, 80);
        assert(longLine == threeLines);
    }
    {
        const std::string input
            = "Though from the outside this alcove looks simple, from within it grows to a\n"
              "cavernous size, growing tall into the rock and furnished with many walkways and\n"
              "terraces. Here, dwarves who have travelled from abroad may claim lodging for\n"
              "the night, resting their backs on the gentle beds of the inn. The atmosphere is\n"
              "relaxed, as if the tension from without cannot penetrate its hallowed stone\n"
              "walls.\n";
        const std::string expect
            = "Though from the outside this alcove looks simple, from within it grows to a\n"
              "cavernous size, growing tall into the rock and furnished with many walkways and\n"
              "terraces. Here, dwarves who have travelled from abroad may claim lodging for the\n"
              "night, resting their backs on the gentle beds of the inn. The atmosphere is\n"
              "relaxed, as if the tension from without cannot penetrate its hallowed stone\n"
              "walls.\n";
        assert(input != expect);
        auto output = input;
        wordWrap(output, 80);
        assert(output == expect);
        assert(sanitizer::isSanitizedMultiline(output));
        assert(sanitizer::isSanitizedWordWraped(output, 80));
    }
}
} // namespace

namespace test {
void test_mmapper2room()
{
    test_room_descs();
}
} // namespace test
