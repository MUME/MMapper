// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "ExitDirection.h"

#include "../global/Array.h"
#include "../global/Consts.h"
#include "../global/EnumIndexedArray.h"
#include "../global/enums.h"
#include "coordinate.h"

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

const char *lowercaseDirection(const ExitDirEnum dir)
{
#define X_CASE(UPPER, lower) \
    case ExitDirEnum::UPPER: \
        return #lower
    switch (dir) {
        X_CASE(NORTH, north);
        X_CASE(SOUTH, south);
        X_CASE(EAST, east);
        X_CASE(WEST, west);
        X_CASE(UP, up);
        X_CASE(DOWN, down);
        X_CASE(UNKNOWN, unknown);
    default:
        X_CASE(NONE, none);
    }
#undef X_CASE
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

char charForDir(const ExitDirEnum dir)
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
    return char_consts::C_QUESTION_MARK;
}
} // namespace Mmapper2Exit

using ExitCoordinates = EnumIndexedArray<Coordinate, ExitDirEnum, NUM_EXITS_INCLUDING_NONE>;
NODISCARD static ExitCoordinates initExitCoordinates()
{
    ExitCoordinates exitDirs;
    exitDirs[ExitDirEnum::NORTH] = Coordinate(0, 1, 0);
    exitDirs[ExitDirEnum::SOUTH] = Coordinate(0, -1, 0);
    exitDirs[ExitDirEnum::EAST] = Coordinate(1, 0, 0);
    exitDirs[ExitDirEnum::WEST] = Coordinate(-1, 0, 0);
    exitDirs[ExitDirEnum::UP] = Coordinate(0, 0, 1);
    exitDirs[ExitDirEnum::DOWN] = Coordinate(0, 0, -1);
    return exitDirs;
}

const Coordinate &exitDir(const ExitDirEnum dir)
{
    static const auto exitDirs = initExitCoordinates();
    return exitDirs[dir];
}

const std::string_view to_string_view(const ExitDirEnum dir)
{
    return lowercaseDirection(dir);
}
