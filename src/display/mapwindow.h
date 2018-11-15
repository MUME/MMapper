#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#ifndef MAPWINDOW_H
#define MAPWINDOW_H

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

#endif
