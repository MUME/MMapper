// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CommandId.h"

#include <stdexcept>

#include "../global/enums.h"
#include "../mapdata/ExitDirection.h"

namespace enums {
const std::array<CommandIdType, NUM_COMMANDS> &getAllCommands()
{
    static const auto g_all_commands = genEnumValues<CommandIdType, NUM_COMMANDS>();
    return g_all_commands;
}
} // namespace enums

bool isDirectionNESWUD(const CommandIdType cmd)
{
    switch (cmd) {
    case CommandIdType::NORTH:
    case CommandIdType::SOUTH:
    case CommandIdType::EAST:
    case CommandIdType::WEST:
    case CommandIdType::UP:
    case CommandIdType::DOWN:
        return true;

    case CommandIdType::UNKNOWN:
    case CommandIdType::LOOK:
    case CommandIdType::FLEE:
    case CommandIdType::SCOUT:
    case CommandIdType::NONE:
    default:
        return false;
    }
}
bool isDirection7(const CommandIdType cmd)
{
    switch (cmd) {
    case CommandIdType::NORTH:
    case CommandIdType::SOUTH:
    case CommandIdType::EAST:
    case CommandIdType::WEST:
    case CommandIdType::UP:
    case CommandIdType::DOWN:
    case CommandIdType::UNKNOWN:
        return true;

    case CommandIdType::LOOK:
    case CommandIdType::FLEE:
    case CommandIdType::SCOUT:
    case CommandIdType::NONE:
    default:
        return false;
    }
}
ExitDirection getDirection(const CommandIdType cmd)
{
#define CASE(x) \
    case CommandIdType::x: \
        return ExitDirection::x
    switch (cmd) {
        CASE(NORTH);
        CASE(SOUTH);
        CASE(EAST);
        CASE(WEST);
        CASE(UP);
        CASE(DOWN);
        CASE(UNKNOWN);

    case CommandIdType::LOOK:
    case CommandIdType::FLEE:
    case CommandIdType::SCOUT:
    case CommandIdType::NONE:
    default:
        return ExitDirection::NONE;
    }
#undef CASE
}

const char *getUppercase(const CommandIdType cmd)
{
#define CASE(x) \
    case CommandIdType::x: \
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

const char *getLowercase(const CommandIdType cmd)
{
#define CASE(UPPER, lower) \
    case CommandIdType::UPPER: \
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
