#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/ChangeMonitor.h"

namespace MapCanvasConfig {

extern ConnectionSet registerChangeCallback(ChangeMonitor::Function callback);

extern bool isIn3dMode();
extern void set3dMode(bool);
extern bool isAutoTilt();
extern void setAutoTilt(bool val);
extern bool getShowPerfStats();
extern void setShowPerfStats(bool);

} // namespace MapCanvasConfig
