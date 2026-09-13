#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

// MapScroller holds the map view's scroll math (world<->scroll-unit
// conversion, driven by the known map bounds) and its continuous-scroll
// timer. It owns no widgets, so it belongs in the unconditional display
// sources.
//
// MapWindow (see mapwindow.cpp) uses this for the math/timer, and keeps
// owning its QScrollBar widgets and the "read bar value, compute delta,
// write bar value" glue, since that part is inherently QWidget-specific.
#include "../global/macros.h"
#include "../map/coordinate.h"
#include "MapCanvasConfig.h"

#include <glm/glm.hpp>

#include <QObject>
#include <QPointer>

class QTimer;

class NODISCARD_QOBJECT MapScroller final : public QObject
{
    Q_OBJECT

private:
    // World<->scroll-unit conversion, driven by the known map bounds.
    struct NODISCARD KnownMapSize final
    {
        glm::ivec3 min{0};
        glm::ivec3 max{0};

        NODISCARD glm::ivec2 size() const { return glm::ivec2{max - min}; }

        NODISCARD glm::vec2 scrollToWorld(glm::ivec2 scrollPos) const;
        NODISCARD glm::ivec2 worldToScroll(glm::vec2 worldPos) const;
    } m_knownMapSize;

    QPointer<QTimer> m_scrollTimer;
    int m_horizontalScrollStep = 0;
    int m_verticalScrollStep = 0;
    int m_horizontalScrollMax = 0;
    int m_verticalScrollMax = 0;

public:
    explicit MapScroller(QObject *parent = nullptr);
    ~MapScroller() final;

public:
    NODISCARD glm::vec2 scrollToWorld(glm::ivec2 scrollPos) const
    {
        return m_knownMapSize.scrollToWorld(scrollPos);
    }
    NODISCARD glm::ivec2 worldToScroll(glm::vec2 worldPos) const
    {
        return m_knownMapSize.worldToScroll(worldPos);
    }

    NODISCARD int getHorizontalScrollMax() const { return m_horizontalScrollMax; }
    NODISCARD int getVerticalScrollMax() const { return m_verticalScrollMax; }

public slots:
    // Stores the known map bounds and recomputes the scrollbar ranges (min
    // is always 0).
    void slot_setScrollBars(Coordinate min, Coordinate max);

    // Starts/stops a 100ms-interval timer while a non-zero step is active,
    // and emits sig_continuousScrollStep() on every tick.
    void slot_continuousScroll(int hStep, int vStep);

signals:
    // Emitted every 100ms while a continuous scroll is active (see
    // slot_continuousScroll()).
    void sig_continuousScrollStep(int hStep, int vStep);

private slots:
    void slot_scrollTimerTimeout();
};
