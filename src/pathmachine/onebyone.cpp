// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "onebyone.h"

#include <list>

#include "../expandoracommon/AbstractRoomFactory.h"
#include "../expandoracommon/parseevent.h"
#include "../global/utils.h"
#include "../parser/CommandId.h"
#include "experimenting.h"
#include "pathparameters.h"
#include "roomsignalhandler.h"

class Path;

OneByOne::OneByOne(AbstractRoomFactory *const in_factory,
                   const SigParseEvent &sigParseEvent,
                   PathParameters &in_params,
                   RoomSignalHandler *const in_handler)
    : Experimenting{new std::list<Path *>,
                    getDirection(sigParseEvent.deref().getMoveType()),
                    in_params,
                    in_factory}
    , event{sigParseEvent.getShared()}
    , handler{in_handler}
{}

void OneByOne::receiveRoom(RoomAdmin *const admin, const Room *const room)
{
    if (factory->compare(room, deref(event), params.matchingTolerance) == ComparisonResult::EQUAL) {
        augmentPath(shortPaths->back(), admin, room);
    } else {
        // needed because the memory address is not unique and
        // calling admin->release might destroy a room still held by some path
        handler->hold(room, admin, this);
        handler->release(room);
    }
}

void OneByOne::addPath(Path *const path)
{
    shortPaths->push_back(path);
}
