// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "crossover.h"

#include "../mapdata/ExitDirection.h"
#include "experimenting.h"

class Path;
class PathParameters;
class Room;
class RoomAdmin;

Crossover::Crossover(std::list<Path *> *paths,
                     ExitDirection dirCode,
                     PathParameters &params,
                     AbstractRoomFactory *in_factory)
    : Experimenting(paths, dirCode, params, in_factory)
{}

void Crossover::receiveRoom(RoomAdmin *map, const Room *room)
{
    for (auto &shortPath : *shortPaths) {
        augmentPath(shortPath, map, room);
    }
}
