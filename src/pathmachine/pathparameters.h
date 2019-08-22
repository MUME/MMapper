#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
using uint = unsigned int;

class PathParameters
{
public:
    PathParameters();

    double acceptBestRelative;
    double acceptBestAbsolute;
    double newRoomPenalty;
    double multipleConnectionsPenalty;
    double correctPositionBonus;
    double maxPaths;
    int matchingTolerance;
    uint maxSkipped;
    ~PathParameters() {}
};
