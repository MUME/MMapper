#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "RawRoom.h"
#include "room.h"

NODISCARD extern ComparisonResultEnum compare(const RawRoom &,
                                              const ParseEvent &event,
                                              int tolerance);
NODISCARD extern ComparisonResultEnum compareWeakProps(const RawRoom &, const ParseEvent &event);
