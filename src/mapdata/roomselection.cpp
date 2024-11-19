// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "roomselection.h"

#include "mapdata.h"

#include <memory>

RoomSelection::RoomSelection(Badge<RoomSelection>, RoomIdSet set)
    : m_set{std::move(set)}
{}

SharedRoomSelection RoomSelection::createSelection(RoomIdSet set)
{
    return std::make_shared<RoomSelection>(Badge<RoomSelection>{}, std::move(set));
}

RoomId RoomSelection::getFirstRoomId() const
{
    if (empty()) {
        throw std::runtime_error("empty");
    }
    return m_set.first();
}

void RoomSelection::insert(const RoomHandle &room)
{
    insert(room.getId());
}

void RoomSelection::removeMissing(MapData &mapData)
{
    mapData.removeMissing(m_set);
}
