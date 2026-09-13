#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/ChangeMonitor.h"

namespace MapCanvasConfig {

// Scroll-bar units per world unit; shared by MapCanvas and MapScroller.
static constexpr const int SCROLL_SCALE = 64;

void registerChangeCallback(const ChangeMonitor::Lifetime &lifetime,
                            ChangeMonitor::Function callback);

NODISCARD extern bool isIn3dMode();
extern void set3dMode(bool);
NODISCARD extern bool isAutoTilt();
extern void setAutoTilt(bool val);
NODISCARD extern bool getShowPerfStats();
extern void setShowPerfStats(bool);

} // namespace MapCanvasConfig
