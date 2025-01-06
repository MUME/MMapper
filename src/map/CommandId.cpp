// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CommandId.h"

#include "../global/enums.h"
#include "ExitDirection.h"

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
#define X_CASE(x) \
    case CommandEnum::x: \
        return ExitDirEnum::x
    switch (cmd) {
        X_CASE(NORTH);
        X_CASE(SOUTH);
        X_CASE(EAST);
        X_CASE(WEST);
        X_CASE(UP);
        X_CASE(DOWN);
        X_CASE(UNKNOWN);

    case CommandEnum::LOOK:
    case CommandEnum::FLEE:
    case CommandEnum::SCOUT:
    case CommandEnum::NONE:
    default:
        return ExitDirEnum::NONE;
    }
#undef X_CASE
}
CommandEnum getCommand(const ExitDirEnum dir)
{
#define X_CASE(x) \
    case ExitDirEnum::x: \
        return CommandEnum::x
    switch (dir) {
        X_CASE(NORTH);
        X_CASE(SOUTH);
        X_CASE(EAST);
        X_CASE(WEST);
        X_CASE(UP);
        X_CASE(DOWN);
        X_CASE(UNKNOWN);
        X_CASE(NONE);

    default:
        return CommandEnum::LOOK;
    }
#undef X_CASE
}

const char *getUppercase(const CommandEnum cmd)
{
#define X_CASE(x) \
    case CommandEnum::x: \
        return #x
    switch (cmd) {
        X_CASE(NORTH);
        X_CASE(SOUTH);
        X_CASE(EAST);
        X_CASE(WEST);
        X_CASE(UP);
        X_CASE(DOWN);
        X_CASE(UNKNOWN);
        X_CASE(LOOK);
        X_CASE(FLEE);
        X_CASE(SCOUT);
        X_CASE(NONE);
    }
#undef X_CASE

    throw std::runtime_error("missing name for command");
}

const char *getLowercase(const CommandEnum cmd)
{
#define X_CASE(UPPER, lower) \
    case CommandEnum::UPPER: \
        return #lower
    switch (cmd) {
        X_CASE(NORTH, north);
        X_CASE(SOUTH, south);
        X_CASE(EAST, east);
        X_CASE(WEST, west);
        X_CASE(UP, up);
        X_CASE(DOWN, down);
        X_CASE(UNKNOWN, unknown);
        X_CASE(LOOK, look);
        X_CASE(FLEE, flee);
        X_CASE(SCOUT, scout);
        X_CASE(NONE, none);
    }
#undef X_CASE

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
