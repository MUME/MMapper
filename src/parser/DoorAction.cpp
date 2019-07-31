// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "DoorAction.h"

#include <array>

#include "../global/enums.h"

namespace enums {

const MMapper::Array<DoorActionEnum, NUM_DOOR_ACTION_TYPES> &getAllDoorActionTypes()
{
    static const auto g_door_actions = genEnumValues<DoorActionEnum, NUM_DOOR_ACTION_TYPES>();
    return g_door_actions;
}

} // namespace enums
