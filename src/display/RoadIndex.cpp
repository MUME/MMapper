// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "RoadIndex.h"

#include <stdexcept>

#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../mapdata/ExitDirection.h"

RoadIndexMaskEnum getRoadIndex(const ExitDirEnum dir)
{
    if (isNESW(dir))
        return static_cast<RoadIndexMaskEnum>(1 << static_cast<int>(dir));
    throw std::invalid_argument("dir");
}

RoadIndexMaskEnum getRoadIndex(const Room &room)
{
    RoadIndexMaskEnum roadIndex = RoadIndexMaskEnum::NONE;

    for (const ExitDirEnum dir : ALL_EXITS_NESW)
        if (room.exit(dir).exitIsRoad())
            roadIndex |= getRoadIndex(dir);

    return roadIndex;
}
