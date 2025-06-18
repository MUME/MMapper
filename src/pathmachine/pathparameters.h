#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/macros.h"

#include <cstdint>

/*!
 * @brief Parameters used by the PathMachine for pathfinding calculations.
 */
struct NODISCARD PathParameters final
{
    double acceptBestRelative = 10.0;
    double acceptBestAbsolute = 3.0;
    double newRoomPenalty = 5.0;
    double multipleConnectionsPenalty = 2.0;
    double correctPositionBonus = 5.1;
    double maxPaths = 500.0;
    int matchingTolerance = 5;
    uint32_t maxSkipped = 1;
};
