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

#include "mmapper2exit.h"
#include "defs.h"
#include "exit.h"

#include <QString>

ExitFlags getFlags(const Exit & e) 
  {return e[E_FLAGS].toUInt();}

DoorName getDoorName(const Exit & e)
  {return e[E_DOORNAME].toString();}

DoorFlags getDoorFlags(const Exit & e)
  {return e[E_DOORFLAGS].toUInt();}

void orExitFlags(Exit & e, ExitFlags flags)
  {e[E_FLAGS] = getFlags(e) | flags;}
  
void nandExitFlags(Exit & e, ExitFlags flags)
  {e[E_FLAGS] = getFlags(e) & ~flags;}

void orDoorFlags(Exit & e, DoorFlags flags)
  {e[E_DOORFLAGS] = getDoorFlags(e) | flags;}
  
void nandDoorFlags(Exit & e, DoorFlags flags)
  {e[E_DOORFLAGS] = getDoorFlags(e) & ~flags;}


uint opposite(uint in) {
  return opposite((ExitDirection)in);
}

ExitDirection opposite(ExitDirection in)
{
  switch (in)
  {
  case ED_NORTH: return ED_SOUTH;
  case ED_SOUTH: return ED_NORTH;
  case ED_WEST: return ED_EAST;
  case ED_EAST: return ED_WEST;
  case ED_UP: return ED_DOWN;
  case ED_DOWN: return ED_UP;
  default: return ED_UNKNOWN;
  }
}
 

void updateExit(Exit & e, ExitFlags flags ) {
  ExitFlags diff = flags ^ getFlags(e);
  if (diff) {
    flags |= EF_NO_MATCH;
    flags |= diff;
    e[E_FLAGS] = flags;
  }
}


ExitDirection dirForChar(char dir) {
	switch (dir) {
		case 'n': return ED_NORTH;
		case 's': return ED_SOUTH;
		case 'e': return ED_EAST;
		case 'w': return ED_WEST;
		case 'u': return ED_UP;
		case 'd': return ED_DOWN;
		default: return ED_UNKNOWN;
	}
}

