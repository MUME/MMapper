// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "DirectionType.h"

#include "Array.h"

#include "../mapdata/ExitDirection.h"
#include "../parser/CommandId.h"
#include "enums.h"

namespace enums {
const MMapper::Array<DirectionEnum, 6> &getAllDirections6()
{
    static const auto g_all_directions = genEnumValues<DirectionEnum, 6u>();
    return g_all_directions;
}
} // namespace enums

namespace {
#define TEST(a, b) static_assert(static_cast<int>(a) == (b), "")

// NORTH
TEST(DirectionEnum::NORTH, 0);
TEST(ExitDirEnum::NORTH, 0);
TEST(CommandEnum::NORTH, 0);

// SOUTH
TEST(DirectionEnum::SOUTH, 1);
TEST(ExitDirEnum::SOUTH, 1);
TEST(CommandEnum::SOUTH, 1);

// EAST
TEST(DirectionEnum::EAST, 2);
TEST(ExitDirEnum::EAST, 2);
TEST(CommandEnum::EAST, 2);

// WEST
TEST(DirectionEnum::WEST, 3);
TEST(ExitDirEnum::WEST, 3);
TEST(CommandEnum::WEST, 3);

// UP
TEST(DirectionEnum::UP, 4);
TEST(ExitDirEnum::UP, 4);
TEST(CommandEnum::UP, 4);

// DOWN
TEST(DirectionEnum::DOWN, 5);
TEST(ExitDirEnum::DOWN, 5);
TEST(CommandEnum::DOWN, 5);

// UNKNOWN
TEST(DirectionEnum::UNKNOWN, 7);
TEST(ExitDirEnum::UNKNOWN, 6);
TEST(CommandEnum::UNKNOWN, 6);

// NONE
TEST(DirectionEnum::NONE, 6);
TEST(ExitDirEnum::NONE, 7);
TEST(CommandEnum::NONE, 10);

#undef TEST
} // namespace
