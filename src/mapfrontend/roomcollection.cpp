// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "roomcollection.h"

#include "../global/RAII.h"
#include "../map/room.h"
#include "AbstractRoomVisitor.h"

#include <cassert>
#include <memory>

#ifndef NDEBUG
#define DEBUG_ONLY(x) x // NOLINT
#else
#define DEBUG_ONLY(x) static_cast<void>(42)
#endif

#define DEBUG_LOCK() DEBUG_ONLY(assert(!m_inUse); MAYBE_UNUSED const RAIIBool useLock{m_inUse})

void RoomCollection::addRoom(Room *const room)
{
    if (room == nullptr) {
        assert(false);
        return;
    }

    DEBUG_LOCK();
    m_rooms.insert(room->shared_from_this());
}

void RoomCollection::removeRoom(Room *const room)
{
    if (room == nullptr) {
        assert(false);
        return;
    }

    DEBUG_LOCK();
    m_rooms.erase(room->shared_from_this());
}

void RoomCollection::clear()
{
    DEBUG_LOCK();
    return m_rooms.clear();
}

void RoomCollection::forEach(AbstractRoomVisitor &stream) const
{
    DEBUG_LOCK();
    for (const std::shared_ptr<Room> &room : m_rooms) {
        stream.visit(room.get());
    }
}
