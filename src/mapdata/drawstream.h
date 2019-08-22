#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <map>
#include <vector>

#include "../display/mapcanvas.h"
#include "../mapfrontend/AbstractRoomVisitor.h"

class DrawStream final : public AbstractRoomVisitor
{
public:
    explicit DrawStream(MapCanvasRoomDrawer &in,
                        const RoomIndex &in_rooms,
                        const RoomLocks &in_locks);
    virtual ~DrawStream() override;
    virtual void visit(const Room *room) override;

    void draw() const;

private:
    LayerToRooms layerToRooms;
    MapCanvasRoomDrawer &canvas;
    const RoomIndex &roomIndex;
    const RoomLocks &locks;
};
