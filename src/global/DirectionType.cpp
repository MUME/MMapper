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

#include "DirectionType.h"

#include <array>

#include "../mapdata/ExitDirection.h"
#include "../mapstorage/oldconnection.h"
#include "../parser/CommandId.h"
#include "enums.h"

namespace enums {
const std::array<DirectionType, 6> &getAllDirections6()
{
    static const auto g_all_directions = genEnumValues<DirectionType, 6u>();
    return g_all_directions;
}
} // namespace enums

namespace {
#define TEST(a, b) static_assert(static_cast<int>(a) == (b), "")

// NORTH
TEST(DirectionType::NORTH, 0);
TEST(ExitDirection::NORTH, 0);
TEST(CommandIdType::NORTH, 0);
TEST(ConnectionDirection::NORTH, 0);

// SOUTH
TEST(DirectionType::SOUTH, 1);
TEST(ExitDirection::SOUTH, 1);
TEST(CommandIdType::SOUTH, 1);
TEST(ConnectionDirection::SOUTH, 1);

// EAST
TEST(DirectionType::EAST, 2);
TEST(ExitDirection::EAST, 2);
TEST(CommandIdType::EAST, 2);
TEST(ConnectionDirection::EAST, 2);

// WEST
TEST(DirectionType::WEST, 3);
TEST(ExitDirection::WEST, 3);
TEST(CommandIdType::WEST, 3);
TEST(ConnectionDirection::WEST, 3);

// UP
TEST(DirectionType::UP, 4);
TEST(ExitDirection::UP, 4);
TEST(CommandIdType::UP, 4);
TEST(ConnectionDirection::UP, 4);

// DOWN
TEST(DirectionType::DOWN, 5);
TEST(ExitDirection::DOWN, 5);
TEST(CommandIdType::DOWN, 5);
TEST(ConnectionDirection::DOWN, 5);

// UNKNOWN
TEST(DirectionType::UNKNOWN, 7);
TEST(ExitDirection::UNKNOWN, 6);
TEST(CommandIdType::UNKNOWN, 6);
TEST(ConnectionDirection::UNKNOWN, 6);

// NONE
TEST(DirectionType::NONE, 6);
TEST(ExitDirection::NONE, 7);
TEST(CommandIdType::NONE, 10);
TEST(ConnectionDirection::NONE, 7);

#undef TEST
} // namespace
