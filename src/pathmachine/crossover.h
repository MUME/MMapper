#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <list>

#include "../mapdata/ExitDirection.h"
#include "../mapdata/mmapper2exit.h"
#include "experimenting.h"

class AbstractRoomFactory;
class Path;
class PathParameters;
class Room;
class RoomAdmin;

class Crossover : public Experimenting
{
public:
    Crossover(std::list<Path *> *paths,
              ExitDirection dirCode,
              PathParameters &params,
              AbstractRoomFactory *factory);
    void receiveRoom(RoomAdmin *, const Room *) override;
};
