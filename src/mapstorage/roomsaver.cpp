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
#include "../expandoracommon/room.h"
#include "../expandoracommon/roomadmin.h"
#include "../mapdata/mapdata.h"
#include <cassert>

RoomSaver::RoomSaver(RoomAdmin &admin, ConstRoomList &list)
    : m_roomList(list)
    , m_admin(admin)
{}

void RoomSaver::receiveRoom(RoomAdmin *admin, const Room *room)
{
    assert(admin == &m_admin);
    if (room->isTemporary()) {
        m_admin.releaseRoom(*this, room->getId());
    } else {
        m_roomList.append(room);
        m_roomsCount++;
    }
}

quint32 RoomSaver::getRoomsCount()
{
    return m_roomsCount;
}

RoomSaver::~RoomSaver()
{
    for (auto &room : m_roomList) {
        if (room != nullptr) {
            m_admin.releaseRoom(*this, room->getId());
            room = nullptr;
        }
    }
}
