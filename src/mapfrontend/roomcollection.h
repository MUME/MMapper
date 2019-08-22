#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

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
