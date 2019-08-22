// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

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
