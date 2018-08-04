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

#include "ExitDirection.h"

#include <array>

#include "../global/enums.h"

namespace enums {
const std::array<ExitDirection, NUM_EXITS_NESW> &getAllExitsNESW()
{
    static const auto g_all_exits = genEnumValues<ExitDirection, NUM_EXITS_NESW>();
    return g_all_exits;
}
const std::array<ExitDirection, NUM_EXITS_NESWUD> &getAllExitsNESWUD()
{
    static const auto g_all_exits = genEnumValues<ExitDirection, NUM_EXITS_NESWUD>();
    return g_all_exits;
}
const std::array<ExitDirection, NUM_EXITS> &getAllExits7()
{
    static const auto g_all_exits = genEnumValues<ExitDirection, NUM_EXITS>();
    return g_all_exits;
}

} // namespace enums

bool isNESW(const ExitDirection dir)
{
    switch (dir) {
    case ExitDirection::NORTH:
    case ExitDirection::SOUTH:
    case ExitDirection::EAST:
    case ExitDirection::WEST:
        return true;
    default:
        return false;
    }
}
bool isUpDown(const ExitDirection dir)
{
    switch (dir) {
    case ExitDirection::UP:
    case ExitDirection::DOWN:
        return true;
    default:
        return false;
    }
}

bool isNESWUD(const ExitDirection dir)
{
    switch (dir) {
    case ExitDirection::NORTH:
    case ExitDirection::SOUTH:
    case ExitDirection::EAST:
    case ExitDirection::WEST:
    case ExitDirection::UP:
    case ExitDirection::DOWN:
        return true;
    default:
        return false;
    }
}

// TODO: merge this with other implementations
ExitDirection opposite(const ExitDirection in)
{
#define PAIR(A, B) \
    do { \
    case ExitDirection::A: \
        return ExitDirection::B; \
    case ExitDirection::B: \
        return ExitDirection::A; \
    } while (false)
    switch (in) {
        PAIR(NORTH, SOUTH);
        PAIR(WEST, EAST);
        PAIR(UP, DOWN);
    default:
        return ExitDirection::UNKNOWN;
    }
#undef PAIR
}

const char *lowercaseDirection(const ExitDirection dir)
{
#define CASE(UPPER, lower) \
    case ExitDirection::UPPER: \
        return #lower
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

namespace Mmapper2Exit {

ExitDirection dirForChar(const char dir)
{
    switch (dir) {
    case 'n':
        return ExitDirection::NORTH;
    case 's':
        return ExitDirection::SOUTH;
    case 'e':
        return ExitDirection::EAST;
    case 'w':
        return ExitDirection::WEST;
    case 'u':
        return ExitDirection::UP;
    case 'd':
        return ExitDirection::DOWN;
    default:
        return ExitDirection::UNKNOWN;
    }
}

char charForDir(ExitDirection dir)
{
    switch (dir) {
    case ExitDirection::NORTH:
        return 'n';
    case ExitDirection::SOUTH:
        return 's';
    case ExitDirection::EAST:
        return 'e';
    case ExitDirection::WEST:
        return 'w';
    case ExitDirection::UP:
        return 'u';
    case ExitDirection::DOWN:
        return 'd';
    default:
        return '?';
    }
}
} // namespace Mmapper2Exit
