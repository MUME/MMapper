#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QPoint>
#include <QString>
#include <QWidget>
#include <QtCore>
#include <QtGlobal>

#include "../expandoracommon/coordinate.h"

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

class MapWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MapWindow(MapData *mapData,
                       PrespammedPath *pp,
                       Mmapper2Group *gm,
                       QWidget *parent = nullptr);
    ~MapWindow() override;

    void resizeEvent(QResizeEvent *event) override;

    MapCanvas *getCanvas() const;

signals:

    void setScroll(int, int);

public slots:
    void setScrollBars(const Coordinate &, const Coordinate &);

    void ensureVisible(qint32 x, qint32 y);
    void center(qint32 x, qint32 y);

    void mapMove(int, int);
    void continuousScroll(qint8, qint8);

    void verticalScroll(qint8);
    void horizontalScroll(qint8);

    void scrollTimerTimeout();

protected:
    //   void resizeEvent ( QResizeEvent * event );
    //   void paintEvent ( QPaintEvent * event );

    QTimer *scrollTimer = nullptr;
    qint8 m_verticalScrollStep = 0;
    qint8 m_horizontalScrollStep = 0;

    QGridLayout *m_gridLayout = nullptr;
    QScrollBar *m_horizontalScrollBar = nullptr;
    QScrollBar *m_verticalScrollBar = nullptr;
    MapCanvas *m_canvas = nullptr;

    Coordinate m_scrollBarMinimumVisible{};
    Coordinate m_scrollBarMaximumVisible{};

private:
    QPoint mousePressPos{};
    QPoint scrollBarValuesOnMousePress{};
};
