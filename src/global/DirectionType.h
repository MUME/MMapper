#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "Array.h"

enum class DirectionEnum {
    NORTH = 0,
    SOUTH = 1,
    EAST = 2,
    WEST = 3,
    UP = 4,
    DOWN = 5,
    NONE = 6,   // CAUTION: ExitDirEnum::UNKNOWN == 6
    UNKNOWN = 7 // CAUTION: ExitDirEnum::NONE == 7
};

namespace enums {
const MMapper::Array<DirectionEnum, 6> &getAllDirections6();
}

#define ALL_DIRECTIONS6 enums::getAllDirections6()
