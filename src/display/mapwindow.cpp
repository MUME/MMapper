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

#include "mapwindow.h"
#include "mapcanvas.h"

MapWindow::MapWindow(MapData *mapData, PrespammedPath* pp, CGroup* gm,  QWidget * parent)
    : QWidget(parent)
{
	m_verticalScrollStep = 0;
	m_horizontalScrollStep = 0;
	scrollTimer = NULL;

  m_gridLayout = new QGridLayout(this);
  m_gridLayout->setSpacing(0);
  m_gridLayout->setMargin(0);

  m_verticalScrollBar = new QScrollBar();
  m_verticalScrollBar->setOrientation(Qt::Vertical);
  m_verticalScrollBar->setRange(0, 0);
  m_verticalScrollBar->hide();

  m_gridLayout->addWidget(m_verticalScrollBar, 0, 1, 1, 1);

  m_horizontalScrollBar = new QScrollBar();
  m_horizontalScrollBar->setOrientation(Qt::Horizontal);
  m_horizontalScrollBar->setRange(0, 0);
  m_horizontalScrollBar->hide();

  m_gridLayout->addWidget(m_horizontalScrollBar, 1, 0, 1, 1);

  QGLFormat fmt;
  fmt.setDoubleBuffer( TRUE );
  fmt.setDirectRendering( TRUE );
  fmt.setRgba( TRUE );
  fmt.setDepth( TRUE );
  m_canvas = new MapCanvas(mapData, pp, gm, fmt, NULL);

  m_gridLayout->addWidget(m_canvas, 0, 0, 1, 1);

  connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)), m_canvas, SLOT(setHorizontalScroll(int)));
  connect(m_verticalScrollBar, SIGNAL(valueChanged(int)), m_canvas, SLOT(setVerticalScroll(int)));

  connect(m_canvas, SIGNAL(onEnsureVisible( qint32, qint32 )), this, SLOT(ensureVisible( qint32, qint32 )));
  connect(m_canvas, SIGNAL(onCenter( qint32, qint32 )), this, SLOT(center( qint32, qint32 )));

  connect(m_canvas, SIGNAL(setScrollBars(const Coordinate &, const Coordinate &)), this, SLOT(setScrollBars(const Coordinate &, const Coordinate &)));

  connect ( m_canvas, SIGNAL(continuousScroll(qint8, qint8)),  this, SLOT(continuousScroll(qint8, qint8)) );
  connect ( m_canvas, SIGNAL(mapMove(int, int)),  this, SLOT(mapMove(int, int)) );
  connect( this, SIGNAL(setScroll(int, int)), m_canvas, SLOT(setScroll(int, int)));
}

MapWindow::~MapWindow()
{
	if (m_canvas) delete m_canvas;
	if (m_gridLayout) delete m_gridLayout;
	if (m_verticalScrollBar) delete m_verticalScrollBar;
	if (m_horizontalScrollBar) delete m_horizontalScrollBar;
}
void MapWindow::mapMove(int dx, int dy)
{
	int hValue = m_horizontalScrollBar->value() + dx;
	int vValue = m_verticalScrollBar->value() + dy;

    disconnect(m_horizontalScrollBar, SIGNAL(valueChanged(int)), m_canvas, SLOT(setHorizontalScroll(int)));
    disconnect(m_verticalScrollBar, SIGNAL(valueChanged(int)), m_canvas, SLOT(setVerticalScroll(int)));

	m_horizontalScrollBar->setValue( hValue );
	m_verticalScrollBar->setValue( vValue );

    emit setScroll(hValue, vValue);

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)), m_canvas, SLOT(setHorizontalScroll(int)));
    connect(m_verticalScrollBar, SIGNAL(valueChanged(int)), m_canvas, SLOT(setVerticalScroll(int)));
}

void MapWindow::continuousScroll(qint8 hStep, qint8 vStep)
{
	m_horizontalScrollStep = hStep;
	m_verticalScrollStep = vStep;

	//stop
	if (scrollTimer && hStep==0 && vStep==0)
	{
		if (scrollTimer->isActive()) scrollTimer->stop();
		delete scrollTimer;
		scrollTimer = NULL;
	}

	//start
	if (!scrollTimer && (hStep!=0 || vStep!=0) )
	{
		scrollTimer = new QTimer(this);
		connect(scrollTimer, SIGNAL(timeout()), this, SLOT(scrollTimerTimeout()));
		scrollTimer->start(10);
	}
}



void MapWindow::scrollTimerTimeout()
{
	qint32 vValue = m_verticalScrollBar->value() + m_verticalScrollStep;
	qint32 hValue = m_horizontalScrollBar->value() + m_horizontalScrollStep;

    disconnect(m_horizontalScrollBar, SIGNAL(valueChanged(int)), m_canvas, SLOT(setHorizontalScroll(int)));
    disconnect(m_verticalScrollBar, SIGNAL(valueChanged(int)), m_canvas, SLOT(setVerticalScroll(int)));

	m_horizontalScrollBar->setValue( hValue );
	m_verticalScrollBar->setValue( vValue );

    emit setScroll(hValue, vValue);

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)), m_canvas, SLOT(setHorizontalScroll(int)));
    connect(m_verticalScrollBar, SIGNAL(valueChanged(int)), m_canvas, SLOT(setVerticalScroll(int)));
}

void MapWindow::verticalScroll(qint8 step)
{
  m_verticalScrollBar->setValue( m_verticalScrollBar->value() + step );
}

void MapWindow::horizontalScroll(qint8 step)
{
  m_horizontalScrollBar->setValue( m_horizontalScrollBar->value() + step );
}


void MapWindow::center( qint32 x, qint32 y )
{
  m_horizontalScrollBar->setValue( (qint32)(x / SCROLLFACTOR) );
  m_verticalScrollBar->setValue( (qint32)(y / SCROLLFACTOR) );
}



void MapWindow::ensureVisible( qint32 x, qint32 y )
{
  qint32 X1 = (qint32)(m_horizontalScrollBar->value()* SCROLLFACTOR - 5);
  qint32 Y1 = (qint32)(m_verticalScrollBar->value()* SCROLLFACTOR - 5);
  qint32 X2 = (qint32)(m_horizontalScrollBar->value()* SCROLLFACTOR + 5);
  qint32 Y2 = (qint32)(m_verticalScrollBar->value()* SCROLLFACTOR + 5);

  if (x > X2 - 1 )
  {
    m_horizontalScrollBar->setValue( (qint32)((x - 2)/ SCROLLFACTOR) );
  }
  else
  if (x < X1 + 1 )
  {
    m_horizontalScrollBar->setValue( (qint32)((x + 2)/ SCROLLFACTOR) );
  }

  if (y > Y2 - 1 )
  {
    m_verticalScrollBar->setValue( (qint32)((y - 2)/ SCROLLFACTOR) );
  }
  else
  if (y < Y1 + 1 )
  {
    m_verticalScrollBar->setValue( (qint32)((y + 2)/ SCROLLFACTOR) );
  }

  bool scrollsNeedsUpdate = false;
  if (x < m_scrollBarMinimumVisible.x)
  {
  	m_scrollBarMinimumVisible.x = x;
  	scrollsNeedsUpdate = true;
  }
  if (x > m_scrollBarMaximumVisible.x)
  {
  	m_scrollBarMaximumVisible.x = x;
  	scrollsNeedsUpdate = true;
  }
  if (y < m_scrollBarMinimumVisible.y)
  {
  	m_scrollBarMinimumVisible.y = y;
  	scrollsNeedsUpdate = true;
  }
  if (y > m_scrollBarMaximumVisible.y)
  {
  	m_scrollBarMaximumVisible.y = y;
  	scrollsNeedsUpdate = true;
  }

  if(scrollsNeedsUpdate)
	setScrollBars(m_scrollBarMinimumVisible, m_scrollBarMaximumVisible);
}

void MapWindow::resizeEvent (QResizeEvent *)
{
	setScrollBars(m_scrollBarMinimumVisible, m_scrollBarMaximumVisible);
}

void MapWindow::setScrollBars(const Coordinate & ulf, const Coordinate & lrb)
{
  m_scrollBarMinimumVisible = ulf;
  m_scrollBarMaximumVisible = lrb;
  float dwp = m_canvas->getDW();
  float dhp = m_canvas->getDH();

  int horizontalMax = (int)(((float)lrb.x - dwp/2 + 1.5) / SCROLLFACTOR);
  int horizontalMin = (int)(((float)ulf.x + dwp/2 - 2.0) / SCROLLFACTOR);
  int verticalMax= (int)(((float)lrb.y - dhp/2 + 1.5) / SCROLLFACTOR);
  int verticalMin = (int)(((float)ulf.y + dhp/2 - 2.0) / SCROLLFACTOR);

  m_horizontalScrollBar->setRange(horizontalMin, horizontalMax );
  if (lrb.x - ulf.x + 1 > dwp) m_horizontalScrollBar->show();
  else m_horizontalScrollBar->hide();

  m_verticalScrollBar->setRange(verticalMin, verticalMax );
  if (lrb.y - ulf.y + 1 > dhp) m_verticalScrollBar->show();
  else m_verticalScrollBar->hide();

}


