// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "crossover.h"

#include <memory>

#include "../mapdata/ExitDirection.h"
#include "experimenting.h"

class Path;
class Room;
class RoomAdmin;
struct PathParameters;

Crossover::Crossover(std::shared_ptr<PathList> paths, ExitDirEnum dirCode, PathParameters &params)
    : Experimenting(std::move(paths), dirCode, params)
{}

void Crossover::receiveRoom(RoomAdmin *map, const Room *room)
{
    for (auto &shortPath : *shortPaths) {
        augmentPath(shortPath, map, room);
    }
}
