#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../global/macros.h"
#include "Changes.h"

#include <optional>
#include <ostream>

namespace room_revert {

struct NODISCARD RevertPlan final
{
    RawRoom expect;     // what we expect to see at the end
    ChangeList changes; // changes to apply
    bool hintUndelete = false;
    bool warnNoEntrances = false;
};

// assumes the current map is a modified version of the base map
NODISCARD std::optional<RevertPlan> build_plan(AnsiOstream &os,
                                               const Map &currentMap,
                                               RoomId roomId,
                                               const Map &baseMap);

} // namespace room_revert
