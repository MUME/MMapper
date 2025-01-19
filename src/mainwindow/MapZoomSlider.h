#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019-2024 The MMapper Authors

#include "../display/MapCanvasData.h"
#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <QSlider>

class MapWindow;

class NODISCARD_QOBJECT MapZoomSlider final : public QSlider
{
private:
    static constexpr float SCALE = 100.f;
    static constexpr float INV_SCALE = 1.f / SCALE;

    // can't get this to work as constexpr, so we'll just inline the static min/max.
    static int calcPos(const float zoom) noexcept;
    static inline const int min = calcPos(ScaleFactor::MIN_VALUE);
    static inline const int max = calcPos(ScaleFactor::MAX_VALUE);
    MapWindow &m_map;

public:
    explicit MapZoomSlider(MapWindow &map, QWidget *const parent);
    ~MapZoomSlider() final;

public:
    void requestChange();

    void setFromActual();

private:
    static int clamp(int val) { return std::clamp(val, min, max); }
};
