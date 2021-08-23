#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <memory>

#include "../mapdata/ExitDirection.h"
#include "../mapdata/mmapper2exit.h"
#include "experimenting.h"
#include "path.h"

class Room;
class RoomAdmin;
struct PathParameters;

class NODISCARD Crossover final : public Experimenting
{
public:
    Crossover(std::shared_ptr<PathList> paths, ExitDirEnum dirCode, PathParameters &params);

private:
    void virt_receiveRoom(RoomAdmin *, const Room *) final;
};
