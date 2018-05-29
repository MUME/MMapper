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

#include "mmapper2room.h"
#include "room.h"

#include <QString>

RoomName Room::getName() const
{
    return at(R_NAME).toString();
}

RoomDescription Room::getDescription() const
{
    return at(R_DESC).toString();
}

RoomDescription Room::getDynamicDescription() const
{
    return at(R_DYNAMICDESC).toString();
}

RoomNote Room::getNote() const
{
    return at(R_NOTE).toString();
}

RoomMobFlags Room::getMobFlags() const
{
    return at(R_MOBFLAGS).toUInt();
}

RoomLoadFlags Room::getLoadFlags() const
{
    return at(R_LOADFLAGS).toUInt();
}

RoomTerrainType Room::getTerrainType() const
{
    return static_cast<RoomTerrainType>(at(R_TERRAINTYPE).toUInt());
}

RoomPortableType Room::getPortableType() const
{
    return static_cast<RoomPortableType>(at(R_PORTABLETYPE).toUInt());
}

RoomLightType Room::getLightType() const
{
    return static_cast<RoomLightType>(at(R_LIGHTTYPE).toUInt());
}

RoomAlignType Room::getAlignType() const
{
    return static_cast<RoomAlignType>(at(R_ALIGNTYPE).toUInt());
}

RoomRidableType Room::getRidableType() const
{
    return static_cast<RoomRidableType>(at(R_RIDABLETYPE).toUInt());
}

RoomSundeathType Room::getSundeathType() const
{
    return static_cast<RoomSundeathType>(at(R_SUNDEATHTYPE).toUInt());
}


namespace Mmapper2Room {
RoomName getName(const Room *room)
{
    return room->getName();
}

RoomDescription getDescription(const Room *room)
{
    return room->getDescription();
}

RoomDescription getDynamicDescription(const Room *room)
{
    return room->getDynamicDescription();
}

RoomNote getNote(const Room *room)
{
    return room->getNote();
}

RoomMobFlags getMobFlags(const Room *room)
{
    return room->getMobFlags();
}

RoomLoadFlags getLoadFlags(const Room *room)
{
    return room->getLoadFlags();
}

RoomTerrainType getTerrainType(const Room *room)
{
    return room->getTerrainType();
}

RoomPortableType getPortableType(const Room *room)
{
    return room->getPortableType();
}

RoomLightType getLightType(const Room *room)
{
    return room->getLightType();
}

RoomAlignType getAlignType(const Room *room)
{
    return room->getAlignType();
}

RoomRidableType getRidableType(const Room *room)
{
    return room->getRidableType();
}

RoomSundeathType getSundeathType(const Room *room)
{
    return room->getSundeathType();
}
}  // namespace Mmapper2Room
