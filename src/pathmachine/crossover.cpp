// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "crossover.h"

#include <memory>

#include "../expandoracommon/RoomAdmin.h"
#include "../expandoracommon/room.h"
#include "../mapdata/ExitDirection.h"
#include "experimenting.h"

struct PathParameters;

Crossover::Crossover(std::shared_ptr<PathList> _paths,
                     const ExitDirEnum _dirCode,
                     PathParameters &_params)
    : Experimenting(std::move(_paths), _dirCode, _params)
{}

void Crossover::virt_receiveRoom(RoomAdmin *const admin, const Room *const room)
{
    if (shortPaths->empty())
        admin->releaseRoom(*this, room->getId());

    for (auto &shortPath : *shortPaths) {
        augmentPath(shortPath, admin, room);
    }
}
