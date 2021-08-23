#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstddef>

#include "../global/Array.h"

enum class NODISCARD DoorActionEnum {
    OPEN,
    CLOSE,
    LOCK,
    UNLOCK,
    PICK,
    ROCK,
    BASH,
    BREAK,
    BLOCK,
    KNOCK,
    NONE
};

/* does not include NONE */
static constexpr const size_t NUM_DOOR_ACTION_TYPES = 10;

namespace enums {
#define ALL_DOOR_ACTION_TYPES ::enums::getAllDoorActionTypes()
NODISCARD const MMapper::Array<DoorActionEnum, NUM_DOOR_ACTION_TYPES> &getAllDoorActionTypes();

} // namespace enums
