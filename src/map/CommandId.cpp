// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CommandId.h"

#include "../global/enums.h"
#include "ExitDirection.h"

#include <array>
#include <stdexcept>

namespace enums {
const MMapper::Array<CommandEnum, NUM_COMMANDS> &getAllCommands()
{
    static const auto g_all_commands = genEnumValues<CommandEnum, NUM_COMMANDS>();
    return g_all_commands;
}
} // namespace enums

bool isDirectionNESWUD(const CommandEnum cmd)
{
    switch (cmd) {
    case CommandEnum::NORTH:
    case CommandEnum::SOUTH:
    case CommandEnum::EAST:
    case CommandEnum::WEST:
    case CommandEnum::UP:
    case CommandEnum::DOWN:
        return true;

    case CommandEnum::UNKNOWN:
    case CommandEnum::LOOK:
    case CommandEnum::FLEE:
    case CommandEnum::SCOUT:
    case CommandEnum::NONE:
    default:
        return false;
    }
}
bool isDirection7(const CommandEnum cmd)
{
    switch (cmd) {
    case CommandEnum::NORTH:
    case CommandEnum::SOUTH:
    case CommandEnum::EAST:
    case CommandEnum::WEST:
    case CommandEnum::UP:
    case CommandEnum::DOWN:
    case CommandEnum::UNKNOWN:
        return true;

    case CommandEnum::LOOK:
    case CommandEnum::FLEE:
    case CommandEnum::SCOUT:
    case CommandEnum::NONE:
    default:
        return false;
    }
}
ExitDirEnum getDirection(const CommandEnum cmd)
{
#define CASE(x) \
    case CommandEnum::x: \
        return ExitDirEnum::x
    switch (cmd) {
        CASE(NORTH);
        CASE(SOUTH);
        CASE(EAST);
        CASE(WEST);
        CASE(UP);
        CASE(DOWN);
        CASE(UNKNOWN);

    case CommandEnum::LOOK:
    case CommandEnum::FLEE:
    case CommandEnum::SCOUT:
    case CommandEnum::NONE:
    default:
        return ExitDirEnum::NONE;
    }
#undef CASE
}
CommandEnum getCommand(const ExitDirEnum dir)
{
#define CASE(x) \
    case ExitDirEnum::x: \
        return CommandEnum::x
    switch (dir) {
        CASE(NORTH);
        CASE(SOUTH);
        CASE(EAST);
        CASE(WEST);
        CASE(UP);
        CASE(DOWN);
        CASE(UNKNOWN);
        CASE(NONE);

    default:
        return CommandEnum::LOOK;
    }
#undef CASE
}

const char *getUppercase(const CommandEnum cmd)
{
#define CASE(x) \
    case CommandEnum::x: \
        return #x
    switch (cmd) {
        CASE(NORTH);
        CASE(SOUTH);
        CASE(EAST);
        CASE(WEST);
        CASE(UP);
        CASE(DOWN);
        CASE(UNKNOWN);
        CASE(LOOK);
        CASE(FLEE);
        CASE(SCOUT);
        CASE(NONE);
    }
#undef CASE

    throw std::runtime_error("missing name for command");
}

const char *getLowercase(const CommandEnum cmd)
{
#define CASE(UPPER, lower) \
    case CommandEnum::UPPER: \
        return #lower
    switch (cmd) {
        CASE(NORTH, north);
        CASE(SOUTH, south);
        CASE(EAST, east);
        CASE(WEST, west);
        CASE(UP, up);
        CASE(DOWN, down);
        CASE(UNKNOWN, unknown);
        CASE(LOOK, look);
        CASE(FLEE, flee);
        CASE(SCOUT, scout);
        CASE(NONE, none);
    }
#undef CASE

    throw std::runtime_error("missing name for command");
}

namespace { // anonymous
#define TEST(a, b) static_assert(static_cast<int>(a) == (b), "")

// NORTH
TEST(ExitDirEnum::NORTH, 0);
TEST(CommandEnum::NORTH, 0);

// SOUTH
TEST(ExitDirEnum::SOUTH, 1);
TEST(CommandEnum::SOUTH, 1);

// EAST
TEST(ExitDirEnum::EAST, 2);
TEST(CommandEnum::EAST, 2);

// WEST
TEST(ExitDirEnum::WEST, 3);
TEST(CommandEnum::WEST, 3);

// UP
TEST(ExitDirEnum::UP, 4);
TEST(CommandEnum::UP, 4);

// DOWN
TEST(ExitDirEnum::DOWN, 5);
TEST(CommandEnum::DOWN, 5);

// UNKNOWN
TEST(ExitDirEnum::UNKNOWN, 6);
TEST(CommandEnum::UNKNOWN, 6);

// NONE
TEST(ExitDirEnum::NONE, 7);
TEST(CommandEnum::NONE, 10);

#undef TEST
} // namespace
