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

#ifndef MMAPPER_DOORACTION_H
#define MMAPPER_DOORACTION_H

#include <array>
#include <cstddef>

enum class DoorActionType {
    OPEN,
    CLOSE,
    LOCK,
    UNLOCK,
    PICK,
    ROCK,
    BASH,
    BREAK,
    BLOCK,
    KNOCK,
    NONE
};

/* does not include NONE */
static constexpr const size_t NUM_DOOR_ACTION_TYPES = 10;

namespace enums {
#define ALL_DOOR_ACTION_TYPES ::enums::getAllDoorActionTypes()
const std::array<DoorActionType, NUM_DOOR_ACTION_TYPES> &getAllDoorActionTypes();

} // namespace enums

#endif // MMAPPER_DOORACTION_H
