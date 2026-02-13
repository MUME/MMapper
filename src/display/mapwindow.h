#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../map/coordinate.h"
#include "mapcanvas.h"

#include <QLabel>
#include <QPixmap>
#include <QPoint>
#include <QPointer>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QtCore>
#include <QtGlobal>

class MapCanvas;
class MapData;
class Mmapper2Group;
class PrespammedPath;
class QGridLayout;
class QKeyEvent;
class QMouseEvent;
class QObject;
class QResizeEvent;
class QScrollBar;
class QTimer;

class NODISCARD_QOBJECT MapWindow final : public QWidget
{
    Q_OBJECT

protected:
    QPointer<QGridLayout> m_gridLayout;
    QPointer<QScrollBar> m_horizontalScrollBar;
    QPointer<QScrollBar> m_verticalScrollBar;
    QPointer<MapCanvas> m_canvas;
    QPointer<QWidget> m_canvasContainer;
    QPointer<QLabel> m_splashLabel;
    QPointer<QTimer> m_scrollTimer;
    int m_verticalScrollStep = 0;
    int m_horizontalScrollStep = 0;

private:
    struct NODISCARD KnownMapSize final
    {
        glm::ivec3 min{0};
        glm::ivec3 max{0};

        NODISCARD glm::ivec2 size() const { return glm::ivec2{max - min}; }

        NODISCARD glm::vec2 scrollToWorld(const glm::ivec2 &scrollPos) const;
        NODISCARD glm::ivec2 worldToScroll(const glm::vec2 &worldPos) const;

    } m_knownMapSize;

public:
    explicit MapWindow(MapData &mapData, PrespammedPath &pp, Mmapper2Group &gm, QWidget *parent);
    ~MapWindow() final;

public:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    NODISCARD MapCanvas *getCanvas() const;

public:
    void updateScrollBars();
    void setZoom(float zoom);
    NODISCARD float getZoom() const;
    void hideSplashImage();

private:
    void centerOnScrollPos(const glm::ivec2 &scrollPos);

signals:
    void sig_setScroll(const glm::vec2 &worldPos);
    void sig_zoomChanged(float zoom);

public slots:
    void slot_setScrollBars(const Coordinate &, const Coordinate &);
    void slot_centerOnWorldPos(const glm::vec2 &worldPos);
    void slot_mapMove(int dx, int dy);
    void slot_continuousScroll(int dx, int dy);
    void slot_scrollTimerTimeout();
    void slot_graphicsSettingsChanged();
    void slot_zoomChanged(const float zoom) { emit sig_zoomChanged(zoom); }
    void slot_showTooltip(const QString &text, const QPoint &pos);

    void setCanvasEnabled(bool enabled);
};
