#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <memory>
#include <QPoint>
#include <QString>
#include <QWidget>
#include <QtCore>
#include <QtGlobal>

#include "../expandoracommon/coordinate.h"
#include "../global/utils.h"
#include "mapcanvas.h"

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

class MapWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit MapWindow(MapData *mapData,
                       PrespammedPath *pp,
                       Mmapper2Group *gm,
                       QWidget *parent = nullptr);
    ~MapWindow() override;

public:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    MapCanvas *getCanvas() const;

signals:
    void sig_setScroll(const glm::vec2 &worldPos);
    void sig_zoomChanged(float zoom);

public slots:
    void setScrollBars(const Coordinate &, const Coordinate &);
    void centerOnWorldPos(const glm::vec2 &worldPos);
    void mapMove(int dx, int dy);
    void continuousScroll(int dx, int dy);

    void scrollTimerTimeout();
    void graphicsSettingsChanged();

public:
    void updateScrollBars();
    void setZoom(float zoom);
    float getZoom() const;

protected:
    std::unique_ptr<QTimer> scrollTimer;
    int m_verticalScrollStep = 0;
    int m_horizontalScrollStep = 0;

    std::unique_ptr<QGridLayout> m_gridLayout;
    std::unique_ptr<QScrollBar> m_horizontalScrollBar;
    std::unique_ptr<QScrollBar> m_verticalScrollBar;
    std::unique_ptr<MapCanvas> m_canvas;

private:
    QPoint mousePressPos;
    QPoint scrollBarValuesOnMousePress;
    struct NODISCARD KnownMapSize final
    {
        glm::ivec3 min{0};
        glm::ivec3 max{0};

        NODISCARD glm::ivec2 size() const { return glm::ivec2{max - min}; }

        NODISCARD glm::vec2 scrollToWorld(const glm::ivec2 &scrollPos) const;
        NODISCARD glm::ivec2 worldToScroll(const glm::vec2 &worldPos) const;

    } m_knownMapSize;

private:
    void centerOnScrollPos(const glm::ivec2 &scrollPos);
};
