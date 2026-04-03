#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Array.h"
#include "../global/enums.h"

#include <cstddef>

enum class NODISCARD DoorActionEnum : uint8_t {
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

DEFINE_ENUM_COUNT(DoorActionEnum, NUM_DOOR_ACTION_TYPES)

namespace enums {
#define ALL_DOOR_ACTION_TYPES ::enums::getAllDoorActionTypes()
DECL_ENUM_GETTER2(DoorActionEnum, NUM_DOOR_ACTION_TYPES, getAllDoorActionTypes)
} // namespace enums
