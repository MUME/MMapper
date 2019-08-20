#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <cassert>
#include <memory>
#include <set>

#include "../global/RAII.h"
#include "AbstractRoomVisitor.h"

class AbstractRoomVisitor;
class Room;

class RoomCollection final
{
private:
    using RoomSet = std::set<std::shared_ptr<Room>>;
    RoomSet m_rooms;
    mutable bool m_inUse = false;

public:
    void addRoom(Room *room);
    void removeRoom(Room *room);

    void addRoom(const SharedRoom &room) { addRoom(room.get()); }
    void removeRoom(const SharedRoom &room) { removeRoom(room.get()); }

public:
    void clear();
    size_t size() const { return m_rooms.size(); }

public:
    /* NOTE: It's not safe for the stream to modify this
     * collection during this function call. */
    void forEach(AbstractRoomVisitor &stream) const;
};
