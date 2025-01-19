// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "roomlocker.h"

#include "../map/RoomRecipient.h"
#include "../map/room.h"
#include "mapfrontend.h"

RoomLocker::RoomLocker(RoomRecipient &forward,
                       MapFrontend &frontend,
                       const ParseEvent *const compare)
    : recipient(forward)
    , data(frontend)
    , comparator(compare)
{}

RoomLocker::~RoomLocker() = default;

void RoomLocker::visit(const Room *room)
{
    if (comparator == nullptr) {
        data.lockRoom(&recipient, room->getId());
        recipient.receiveRoom(&data, room);
    } else if (Room::compareWeakProps(room, *comparator) != ComparisonResultEnum::DIFFERENT) {
        data.lockRoom(&recipient, room->getId());
        recipient.receiveRoom(&data, room);
    }
}
