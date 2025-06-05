// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019-2024 The MMapper Authors

#include "utils.h"

#include "../display/mapcanvas.h"
#include "../display/mapwindow.h"

CanvasDisabler::CanvasDisabler(MapWindow &in_window)
    : window{in_window}
{
    window.getCanvas()->setEnabled(false);
}

CanvasDisabler::~CanvasDisabler()
{
    window.getCanvas()->setEnabled(true);
    window.hideSplashImage();
}

MapFrontendBlocker::MapFrontendBlocker(MapFrontend &in_data)
    : data(in_data)
{
    data.block();
}

MapFrontendBlocker::~MapFrontendBlocker()
{
    data.unblock();
}
