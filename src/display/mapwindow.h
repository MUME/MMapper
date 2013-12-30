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

#include <QWidget>
#include "coordinate.h"

class QScrollBar;
class QGridLayout;
class QKeyEvent;
class QMouseEvent;
class MapCanvas;
class MapData;
class PrespammedPath;
class CGroup;

class MapWindow : public QWidget
{
    Q_OBJECT

public:
    MapWindow(MapData *mapData, PrespammedPath* pp, CGroup* gm, QWidget * parent = 0);
    ~MapWindow();

	void resizeEvent (QResizeEvent * event);

    MapCanvas* getCanvas() const;

signals:

	void setScroll(int, int);

public slots:
    void setScrollBars(const Coordinate &, const Coordinate &);

    void ensureVisible( qint32 x, qint32 y );
    void center( qint32 x, qint32 y );

	void mapMove(int, int);
	void continuousScroll(qint8, qint8);

	void verticalScroll(qint8);
	void horizontalScroll(qint8);

	void scrollTimerTimeout();

protected:
 //   void resizeEvent ( QResizeEvent * event );
 //   void paintEvent ( QPaintEvent * event );

	QTimer *scrollTimer;
	qint8 m_verticalScrollStep;
	qint8 m_horizontalScrollStep;

    QGridLayout *m_gridLayout;
    QScrollBar *m_horizontalScrollBar;
    QScrollBar *m_verticalScrollBar;
    MapCanvas *m_canvas;

    Coordinate m_scrollBarMinimumVisible;
    Coordinate m_scrollBarMaximumVisible;

private:
    QPoint mousePressPos;
    QPoint scrollBarValuesOnMousePress;
};


#endif
