/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#include "oldconnection.h"

#include <utility>

#include "../global/EnumIndexedArray.h"
#include "olddoor.h"

Connection::Connection()
{
    // REVISIT: These might already be set correctly by default.
    m_rooms[Hand::LEFT] = nullptr;
    m_rooms[Hand::RIGHT] = nullptr;
    m_doors[Hand::LEFT] = nullptr;
    m_doors[Hand::RIGHT] = nullptr;
    m_directions[Hand::LEFT] = ConnectionDirection::UNKNOWN;
    m_directions[Hand::RIGHT] = ConnectionDirection::UNKNOWN;
}

Connection::~Connection()
{
    // no one keeps track of the doors so we have to remove them here
    for (auto &d : m_doors)
        delete std::exchange(d, nullptr);
}

ConnectionDirection opposite(ConnectionDirection in)
{
    switch (in) {
    case ConnectionDirection::NORTH:
        return ConnectionDirection::SOUTH;
    case ConnectionDirection::SOUTH:
        return ConnectionDirection::NORTH;
    case ConnectionDirection::WEST:
        return ConnectionDirection::EAST;
    case ConnectionDirection::EAST:
        return ConnectionDirection::WEST;
    case ConnectionDirection::UP:
        return ConnectionDirection::DOWN;
    case ConnectionDirection::DOWN:
        return ConnectionDirection::UP;
    default:
        return ConnectionDirection::UNKNOWN;
    }
}
