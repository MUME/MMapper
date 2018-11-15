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

#include "roomcollection.h"

#include <cassert>

#include "../global/RAII.h"
#include "AbstractRoomVisitor.h"

#ifndef NDEBUG
#define DEBUG_ONLY(x) x // NOLINT
#else
#define DEBUG_ONLY(x)
#endif

#define DEBUG_LOCK() DEBUG_ONLY(assert(!m_inUse); const RAIIBool useLock{m_inUse})

IRoomCollection::~IRoomCollection() = default;

void RoomCollection::addRoom(Room *room)
{
    DEBUG_LOCK();
    m_rooms.insert(room);
}

void RoomCollection::removeRoom(Room *room)
{
    DEBUG_LOCK();
    m_rooms.erase(room);
}

void RoomCollection::clear()
{
    DEBUG_LOCK();
    return m_rooms.clear();
}

void RoomCollection::forEach(AbstractRoomVisitor &stream) const
{
    DEBUG_LOCK();
    for (Room *const room : m_rooms) {
        stream.visit(room);
    }
}
