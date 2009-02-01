/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper2 project.
** Maintained by Marek Krejza <krejza@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file COPYING included in the packaging of
** this file.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
*************************************************************************/

#ifndef MAPWINDOW_H
#define MAPWINDOW_H

#include <QtGui>
#include <QtOpenGL>
#include "coordinate.h"

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

    MapCanvas* getCanvas(){return m_canvas;};

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
