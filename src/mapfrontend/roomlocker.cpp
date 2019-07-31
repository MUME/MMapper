// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "roomlocker.h"

#include "../expandoracommon/AbstractRoomFactory.h"
#include "../expandoracommon/RoomRecipient.h"
#include "../expandoracommon/room.h"
#include "mapfrontend.h"

RoomLocker::RoomLocker(RoomRecipient &forward,
                       MapFrontend &frontend,
                       AbstractRoomFactory *in_factory,
                       ParseEvent *compare)
    : recipient(forward)
    , data(frontend)
    , factory(in_factory)
    , comparator(compare)
{}

RoomLocker::~RoomLocker() = default;

void RoomLocker::visit(const Room *room)
{
    if ((factory != nullptr) && (comparator != nullptr)) {
        if (factory->compareWeakProps(room, *comparator) != ComparisonResultEnum::DIFFERENT) {
            data.lockRoom(&recipient, room->getId());
            recipient.receiveRoom(&data, room);
        }
    } else {
        data.lockRoom(&recipient, room->getId());
        recipient.receiveRoom(&data, room);
    }
}
