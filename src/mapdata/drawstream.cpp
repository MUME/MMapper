// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "drawstream.h"

DrawStream::DrawStream(MapCanvasRoomDrawer &in, const RoomIndex &in_rooms, const RoomLocks &in_locks)
    : canvas(in)
    , roomIndex(in_rooms)
    , locks(in_locks)
{}

DrawStream::~DrawStream() = default;

void DrawStream::visit(const Room *room)
{
    if (room != nullptr) {
        const auto z = room->getPosition().z;
        layerToRooms[z].emplace_back(room);
    }
}

void DrawStream::draw() const
{
    canvas.drawRooms(layerToRooms, roomIndex, locks);
}
