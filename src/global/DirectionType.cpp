// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "DirectionType.h"

#include <array>

#include "../mapdata/ExitDirection.h"
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

// SOUTH
TEST(DirectionType::SOUTH, 1);
TEST(ExitDirection::SOUTH, 1);
TEST(CommandIdType::SOUTH, 1);

// EAST
TEST(DirectionType::EAST, 2);
TEST(ExitDirection::EAST, 2);
TEST(CommandIdType::EAST, 2);

// WEST
TEST(DirectionType::WEST, 3);
TEST(ExitDirection::WEST, 3);
TEST(CommandIdType::WEST, 3);

// UP
TEST(DirectionType::UP, 4);
TEST(ExitDirection::UP, 4);
TEST(CommandIdType::UP, 4);

// DOWN
TEST(DirectionType::DOWN, 5);
TEST(ExitDirection::DOWN, 5);
TEST(CommandIdType::DOWN, 5);

// UNKNOWN
TEST(DirectionType::UNKNOWN, 7);
TEST(ExitDirection::UNKNOWN, 6);
TEST(CommandIdType::UNKNOWN, 6);

// NONE
TEST(DirectionType::NONE, 6);
TEST(ExitDirection::NONE, 7);
TEST(CommandIdType::NONE, 10);

#undef TEST
} // namespace
