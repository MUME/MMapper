// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mapwindow.h"

#include <QGridLayout>
#include <QScrollBar>

#include "mapcanvas.h"

class QResizeEvent;

MapWindow::MapWindow(MapData *mapData, PrespammedPath *pp, Mmapper2Group *gm, QWidget *parent)
    : QWidget(parent)
{
    m_verticalScrollStep = 0;
    m_horizontalScrollStep = 0;
    scrollTimer = nullptr;

    m_gridLayout = new QGridLayout(this);
    m_gridLayout->setSpacing(0);
    m_gridLayout->setMargin(0);

    m_verticalScrollBar = new QScrollBar(this);
    m_verticalScrollBar->setOrientation(Qt::Vertical);
    m_verticalScrollBar->setRange(0, 0);
    m_verticalScrollBar->hide();

    m_gridLayout->addWidget(m_verticalScrollBar, 0, 1, 1, 1);

    m_horizontalScrollBar = new QScrollBar(this);
    m_horizontalScrollBar->setOrientation(Qt::Horizontal);
    m_horizontalScrollBar->setRange(0, 0);
    m_horizontalScrollBar->hide();

    m_gridLayout->addWidget(m_horizontalScrollBar, 1, 0, 1, 1);

    m_canvas = new MapCanvas(mapData, pp, gm, this);

    m_gridLayout->addWidget(m_canvas, 0, 0, 1, 1);

    connect(m_horizontalScrollBar,
            &QAbstractSlider::valueChanged,
            m_canvas,
            &MapCanvas::setHorizontalScroll);
    connect(m_verticalScrollBar,
            &QAbstractSlider::valueChanged,
            m_canvas,
            &MapCanvas::setVerticalScroll);

    connect(m_canvas, &MapCanvas::onEnsureVisible, this, &MapWindow::ensureVisible);
    connect(m_canvas, &MapCanvas::onCenter, this, &MapWindow::center);

    connect(m_canvas, &MapCanvas::setScrollBars, this, &MapWindow::setScrollBars);

    connect(m_canvas, &MapCanvas::continuousScroll, this, &MapWindow::continuousScroll);
    connect(m_canvas, &MapCanvas::mapMove, this, &MapWindow::mapMove);
    connect(this, &MapWindow::setScroll, m_canvas, &MapCanvas::setScroll);
}

MapWindow::~MapWindow()
{
    delete m_canvas;
    delete m_gridLayout;
    delete m_verticalScrollBar;
    delete m_horizontalScrollBar;
}

void MapWindow::mapMove(int dx, int dy)
{
    int hValue = m_horizontalScrollBar->value() + dx;
    int vValue = m_verticalScrollBar->value() + dy;

    disconnect(m_horizontalScrollBar,
               &QAbstractSlider::valueChanged,
               m_canvas,
               &MapCanvas::setHorizontalScroll);
    disconnect(m_verticalScrollBar,
               &QAbstractSlider::valueChanged,
               m_canvas,
               &MapCanvas::setVerticalScroll);

    m_horizontalScrollBar->setValue(hValue);
    m_verticalScrollBar->setValue(vValue);

    emit setScroll(hValue, vValue);

    connect(m_horizontalScrollBar,
            &QAbstractSlider::valueChanged,
            m_canvas,
            &MapCanvas::setHorizontalScroll);
    connect(m_verticalScrollBar,
            &QAbstractSlider::valueChanged,
            m_canvas,
            &MapCanvas::setVerticalScroll);
}

void MapWindow::continuousScroll(qint8 hStep, qint8 vStep)
{
    m_horizontalScrollStep = hStep;
    m_verticalScrollStep = vStep;

    // stop
    if ((scrollTimer != nullptr) && hStep == 0 && vStep == 0) {
        if (scrollTimer->isActive()) {
            scrollTimer->stop();
        }
        delete scrollTimer;
        scrollTimer = nullptr;
    }

    // start
    if ((scrollTimer == nullptr) && (hStep != 0 || vStep != 0)) {
        scrollTimer = new QTimer(this);
        connect(scrollTimer, &QTimer::timeout, this, &MapWindow::scrollTimerTimeout);
        scrollTimer->start(10);
    }
}

void MapWindow::scrollTimerTimeout()
{
    qint32 vValue = m_verticalScrollBar->value() + m_verticalScrollStep;
    qint32 hValue = m_horizontalScrollBar->value() + m_horizontalScrollStep;

    disconnect(m_horizontalScrollBar,
               &QAbstractSlider::valueChanged,
               m_canvas,
               &MapCanvas::setHorizontalScroll);
    disconnect(m_verticalScrollBar,
               &QAbstractSlider::valueChanged,
               m_canvas,
               &MapCanvas::setVerticalScroll);

    m_horizontalScrollBar->setValue(hValue);
    m_verticalScrollBar->setValue(vValue);

    emit setScroll(hValue, vValue);

    connect(m_horizontalScrollBar,
            &QAbstractSlider::valueChanged,
            m_canvas,
            &MapCanvas::setHorizontalScroll);
    connect(m_verticalScrollBar,
            &QAbstractSlider::valueChanged,
            m_canvas,
            &MapCanvas::setVerticalScroll);
}

void MapWindow::verticalScroll(qint8 step)
{
    m_verticalScrollBar->setValue(m_verticalScrollBar->value() + step);
}

void MapWindow::horizontalScroll(qint8 step)
{
    m_horizontalScrollBar->setValue(m_horizontalScrollBar->value() + step);
}

void MapWindow::center(qint32 x, qint32 y)
{
    const float scrollfactor = MapCanvas::SCROLLFACTOR();
    m_horizontalScrollBar->setValue(static_cast<qint32>(static_cast<float>(x) / scrollfactor));
    m_verticalScrollBar->setValue(static_cast<qint32>(static_cast<float>(y) / scrollfactor));
}

void MapWindow::ensureVisible(qint32 x, qint32 y)
{
    const float scrollfactor = MapCanvas::SCROLLFACTOR();
    const float horizontalValue = static_cast<float>(m_horizontalScrollBar->value()) * scrollfactor;
    const float verticalValue = static_cast<float>(m_verticalScrollBar->value()) * scrollfactor;
    const auto X1 = static_cast<qint32>(horizontalValue - 5.0f);
    const auto Y1 = static_cast<qint32>(verticalValue - 5.0f);
    const auto X2 = static_cast<qint32>(horizontalValue + 5.0f);
    const auto Y2 = static_cast<qint32>(verticalValue + 5.0f);

    if (x > X2 - 1) {
        m_horizontalScrollBar->setValue(
            static_cast<qint32>(static_cast<float>(x - 2) / scrollfactor));
    } else if (x < X1 + 1) {
        m_horizontalScrollBar->setValue(
            static_cast<qint32>(static_cast<float>(x + 2) / scrollfactor));
    }

    if (y > Y2 - 1) {
        m_verticalScrollBar->setValue(static_cast<qint32>(static_cast<float>(y - 2) / scrollfactor));
    } else if (y < Y1 + 1) {
        m_verticalScrollBar->setValue(static_cast<qint32>(static_cast<float>(y + 2) / scrollfactor));
    }

    bool scrollsNeedsUpdate = false;
    if (x < m_scrollBarMinimumVisible.x) {
        m_scrollBarMinimumVisible.x = x;
        scrollsNeedsUpdate = true;
    }
    if (x > m_scrollBarMaximumVisible.x) {
        m_scrollBarMaximumVisible.x = x;
        scrollsNeedsUpdate = true;
    }
    if (y < m_scrollBarMinimumVisible.y) {
        m_scrollBarMinimumVisible.y = y;
        scrollsNeedsUpdate = true;
    }
    if (y > m_scrollBarMaximumVisible.y) {
        m_scrollBarMaximumVisible.y = y;
        scrollsNeedsUpdate = true;
    }

    if (scrollsNeedsUpdate) {
        setScrollBars(m_scrollBarMinimumVisible, m_scrollBarMaximumVisible);
    }
}

void MapWindow::resizeEvent(QResizeEvent * /*event*/)
{
    setScrollBars(m_scrollBarMinimumVisible, m_scrollBarMaximumVisible);
}

void MapWindow::setScrollBars(const Coordinate &min, const Coordinate &max)
{
    m_scrollBarMinimumVisible = min;
    m_scrollBarMaximumVisible = max;

    // TODO: clean up all this code duplication.

    const float dwp = m_canvas->getDW();
    const float dhp = m_canvas->getDH();

    const float scrollfactor = MapCanvas::SCROLLFACTOR();
    const auto horizontalMax = static_cast<int>((static_cast<float>(max.x) - dwp / 2.0f + 1.5f)
                                                / scrollfactor);
    const auto horizontalMin = static_cast<int>((static_cast<float>(min.x) + dwp / 2.0f - 2.0f)
                                                / scrollfactor);
    const auto verticalMax = static_cast<int>((static_cast<float>(max.y) - dhp / 2.0f + 1.5f)
                                              / scrollfactor);
    const auto verticalMin = static_cast<int>((static_cast<float>(min.y) + dhp / 2.0f - 2.0f)
                                              / scrollfactor);

    m_horizontalScrollBar->setRange(horizontalMin, horizontalMax);
    if (static_cast<float>(max.x - min.x + 1) > dwp) {
        m_horizontalScrollBar->show();
    } else {
        m_horizontalScrollBar->hide();
    }

    m_verticalScrollBar->setRange(verticalMin, verticalMax);
    if (static_cast<float>(max.y - min.y + 1) > dhp) {
        m_verticalScrollBar->show();
    } else {
        m_verticalScrollBar->hide();
    }
}

MapCanvas *MapWindow::getCanvas() const
{
    return m_canvas;
}
