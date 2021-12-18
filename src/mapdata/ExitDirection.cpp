// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "ExitDirection.h"

#include "../global/Array.h"
#include "../global/enums.h"
#include "../global/string_view_utils.h"

namespace enums {
const MMapper::Array<ExitDirEnum, NUM_EXITS_NESW> &getAllExitsNESW()
{
    static const auto g_all_exits = genEnumValues<ExitDirEnum, NUM_EXITS_NESW>();
    return g_all_exits;
}
const MMapper::Array<ExitDirEnum, NUM_EXITS_NESWUD> &getAllExitsNESWUD()
{
    static const auto g_all_exits = genEnumValues<ExitDirEnum, NUM_EXITS_NESWUD>();
    return g_all_exits;
}
const MMapper::Array<ExitDirEnum, NUM_EXITS> &getAllExits7()
{
    static const auto g_all_exits = genEnumValues<ExitDirEnum, NUM_EXITS>();
    return g_all_exits;
}

} // namespace enums

bool isNESW(const ExitDirEnum dir)
{
    switch (dir) {
    case ExitDirEnum::NORTH:
    case ExitDirEnum::SOUTH:
    case ExitDirEnum::EAST:
    case ExitDirEnum::WEST:
        return true;

    case ExitDirEnum::UP:
    case ExitDirEnum::DOWN:
    case ExitDirEnum::UNKNOWN:
    case ExitDirEnum::NONE:
    default:
        return false;
    }
}
bool isUpDown(const ExitDirEnum dir)
{
    switch (dir) {
    case ExitDirEnum::UP:
    case ExitDirEnum::DOWN:
        return true;

    case ExitDirEnum::NORTH:
    case ExitDirEnum::SOUTH:
    case ExitDirEnum::EAST:
    case ExitDirEnum::WEST:
    case ExitDirEnum::UNKNOWN:
    case ExitDirEnum::NONE:
    default:
        return false;
    }
}

bool isNESWUD(const ExitDirEnum dir)
{
    switch (dir) {
    case ExitDirEnum::NORTH:
    case ExitDirEnum::SOUTH:
    case ExitDirEnum::EAST:
    case ExitDirEnum::WEST:
    case ExitDirEnum::UP:
    case ExitDirEnum::DOWN:
        return true;

    case ExitDirEnum::UNKNOWN:
    case ExitDirEnum::NONE:
        break;
    }
    return false;
}

// TODO: merge this with other implementations
ExitDirEnum opposite(const ExitDirEnum in)
{
#define PAIR(A, B) \
    do { \
    case ExitDirEnum::A: \
        return ExitDirEnum::B; \
    case ExitDirEnum::B: \
        return ExitDirEnum::A; \
    } while (false)
    switch (in) {
        PAIR(NORTH, SOUTH);
        PAIR(WEST, EAST);
        PAIR(UP, DOWN);
    case ExitDirEnum::UNKNOWN:
    case ExitDirEnum::NONE:
        break;
    }
    return ExitDirEnum::UNKNOWN;
#undef PAIR
}

std::string_view lowercaseDirection(const ExitDirEnum dir)
{
#define CASE(UPPER, lower) \
    case ExitDirEnum::UPPER: \
        return { #lower, sizeof(#lower) - 1 }
    switch (dir) {
        CASE(NORTH, north);
        CASE(SOUTH, south);
        CASE(EAST, east);
        CASE(WEST, west);
        CASE(UP, up);
        CASE(DOWN, down);
        CASE(UNKNOWN, unknown);
    default:
        CASE(NONE, none);
    }
#undef CASE
}

ExitDirEnum directionForLowercase(const std::u16string_view lowcase)
{
    if (!lowcase.empty()) {
        // convert only the first char to dir.
        const ExitDirEnum dir = Mmapper2Exit::dirForChar(static_cast<char>(lowcase[0]));
        // convert back dir to lowercase string, and compare against the whole string
        // we received: faster than an if-else string comparison waterfall.
        //
        // uses mixed-encoding string_view comparison declared in global/string_view_utils.h
        if (lowcase == lowercaseDirection(dir)) {
            return dir;
        }
    }
    return ExitDirEnum::UNKNOWN;
}

namespace Mmapper2Exit {

ExitDirEnum dirForChar(const char dir)
{
    switch (dir) {
    case 'n':
        return ExitDirEnum::NORTH;
    case 's':
        return ExitDirEnum::SOUTH;
    case 'e':
        return ExitDirEnum::EAST;
    case 'w':
        return ExitDirEnum::WEST;
    case 'u':
        return ExitDirEnum::UP;
    case 'd':
        return ExitDirEnum::DOWN;
    default:
        return ExitDirEnum::UNKNOWN;
    }
}

char charForDir(ExitDirEnum dir)
{
    switch (dir) {
    case ExitDirEnum::NORTH:
        return 'n';
    case ExitDirEnum::SOUTH:
        return 's';
    case ExitDirEnum::EAST:
        return 'e';
    case ExitDirEnum::WEST:
        return 'w';
    case ExitDirEnum::UP:
        return 'u';
    case ExitDirEnum::DOWN:
        return 'd';
    case ExitDirEnum::UNKNOWN:
    case ExitDirEnum::NONE:
        break;
    }
    return '?';
}
} // namespace Mmapper2Exit
