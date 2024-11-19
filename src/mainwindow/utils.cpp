// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019-2024 The MMapper Authors

#include "utils.h"

#include "../display/mapcanvas.h"

CanvasDisabler::CanvasDisabler(MapCanvas &in_canvas)
    : canvas{in_canvas}
{
    canvas.setEnabled(false);
}

CanvasDisabler::~CanvasDisabler()
{
    canvas.setEnabled(true);
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
