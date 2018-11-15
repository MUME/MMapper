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

#ifndef MMAPPER_EXITDIRECTION_H
#define MMAPPER_EXITDIRECTION_H

#include <array>
#include <cstdint>

enum class ExitDirection { NORTH = 0, SOUTH, EAST, WEST, UP, DOWN, UNKNOWN, NONE };

static constexpr const uint32_t NUM_EXITS_NESW = 4u;
static constexpr const uint32_t NUM_EXITS_NESWUD = 6u;
static constexpr const uint32_t NUM_EXITS = 7u;
static constexpr const uint32_t NUM_EXITS_INCLUDING_NONE = 8u;

namespace enums {
const std::array<ExitDirection, NUM_EXITS_NESW> &getAllExitsNESW();
const std::array<ExitDirection, NUM_EXITS_NESWUD> &getAllExitsNESWUD();
const std::array<ExitDirection, NUM_EXITS> &getAllExits7();

#define ALL_EXITS_NESW enums::getAllExitsNESW()
#define ALL_EXITS_NESWUD enums::getAllExitsNESWUD()
#define ALL_EXITS7 enums::getAllExits7()

} // namespace enums

// TODO: move these to another namespace once all of the exit types are merged

bool isNESW(ExitDirection dir);
bool isUpDown(ExitDirection dir);
bool isNESWUD(ExitDirection dir);
ExitDirection opposite(ExitDirection in);
const char *lowercaseDirection(ExitDirection dir);

namespace Mmapper2Exit {

ExitDirection dirForChar(const char dir);

char charForDir(ExitDirection dir);
} // namespace Mmapper2Exit

#endif // MMAPPER_EXITDIRECTION_H
