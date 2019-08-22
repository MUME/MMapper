#pragma once
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

#include <cassert>
#include <set>

#include "../global/RAII.h"
#include "AbstractRoomVisitor.h"

class AbstractRoomVisitor;
class Room;

struct IRoomCollection
{
public:
    virtual ~IRoomCollection();
    virtual void addRoom(Room *room) = 0;
    virtual void removeRoom(Room *room) = 0;

public:
    virtual void clear() = 0;
    virtual size_t size() const = 0;

public:
    /* NOTE: It's not safe for the stream to modify this
     * collection during this function call. */
    virtual void forEach(AbstractRoomVisitor &stream) const = 0;
};

class RoomCollection final : public IRoomCollection
{
private:
    using RoomSet = std::set<Room *>;
    RoomSet m_rooms;
    mutable bool m_inUse = false;

public:
    void addRoom(Room *room) override;
    void removeRoom(Room *room) override;

public:
    void clear() override;
    size_t size() const override { return m_rooms.size(); }

public:
    /* NOTE: It's not safe for the stream to modify this
     * collection during this function call. */
    void forEach(AbstractRoomVisitor &stream) const override;
};
