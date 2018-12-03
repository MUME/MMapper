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

#include "roomselection.h"

#include <cassert>

#include "../expandoracommon/room.h"
#include "../mapdata/mapdata.h"

SigRoomSelection::SigRoomSelection(const SharedRoomSelection &selection)
    : m_sharedRoomSelection{selection}
{
    requireValid(); /* throws invalid argument */
}

RoomSelection &SigRoomSelection::deref() const
{
    return ::deref(m_sharedRoomSelection);
}

const SharedRoomSelection &SigRoomSelection::getShared() const
{
    if (auto &p = m_sharedRoomSelection)
        return p;
    throw std::runtime_error("null pointer");
}

RoomSelection::RoomSelection(MapData *admin)
    : m_admin(admin)
{
    assert(m_admin != nullptr);
}

RoomSelection::~RoomSelection()
{
    m_admin->unselect(this);
}

void RoomSelection::receiveRoom(RoomAdmin *admin, const Room *aRoom)
{
    assert(admin == m_admin);
    insert(aRoom->getId(), aRoom);
}
