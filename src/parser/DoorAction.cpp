// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "DoorAction.h"

#include <array>

#include "../global/enums.h"

namespace enums {

const std::array<DoorActionType, NUM_DOOR_ACTION_TYPES> &getAllDoorActionTypes()
{
    static const auto g_door_actions = genEnumValues<DoorActionType, NUM_DOOR_ACTION_TYPES>();
    return g_door_actions;
}

} // namespace enums
