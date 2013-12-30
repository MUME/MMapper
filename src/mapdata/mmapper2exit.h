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

#ifndef MMAPPER2EXIT_H
#define MMAPPER2EXIT_H

#include <QtGlobal>

class Room;
class Exit;

enum ExitType { ET_NORMAL, ET_LOOP, ET_ONEWAY, ET_UNDEFINED };

enum ExitDirection { ED_NORTH=0, ED_SOUTH, ED_EAST, ED_WEST, ED_UP, 
			   ED_DOWN, ED_UNKNOWN, ED_NONE };

ExitDirection opposite(ExitDirection in);
uint opposite(uint in);
ExitDirection dirForChar(char dir);

typedef class QString DoorName;

#define EF_EXIT       bit1
#define EF_DOOR       bit2
#define EF_ROAD       bit3
#define EF_CLIMB      bit4
#define EF_RANDOM     bit5
#define EF_SPECIAL    bit6
#define EF_NO_MATCH   bit7

#define DF_HIDDEN     bit1
#define DF_NEEDKEY    bit2
#define DF_NOBLOCK    bit3
#define DF_NOBREAK    bit4
#define DF_NOPICK     bit5
#define DF_DELAYED    bit6
#define DF_RESERVED1  bit7
#define DF_RESERVED2  bit8

enum ExitField {E_DOORNAME = 0, E_FLAGS, E_DOORFLAGS};

typedef quint8 ExitFlags;
typedef quint8 DoorFlags;

ExitFlags getFlags(const Exit & e);

DoorName getDoorName(const Exit & e);

DoorFlags getDoorFlags(const Exit & e);

void updateExit(Exit & e, ExitFlags flags);

void orExitFlags(Exit & e, ExitFlags flags);
  
void nandExitFlags(Exit & e, ExitFlags flags);

void orDoorFlags(Exit & e, DoorFlags flags);
  
void nandDoorFlags(Exit & e, DoorFlags flags);
  
#endif
