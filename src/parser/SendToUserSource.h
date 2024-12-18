#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "global/macros.h"

#include <cstdint>

enum class NODISCARD SendToUserSource : uint8_t {
    FromMud,
    DuplicatePrompt,
    SimulatedPrompt,
    SimulatedOutput,
    FromMMapper,
    NoLongerPrompted,
};
