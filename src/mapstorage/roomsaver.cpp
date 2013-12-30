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

#include "roomsaver.h"
#include "room.h"
#include "roomadmin.h"
#include <assert.h>

RoomSaver::RoomSaver(RoomAdmin * in_admin, QList<const Room *> & list) :
roomsCount(0), roomList(list), admin(in_admin) {}


void RoomSaver::receiveRoom(RoomAdmin * in_admin, const Room * room)
{
  assert(in_admin == admin);
  if (room->isTemporary()) {
	admin->releaseRoom(this, room->getId());
  }
  else {
  	roomList.append(room);
  	roomsCount++;
  }
}

quint32 RoomSaver::getRoomsCount()
{
  return roomsCount;
}

RoomSaver::~RoomSaver()
{
  for(int i = 0; i < roomList.size(); ++i)
  {
    const Room * room = roomList[i];
    if (room)
    {
      admin->releaseRoom(this, room->getId());
      roomList[i] = 0;
    }
  }
}
