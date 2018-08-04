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
