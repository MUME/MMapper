// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "drawstream.h"

DrawStream::DrawStream(LayerToRooms &layerToRooms)
    : m_layerToRooms(layerToRooms)
{}

DrawStream::~DrawStream() = default;

void DrawStream::visit(const Room *room)
{
    if (room == nullptr) {
        return;
    }

    const auto z = room->getPosition().z;
    auto &layer = m_layerToRooms[z];
    layer.emplace_back(room);
}
