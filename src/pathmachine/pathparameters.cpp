// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "pathparameters.h"

PathParameters::PathParameters()
    : acceptBestRelative(10.0)
    , acceptBestAbsolute(3.0)
    , newRoomPenalty(5.0)
    , multipleConnectionsPenalty(2.0)
    , correctPositionBonus(5.1)
    , maxPaths(500.0)
    , matchingTolerance(5)
    , maxSkipped(1)
{}
