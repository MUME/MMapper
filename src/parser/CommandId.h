#pragma once
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

#ifndef MMAPPER_COMMANDID_H
#define MMAPPER_COMMANDID_H

#include <array>

#include "../mapdata/ExitDirection.h"

enum class CommandIdType {
    NORTH = 0,
    SOUTH,
    EAST,
    WEST,
    UP,
    DOWN,
    UNKNOWN,
    LOOK,
    FLEE,
    SCOUT,
    /*SYNC, RESET, */
    NONE
};
/* does not include NONE */
static constexpr const int NUM_COMMANDS = 10;

namespace enums {

#define ALL_COMMANDS ::enums::getAllCommands()
const std::array<CommandIdType, NUM_COMMANDS> &getAllCommands();

} // namespace enums

bool isDirectionNESWUD(const CommandIdType cmd);
bool isDirection7(const CommandIdType cmd);
ExitDirection getDirection(const CommandIdType cmd);

const char *getUppercase(const CommandIdType cmd);
const char *getLowercase(const CommandIdType cmd);

#endif //MMAPPER_COMMANDID_H
