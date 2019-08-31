// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "onebyone.h"

#include <list>
#include <memory>

#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/room.h"
#include "../global/utils.h"
#include "../parser/CommandId.h"
#include "experimenting.h"
#include "pathparameters.h"
#include "roomsignalhandler.h"

class Path;

OneByOne::OneByOne(const SigParseEvent &sigParseEvent,
                   PathParameters &in_params,
                   RoomSignalHandler *const in_handler)
    : Experimenting{PathList::alloc(), getDirection(sigParseEvent.deref().getMoveType()), in_params}
    , event{sigParseEvent.getShared()}
    , handler{in_handler}
{}

void OneByOne::receiveRoom(RoomAdmin *const admin, const Room *const room)
{
    if (Room::compare(room, deref(event), params.matchingTolerance) == ComparisonResultEnum::EQUAL) {
        augmentPath(shortPaths->back(), admin, room);
    } else {
        // needed because the memory address is not unique and
        // calling admin->release might destroy a room still held by some path
        handler->hold(room, admin, this);
        handler->release(room);
    }
}

void OneByOne::addPath(std::shared_ptr<Path> path)
{
    shortPaths->emplace_back(std::move(path));
}
