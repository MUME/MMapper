// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019-2024 The MMapper Authors

#include "utils.h"

#include "../display/mapcanvas.h"
#include "../display/mapwindow.h"

CanvasDisabler::CanvasDisabler(MapWindow &window)
    : m_window{window}
{
    m_window.setCanvasEnabled(false);
}

CanvasDisabler::~CanvasDisabler()
{
    m_window.setCanvasEnabled(true);
    m_window.hideSplashImage();
}

MapFrontendBlocker::MapFrontendBlocker(MapFrontend &data)
    : m_data(data)
{
    m_data.block();
}

MapFrontendBlocker::~MapFrontendBlocker()
{
    m_data.unblock();
}
