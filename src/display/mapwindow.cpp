// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mapwindow.h"

#include <memory>
#include <QGridLayout>
#include <QScrollBar>

#include "../global/SignalBlocker.h"
#include "mapcanvas.h"

class QResizeEvent;

MapWindow::MapWindow(MapData *const mapData,
                     PrespammedPath *const pp,
                     Mmapper2Group *const gm,
                     QWidget *const parent)
    : QWidget(parent)
{
    m_gridLayout = std::make_unique<QGridLayout>(this);
    m_gridLayout->setSpacing(0);
    m_gridLayout->setMargin(0);

    m_verticalScrollBar = std::make_unique<QScrollBar>(this);
    m_verticalScrollBar->setOrientation(Qt::Vertical);
    m_verticalScrollBar->setRange(0, 0);
    m_verticalScrollBar->hide();
    m_verticalScrollBar->setSingleStep(MapCanvas::SCROLL_SCALE);

    m_gridLayout->addWidget(m_verticalScrollBar.get(), 0, 1, 1, 1);

    m_horizontalScrollBar = std::make_unique<QScrollBar>(this);
    m_horizontalScrollBar->setOrientation(Qt::Horizontal);
    m_horizontalScrollBar->setRange(0, 0);
    m_horizontalScrollBar->hide();
    m_horizontalScrollBar->setSingleStep(MapCanvas::SCROLL_SCALE);

    m_gridLayout->addWidget(m_horizontalScrollBar.get(), 1, 0, 1, 1);

    m_canvas = std::make_unique<MapCanvas>(mapData, pp, gm, this);
    MapCanvas *const canvas = m_canvas.get();

    m_gridLayout->addWidget(canvas, 0, 0, 1, 1);

    // from map window to canvas
    {
        connect(m_horizontalScrollBar.get(),
                &QScrollBar::valueChanged,
                canvas,
                [this](const int x) -> void {
                    const float val = m_knownMapSize.scrollToWorld(glm::ivec2{x, 0}).x;
                    m_canvas->setHorizontalScroll(val);
                });

        connect(m_verticalScrollBar.get(),
                &QScrollBar::valueChanged,
                canvas,
                [this](const int y) -> void {
                    const float value = m_knownMapSize.scrollToWorld(glm::ivec2{0, y}).y;
                    m_canvas->setVerticalScroll(value);
                });

        connect(this, &MapWindow::sig_setScroll, canvas, &MapCanvas::setScroll);
    }

    // from canvas to map window
    {
        connect(canvas, &MapCanvas::sig_onCenter, this, &MapWindow::centerOnWorldPos);
        connect(canvas, &MapCanvas::sig_setScrollBars, this, &MapWindow::setScrollBars);
        connect(canvas, &MapCanvas::sig_continuousScroll, this, &MapWindow::continuousScroll);
        connect(canvas, &MapCanvas::sig_mapMove, this, &MapWindow::mapMove);
        connect(canvas, &MapCanvas::sig_zoomChanged, this, &MapWindow::sig_zoomChanged);
    }
}

void MapWindow::keyPressEvent(QKeyEvent *const event)
{
    if (event->key() == Qt::Key_Escape) {
        m_canvas->userPressedEscape(true);
        return;
    }
    QWidget::keyPressEvent(event);
}

void MapWindow::keyReleaseEvent(QKeyEvent *const event)
{
    if (event->key() == Qt::Key_Escape) {
        m_canvas->userPressedEscape(false);
        return;
    }
    QWidget::keyReleaseEvent(event);
}

MapWindow::~MapWindow() = default;

void MapWindow::mapMove(const int dx, const int input_dy)
{
    // Y is negated because delta is in world space
    const int dy = -input_dy;

    const SignalBlocker block_horz{*m_horizontalScrollBar};
    const SignalBlocker block_vert{*m_verticalScrollBar};

    const int hValue = m_horizontalScrollBar->value() + dx;
    const int vValue = m_verticalScrollBar->value() + dy;

    const glm::ivec2 scrollPos{hValue, vValue};
    centerOnScrollPos(scrollPos);
}

// REVISIT: This looks more like "delayed jump" than "continuous scroll."
void MapWindow::continuousScroll(const int hStep, const int input_vStep)
{
    const auto fitsInInt8 = [](int n) -> bool {
        // alternate: test against std::numeric_limits<int8_t>::min and max.
        return static_cast<int>(static_cast<int8_t>(n)) == n;
    };

    // code originally used qint8
    assert(fitsInInt8(hStep));
    assert(fitsInInt8(input_vStep));

    // Y is negated because delta is in world space
    const int vStep = -input_vStep;

    m_horizontalScrollStep = hStep;
    m_verticalScrollStep = vStep;

    // stop
    if ((scrollTimer != nullptr) && hStep == 0 && vStep == 0) {
        if (scrollTimer->isActive()) {
            scrollTimer->stop();
        }
        scrollTimer.reset();
    }

    // start
    if ((scrollTimer == nullptr) && (hStep != 0 || vStep != 0)) {
        scrollTimer = std::make_unique<QTimer>(this);
        connect(scrollTimer.get(), &QTimer::timeout, this, &MapWindow::scrollTimerTimeout);
        scrollTimer->start(100);
    }
}

void MapWindow::scrollTimerTimeout()
{
    const SignalBlocker block_horz{*m_horizontalScrollBar};
    const SignalBlocker block_vert{*m_verticalScrollBar};

    const int vValue = m_verticalScrollBar->value() + m_verticalScrollStep;
    const int hValue = m_horizontalScrollBar->value() + m_horizontalScrollStep;

    const glm::ivec2 scrollPos{hValue, vValue};
    centerOnScrollPos(scrollPos);
}

void MapWindow::graphicsSettingsChanged()
{
    this->m_canvas->graphicsSettingsChanged();
}

void MapWindow::centerOnWorldPos(const glm::vec2 &worldPos)
{
    // CHANGE: previously this function didn't emit a signal.
    const auto scrollPos = m_knownMapSize.worldToScroll(worldPos);
    centerOnScrollPos(scrollPos);
}

void MapWindow::centerOnScrollPos(const glm::ivec2 &scrollPos)
{
    m_horizontalScrollBar->setValue(scrollPos.x);
    m_verticalScrollBar->setValue(scrollPos.y);

    const auto worldPos = m_knownMapSize.scrollToWorld(scrollPos);
    emit sig_setScroll(worldPos);
}

void MapWindow::resizeEvent(QResizeEvent * /*event*/)
{
    updateScrollBars();
}

void MapWindow::setScrollBars(const Coordinate &min, const Coordinate &max)
{
    m_knownMapSize.min = min.to_ivec3();
    m_knownMapSize.max = max.to_ivec3();
    updateScrollBars();
}

void MapWindow::updateScrollBars()
{
    const auto dims = m_knownMapSize.size() * MapCanvas::SCROLL_SCALE;

    m_horizontalScrollBar->setRange(0, dims.x);
    if (dims.x > 0) {
        m_horizontalScrollBar->show();
    } else {
        m_horizontalScrollBar->hide();
    }

    m_verticalScrollBar->setRange(0, dims.y);
    if (dims.y > 0) {
        m_verticalScrollBar->show();
    } else {
        m_verticalScrollBar->hide();
    }
}

MapCanvas *MapWindow::getCanvas() const
{
    return m_canvas.get();
}

void MapWindow::setZoom(const float zoom)
{
    m_canvas->setZoom(zoom);
}

float MapWindow::getZoom() const
{
    return m_canvas->getRawZoom();
}

glm::vec2 MapWindow::KnownMapSize::scrollToWorld(const glm::ivec2 &scrollPos) const
{
    auto worldPos = glm::vec2{scrollPos} / static_cast<float>(MapCanvas::SCROLL_SCALE);
    worldPos.y = static_cast<float>(size().y) - worldPos.y; // negate Y
    worldPos += glm::vec2{min};
    return worldPos;
}

glm::ivec2 MapWindow::KnownMapSize::worldToScroll(const glm::vec2 &worldPos_in) const
{
    auto worldPos = worldPos_in;
    worldPos -= glm::vec2{min};
    worldPos.y = static_cast<float>(size().y) - worldPos.y; // negate Y
    const glm::ivec2 scrollPos{worldPos * static_cast<float>(MapCanvas::SCROLL_SCALE)};
    return scrollPos;
}
