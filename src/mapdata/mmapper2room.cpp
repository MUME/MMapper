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

RoomName getName(const Room * room)
  { return (*room)[R_NAME].toString(); }
  
RoomDescription getDescription(const Room * room)
  { return (*room)[R_DESC].toString(); }
  
RoomDescription getDynamicDescription(const Room * room) 
  { return (*room)[R_DYNAMICDESC].toString(); }
  
RoomNote getNote(const Room * room) 
  { return (*room)[R_NOTE].toString(); }

RoomMobFlags getMobFlags(const Room * room) 
  { return (*room)[R_MOBFLAGS].toUInt(); }
  
RoomLoadFlags getLoadFlags(const Room * room) 
  { return (*room)[R_LOADFLAGS].toUInt(); }
  
RoomTerrainType getTerrainType(const Room * room) 
  { return (RoomTerrainType)(*room)[R_TERRAINTYPE].toUInt(); }
  
RoomPortableType getPortableType(const Room * room) 
  { return (RoomPortableType)(*room)[R_PORTABLETYPE].toUInt(); }
  
RoomLightType getLightType(const Room * room) 
  { return (RoomLightType)(*room)[R_LIGHTTYPE].toUInt(); }
  
RoomAlignType getAlignType(const Room * room) 
  { return (RoomAlignType)(*room)[R_ALIGNTYPE].toUInt(); }

RoomRidableType getRidableType(const Room * room) 
  { return (RoomRidableType)(*room)[R_RIDABLETYPE].toUInt(); }

