// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "MapZoomSlider.h"

#include "../display/mapwindow.h"
#include "../global/SignalBlocker.h"

MapZoomSlider::~MapZoomSlider() = default;

int MapZoomSlider::calcPos(const float zoom) noexcept
{
    static const float INV_DIVISOR = 1.f / std::log2(ScaleFactor::ZOOM_STEP);
    return static_cast<int>(std::lround(SCALE * std::log2(zoom) * INV_DIVISOR));
}

MapZoomSlider::MapZoomSlider(MapWindow &map, QWidget *const parent)
    : QSlider(Qt::Orientation::Horizontal, parent)
    , m_map{map}
{
    setRange(min, max);
    setFromActual();

    connect(this, &QSlider::valueChanged, this, [this](int /*value*/) {
        requestChange();
        setFromActual();
    });

    connect(&map, &MapWindow::sig_zoomChanged, this, [this](float) { setFromActual(); });
    setToolTip("Zoom");
}

void MapZoomSlider::requestChange()
{
    const float desiredZoomSteps = static_cast<float>(clamp(value())) * INV_SCALE;
    {
        const SignalBlocker block{*this};
        m_map.setZoom(std::pow(ScaleFactor::ZOOM_STEP, desiredZoomSteps));
    }
    m_map.slot_graphicsSettingsChanged();
}

void MapZoomSlider::setFromActual()
{
    const float actualZoom = m_map.getZoom();
    const int rounded = calcPos(actualZoom);
    {
        const SignalBlocker block{*this};
        setValue(clamp(rounded));
    }
}
