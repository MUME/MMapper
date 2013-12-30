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

#ifndef OLDROOM
#define OLDROOM

#include "oldconnection.h"

#define RSF_ROAD_N    	bit1 //road
#define RSF_ROAD_S    	bit2
#define RSF_ROAD_E    	bit3
#define RSF_ROAD_W    	bit4
#define RSF_ROAD_U    	bit5
#define RSF_ROAD_D    	bit6
#define RSF_EXIT_N 		bit7 //exits WITHOUT doors
#define RSF_EXIT_S 		bit8
#define RSF_EXIT_E 		bit9
#define RSF_EXIT_W 		bit10
#define RSF_EXIT_U 		bit11
#define RSF_EXIT_D 		bit12
#define RSF_DOOR_N 		bit13 //exits WITH doors
#define RSF_DOOR_S 		bit14
#define RSF_DOOR_E 		bit15
#define RSF_DOOR_W 		bit16
#define RSF_DOOR_U 		bit17
#define RSF_DOOR_D 		bit18
#define RSF_NO_MATCH_N      bit19 //do not match exit [dir]
#define RSF_NO_MATCH_S      bit20
#define RSF_NO_MATCH_E      bit21
#define RSF_NO_MATCH_W      bit22
#define RSF_NO_MATCH_U      bit23
#define RSF_NO_MATCH_D      bit24
#define RSF_RESERVED_1      bit25
#define RSF_RESERVED_2      bit26
typedef quint32 RoomSpecialFlags;

typedef quint32 RoomID;
typedef QDateTime RoomTimeStamp;

typedef QList<Connection *> ConnectionList;
typedef QListIterator<Connection *> ConnectionListIterator;

#endif
