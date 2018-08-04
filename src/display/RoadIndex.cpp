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

#include "RoadIndex.h"

#include <stdexcept>

#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../mapdata/ExitDirection.h"

RoadIndex getRoadIndex(const ExitDirection dir)
{
    if (isNESW(dir))
        return static_cast<RoadIndex>(1 << static_cast<int>(dir));
    throw std::invalid_argument("dir");
}

RoadIndex getRoadIndex(const Room &room)
{
    RoadIndex roadIndex = RoadIndex::NONE;

    for (auto dir : ALL_EXITS_NESW)
        if (room.exit(dir).exitIsRoad())
            roadIndex |= getRoadIndex(dir);

    return roadIndex;
}
