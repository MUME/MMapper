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

#include "mapcanvas.h"
#include "mapdata.h"
#include "prespammedpath.h"
#include "room.h"
#include "mmapper2room.h"
#include "mmapper2exit.h"
#include "mmapper2event.h"
#include "coordinate.h"
#include "roomselection.h"
#include "connectionselection.h"
#include "infomarkseditdlg.h"
#include "configuration.h"
#include "abstractparser.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "customaction.h"

#include <assert.h>
#include <math.h>

#include <QWheelEvent>

#define ROOM_Z_DISTANCE (7.0f)
#define ROOM_WALL_ALIGN (0.008f)
#define BASESIZEX 528       // base canvas size
#define BASESIZEY 528

GLubyte halftone[] = {
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55};


  GLubyte quadtoneline = 0x88;
  GLubyte quadtone[] = {
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22};


MapCanvas::MapCanvas( MapData *mapData, PrespammedPath* prespammedPath, CGroup* groupManager, const QGLFormat & fmt, QWidget * parent )
  : QGLWidget(fmt, parent)
  {
    m_scrollX = 0;
    m_scrollY = 0;

    m_currentLayer = 0;

        //no area selected at start time
    m_selectedArea = false;

    m_infoMarkSelection = false;

    m_canvasMouseMode = CMM_MOVE;

    m_roomSelection = NULL;
    m_connectionSelection = NULL;

    m_mouseRightPressed = false;
    m_mouseLeftPressed = false;
    m_altPressed = false;
    m_ctrlPressed = false;


    m_roomShadowPixmap = new QPixmap(30,30);
    m_roomShadowPixmap->fill(QColor(0,0,0,175));

    m_scaleFactor = 1.0f;  //scale rooms

    m_glFont = new QFont(QFont(),this);
    m_glFont->setStyleHint(QFont::System, QFont::OpenGLCompatible);

    m_glFont->setStretch(QFont::Unstretched);

    m_glFontMetrics = new QFontMetrics(*m_glFont);

    m_firstDraw = true;

    m_data = mapData;
    m_prespammedPath = prespammedPath;
    m_groupManager = groupManager;

    m_infoMarksEditDlg = new InfoMarksEditDlg(mapData, this);
    connect(m_infoMarksEditDlg, SIGNAL(mapChanged()), this, SLOT(update()));
    connect(m_infoMarksEditDlg, SIGNAL(closeEventReceived()), this, SLOT(onInfoMarksEditDlgClose()));

    for (int i=0; i<16; i++)
    {
      m_terrainPixmaps[i] = new QPixmap(QString(":/pixmaps/terrain%1.png").arg(i));
      m_roadPixmaps[i] = new QPixmap(QString(":/pixmaps/road%1.png").arg(i));
      m_loadPixmaps[i] = new QPixmap(QString(":/pixmaps/load%1.png").arg(i));
      m_mobPixmaps[i] = new QPixmap(QString(":/pixmaps/mob%1.png").arg(i));
      m_trailPixmaps[i] = new QPixmap(QString(":/pixmaps/trail%1.png").arg(i));
    }
    m_updatePixmap[0] = new QPixmap(QString(":/pixmaps/update0.png"));
  }


  void MapCanvas::onInfoMarksEditDlgClose()
  {
    m_infoMarkSelection = false;
    updateGL();
  }

  MapCanvas::~MapCanvas()
  {
    for (int i=0; i<16; i++)
    {
      if (m_terrainPixmaps[i]) delete m_terrainPixmaps[i];
      if (m_roadPixmaps[i]) delete m_roadPixmaps[i];
      if (m_loadPixmaps[i]) delete m_loadPixmaps[i];
      if (m_mobPixmaps[i]) delete m_mobPixmaps[i];
    }
    if (m_updatePixmap[0]) delete m_updatePixmap[0];

    if (m_roomSelection) m_data->unselect(m_roomSelection);
    if (m_connectionSelection) delete m_connectionSelection;
    if ( m_glFont ) delete m_glFont;
    if ( m_glFontMetrics ) delete m_glFontMetrics;
  }
/*
  void MapCanvas::closeEvent(QCloseEvent *event){
  if ( m_infoMarksEditDlg )
  {
  m_infoMarksEditDlg->hide();
  delete m_infoMarksEditDlg;
}
}
*/

float MapCanvas::SCROLLFACTOR()
{
    return 0.2f;
}

  void MapCanvas::layerUp()
  {
    m_currentLayer++;
    updateGL();
  }

  void MapCanvas::layerDown()
  {
    m_currentLayer--;
    updateGL();
  }

  int MapCanvas::GLtoMap(double arg)
  {
    if (arg >=0)
      return (int)(arg+0.5f);
    return (int)(arg-0.5f);
  }

  void MapCanvas::setCanvasMouseMode(CanvasMouseMode mode)
  {
    m_canvasMouseMode = mode;

    if (m_canvasMouseMode != CMM_EDIT_INFOMARKS)
    {
      m_infoMarksEditDlg->hide();
      m_infoMarkSelection = false;
    }

    clearRoomSelection();
    clearConnectionSelection();

    m_selectedArea = false;
    updateGL();
  }

  void MapCanvas::clearRoomSelection()
  {
    if (m_roomSelection) m_data->unselect(m_roomSelection);
    m_roomSelection = NULL;
    emit newRoomSelection(m_roomSelection);
  }

  void MapCanvas::clearConnectionSelection()
  {
    if (m_connectionSelection) delete m_connectionSelection;
    m_connectionSelection = NULL;
    emit newConnectionSelection(m_connectionSelection);
  }

  void MapCanvas::wheelEvent ( QWheelEvent * event )
  {
    if (event->delta() > 100)
    {
      switch (m_canvasMouseMode)
      {
        case CMM_MOVE:
          zoomIn();
          break;
        case CMM_EDIT_INFOMARKS:
        case CMM_SELECT_ROOMS:
        case CMM_SELECT_CONNECTIONS:
        case CMM_CREATE_ROOMS:
        case CMM_CREATE_CONNECTIONS:
        case CMM_CREATE_ONEWAY_CONNECTIONS:
          layerDown();
          break;
          default:;
      }
    }

    if (event->delta() < -100)
    {
      switch (m_canvasMouseMode)
      {
        case CMM_MOVE:
          zoomOut();
          break;
        case CMM_EDIT_INFOMARKS:
        case CMM_SELECT_ROOMS:
        case CMM_SELECT_CONNECTIONS:
        case CMM_CREATE_ROOMS:
        case CMM_CREATE_CONNECTIONS:
        case CMM_CREATE_ONEWAY_CONNECTIONS:
          layerUp();
          break;
          default:;
      }
    }

  }

  void MapCanvas::forceMapperToRoom() {
    const RoomSelection *tmpSel = NULL;
    if (m_roomSelection) {
      tmpSel = m_roomSelection;
      m_roomSelection = 0;
    }
    else {
      tmpSel = m_data->select(Coordinate(GLtoMap(m_selX1), GLtoMap(m_selY1), m_selLayer1));
    }
    if(tmpSel->size() == 1)
    {
      if (Config().m_mapMode == 2)
      {
        const Room *r = tmpSel->values().front();
        emit charMovedEvent(createEvent( CID_UNKNOWN, getName(r), getDynamicDescription(r), getDescription(r), 0, 0));
      }
      else
        emit setCurrentRoom(tmpSel->keys().front());
    }
    m_data->unselect(tmpSel);
    updateGL();
  }

  void MapCanvas::mousePressEvent(QMouseEvent *event)
  {
    GLdouble winX = (GLdouble)event->pos().x();
    GLdouble winY = (GLdouble)m_viewport[3] - (GLdouble)event->pos().y();
    GLdouble winZ = 0.978f;
    GLdouble tmp;
    UnProject((GLdouble) winX, (GLdouble) winY, (GLdouble) winZ,
               &m_selX1, &m_selY1, &tmp);
    m_selX2 = m_selX1;
    m_selY2 = m_selY1;
    m_selLayer1 = m_currentLayer;
    m_selLayer2 = m_currentLayer;

    if (event->buttons() & Qt::LeftButton) {
      m_mouseLeftPressed = true;
    }
    if (event->buttons() & Qt::RightButton) {
      m_mouseRightPressed = true;
    }


    switch (m_canvasMouseMode)
    {
      case CMM_EDIT_INFOMARKS:
        m_infoMarkSelection = true;
        m_infoMarksEditDlg->hide();
        updateGL();
        break;
      case CMM_MOVE:
        if (event->buttons() & Qt::LeftButton) {
          m_moveX1backup = m_selX1;
          m_moveY1backup = m_selY1;
        }
        break;

      case CMM_SELECT_ROOMS:

        if ((event->buttons() & Qt::LeftButton) &&
             (event->modifiers() & Qt::CTRL ) &&
             (event->modifiers() & Qt::ALT ) )
        {
          if (m_roomSelection) {
            m_data->unselect(m_roomSelection);
            m_roomSelection = 0;
          }
          m_ctrlPressed = true;
          m_altPressed = true;
          forceMapperToRoom();
          break;
        }

        if (event->buttons() & Qt::RightButton) {
          m_selectedArea = false;
          if (m_roomSelection != NULL) m_data->unselect(m_roomSelection);
          m_roomSelection = NULL;
          emit newRoomSelection(m_roomSelection);
        }

        if (event->buttons() & Qt::LeftButton) {

          if (event->modifiers() != Qt::CTRL)
          {
            const RoomSelection *tmpSel = m_data->select(Coordinate(GLtoMap(m_selX1), GLtoMap(m_selY1), m_selLayer1));
            if(m_roomSelection && tmpSel->size() > 0 && m_roomSelection->contains( tmpSel->keys().front() ))
            {
              m_roomSelectionMove = true;
              m_roomSelectionMoveX = 0;
              m_roomSelectionMoveY = 0;
              m_roomSelectionMoveWrongPlace = false;
            }
            else
            {
              m_roomSelectionMove = false;
              m_selectedArea = false;
              if (m_roomSelection != NULL) m_data->unselect(m_roomSelection);
              m_roomSelection = NULL;
              emit newRoomSelection(m_roomSelection);
            }
            m_data->unselect(tmpSel);
          }
          else
          {
            m_ctrlPressed = true;
          }
        }
        updateGL();
        break;

      case CMM_CREATE_ONEWAY_CONNECTIONS:
      case CMM_CREATE_CONNECTIONS:
        if (event->buttons() & Qt::LeftButton) {
          if (m_connectionSelection != NULL) delete m_connectionSelection;
          m_connectionSelection = new ConnectionSelection(m_data, m_selX1, m_selY1, m_selLayer1);
          if (!m_connectionSelection->isFirstValid())
          {
            if (m_connectionSelection != NULL) delete m_connectionSelection;
            m_connectionSelection = NULL;
          }
          emit newConnectionSelection(NULL);
        }

        if (event->buttons() & Qt::RightButton) {
          if (m_connectionSelection != NULL) delete m_connectionSelection;
          m_connectionSelection = NULL;

          emit newConnectionSelection(m_connectionSelection);
        }
        updateGL();
        break;

      case CMM_SELECT_CONNECTIONS:
        if (event->buttons() & Qt::LeftButton) {
          if (m_connectionSelection != NULL) delete m_connectionSelection;
          m_connectionSelection = new ConnectionSelection(m_data, m_selX1, m_selY1, m_selLayer1);
          if (!m_connectionSelection->isFirstValid())
          {
            if (m_connectionSelection != NULL) delete m_connectionSelection;
            m_connectionSelection = NULL;
          }
          else
          {
            const Room *r1 = m_connectionSelection->getFirst().room;
            ExitDirection dir1 = m_connectionSelection->getFirst().direction;

            if ( r1->exit(dir1).outBegin() == r1->exit(dir1).outEnd() )
            {
              if (m_connectionSelection != NULL) delete m_connectionSelection;
              m_connectionSelection = NULL;
            }
          }
          emit newConnectionSelection(NULL);
        }

        if (event->buttons() & Qt::RightButton) {
          if (m_connectionSelection != NULL) delete m_connectionSelection;
          m_connectionSelection = NULL;
          emit newConnectionSelection(m_connectionSelection);

        }
        updateGL();
        break;

      case CMM_CREATE_ROOMS:
      {
        const RoomSelection *tmpSel = m_data->select(Coordinate(GLtoMap(m_selX1), GLtoMap(m_selY1), m_selLayer1));
        if(tmpSel->size()==0)
        {
                                //Room * r = new Room(m_data);
          Coordinate c = Coordinate(GLtoMap(m_selX1), GLtoMap(m_selY1), m_currentLayer);
          m_data->createEmptyRoom(c);
                                //m_data->execute(new SingleRoomAction(new ConnectToNeighbours, id));
        }
        m_data->unselect(tmpSel);
      }
      updateGL();
      break;


      default: break;
    }
  }


  void MapCanvas::mouseMoveEvent(QMouseEvent *event)
  {
    if (m_canvasMouseMode != CMM_MOVE)
    {
      qint8 hScroll, vScroll;
      if (event->pos().y() < 100 )
        vScroll = -1;
      else
        if (event->pos().y() > height()- 100 )
          vScroll = 1;
      else
        vScroll = 0;

      if (event->pos().x() < 100 )
        hScroll = -1;
      else
        if (event->pos().x() > width()- 100 )
          hScroll = 1;
      else
        hScroll = 0;

      continuousScroll(hScroll, vScroll);
    }

    GLdouble winX = (GLdouble)event->pos().x();
    GLdouble winY = (GLdouble)m_viewport[3] - (GLdouble)event->pos().y();
    GLdouble winZ = 0.978f;
    GLdouble tmp;
    UnProject((GLdouble) winX, (GLdouble) winY, (GLdouble) winZ,
               &m_selX2, &m_selY2, &tmp);
    m_selLayer2 = m_currentLayer;

    switch (m_canvasMouseMode)
    {
      case CMM_EDIT_INFOMARKS:
        updateGL();
        break;
      case CMM_MOVE:
        if (event->buttons() & Qt::LeftButton && m_mouseLeftPressed) {

          int idx = (int)((m_selX2 - m_moveX1backup)/SCROLLFACTOR());
          int idy = (int)((m_selY2 - m_moveY1backup)/SCROLLFACTOR());

          emit mapMove(-idx, -idy);

          if (idx != 0) m_moveX1backup = m_selX2 - idx*SCROLLFACTOR();
          if (idy != 0) m_moveY1backup = m_selY2 - idy*SCROLLFACTOR();
        }
        break;

      case CMM_SELECT_ROOMS:
        if (event->buttons() & Qt::LeftButton) {

          if (m_roomSelectionMove)
          {
            m_roomSelectionMoveX = GLtoMap(m_selX2) - GLtoMap(m_selX1);
            m_roomSelectionMoveY = GLtoMap(m_selY2) - GLtoMap(m_selY1);

            Coordinate c(m_roomSelectionMoveX, m_roomSelectionMoveY, 0);
            if (m_data->isMovable(c, m_roomSelection))
              m_roomSelectionMoveWrongPlace = false;
            else
              m_roomSelectionMoveWrongPlace = true;
          }
          else
          {
            m_selectedArea = true;
          }
        }
        updateGL();
        break;

      case CMM_CREATE_ONEWAY_CONNECTIONS:
      case CMM_CREATE_CONNECTIONS:
        if (event->buttons() & Qt::LeftButton) {

          if (m_connectionSelection != NULL)
          {
            m_connectionSelection->setSecond(m_data, m_selX2, m_selY2, m_selLayer2);

            const Room *r1 = m_connectionSelection->getFirst().room;
            ExitDirection dir1 = m_connectionSelection->getFirst().direction;
            const Room *r2 = m_connectionSelection->getSecond().room;
            ExitDirection dir2 = m_connectionSelection->getSecond().direction;

            if (r2)
            {
              if ( (r1->exit(dir1).containsOut(r2->getId())) && (r2->exit(dir2).containsOut(r1->getId())) )
              {
                m_connectionSelection->removeSecond();
              }
            }
            updateGL();
          }
        }
        break;

      case CMM_SELECT_CONNECTIONS:
        if (event->buttons() & Qt::LeftButton) {

          if (m_connectionSelection != NULL)
          {
            m_connectionSelection->setSecond(m_data, m_selX2, m_selY2, m_selLayer2);

            const Room *r1 = m_connectionSelection->getFirst().room;
            ExitDirection dir1 = m_connectionSelection->getFirst().direction;
            const Room *r2 = m_connectionSelection->getSecond().room;
            ExitDirection dir2 = m_connectionSelection->getSecond().direction;

            if (r2)
            {
              if ( !(r1->exit(dir1).containsOut(r2->getId())) || !(r2->exit(dir2).containsOut(r1->getId())) ) //not two ways
              {
                if ( dir2 != ED_UNKNOWN)
                  m_connectionSelection->removeSecond();
                else
                  if ( dir2 == ED_UNKNOWN && (!(r1->exit(dir1).containsOut(r2->getId())) || (r1->exit(dir1).containsIn(r2->getId()))) ) //not oneway
                    m_connectionSelection->removeSecond();
              }
            }
            updateGL();
          }
        }
        break;

      case CMM_CREATE_ROOMS:
        break;


        default: break;
    }
  }

  void MapCanvas::mouseReleaseEvent(QMouseEvent *event)
  {
    continuousScroll(0, 0);

    GLdouble winX = (GLdouble)event->pos().x();
    GLdouble winY = (GLdouble)m_viewport[3] - (GLdouble)event->pos().y();
    GLdouble winZ = 0.978f;
    GLdouble tmp;
    UnProject((GLdouble) winX, (GLdouble) winY, (GLdouble) winZ,
               &m_selX2, &m_selY2, &tmp);
    m_selLayer2 = m_currentLayer;

    switch (m_canvasMouseMode)
    {
      case CMM_EDIT_INFOMARKS:
        if ( m_mouseLeftPressed == true )
        {
          m_mouseLeftPressed = false;
          m_infoMarksEditDlg->setPoints(m_selX1, m_selY1, m_selX2, m_selY2, m_selLayer1);
          m_infoMarksEditDlg->show();
        }
        else
        {
          m_infoMarkSelection = false;
          m_infoMarksEditDlg->hide();
        }
        break;
      case CMM_MOVE:
        break;
      case CMM_SELECT_ROOMS:

        if ( m_ctrlPressed && m_altPressed ) break;

        if ( m_mouseLeftPressed == true ) {
          m_mouseLeftPressed = false;

          if (m_roomSelectionMove)
          {
            m_roomSelectionMove = false;
            if (m_roomSelectionMoveWrongPlace == false && m_roomSelection)
            {

              Coordinate moverel(m_roomSelectionMoveX, m_roomSelectionMoveY, 0);
              m_data->execute(new GroupAction(new MoveRelative(moverel), m_roomSelection), m_roomSelection);
            }
          }
          else
          {
            if (m_roomSelection == NULL){
                                        //add rooms to default selections
              m_roomSelection = m_data->select(Coordinate(GLtoMap(m_selX1), GLtoMap(m_selY1), m_selLayer1),Coordinate(GLtoMap(m_selX2), GLtoMap(m_selY2), m_selLayer2));
            }
            else
            {
                                        //add or remove rooms to/from default selection
              const RoomSelection *tmpSel = m_data->select(Coordinate(GLtoMap(m_selX1), GLtoMap(m_selY1), m_selLayer1),Coordinate(GLtoMap(m_selX2), GLtoMap(m_selY2), m_selLayer2));
              QList<uint> keys = tmpSel->keys();
              for (int i = 0; i < keys.size(); ++i)
              {
                if ( m_roomSelection->contains( keys.at(i) ) )
                {
                  m_data->unselect(keys.at(i), m_roomSelection);
                }
                else
                {
                  m_data->getRoom(keys.at(i), m_roomSelection);
                }
              }
              m_data->unselect(tmpSel);
            }

            if (m_roomSelection->size() < 1)
            {
              m_data->unselect(m_roomSelection);
              m_roomSelection = NULL;
            }
            else
            {
              if (m_roomSelection->size() == 1)
              {
                const Room *r = m_roomSelection->values().front();
                qint32 x = r->getPosition().x;
                qint32 y = r->getPosition().y;

                QString etmp = "Exits:";
                for (int j = 0; j < 7; j++) {

                  bool door = false;
                  if (ISSET(getFlags(r->exit(j)),EF_DOOR))
                  {
                    door = true;
                    etmp += " (";
                  }

                  if (ISSET(getFlags(r->exit(j)),EF_EXIT)) {
                    if (!door) etmp += " ";

                    switch(j)
                    {
                      case 0: etmp += "north"; break;
                      case 1: etmp += "south"; break;
                      case 2: etmp += "east"; break;
                      case 3: etmp += "west"; break;
                      case 4: etmp += "up"; break;
                      case 5: etmp += "down"; break;
                      case 6: etmp += "unknown"; break;
                    }
                  }

                  if (door)
                  {
                    if (getDoorName(r->exit(j))!="")
                      etmp += "/"+getDoorName(r->exit(j))+")";
                    else
                      etmp += ")";
                  }
                }
                etmp += ".\n";
                QString ctemp = QString("Selected Room Coordinates: %1 %2").arg(x).arg(y);
                emit log( "MapCanvas", ctemp+"\n"+getName(r)+"\n"+getDescription(r)+getDynamicDescription(r)+etmp);

/*
                if (r->isUpToDate())
                emit log( "MapCanvas", "Room is Online Updated ...");
                else
                emit log( "MapCanvas", "Room is not Online Updated ...");
*/
              }
            }
            emit newRoomSelection(m_roomSelection);
          }
          m_selectedArea = false;
        }
        updateGL();
        break;

      case CMM_CREATE_ONEWAY_CONNECTIONS:
      case CMM_CREATE_CONNECTIONS:
        if ( m_mouseLeftPressed == true ) {
          m_mouseLeftPressed = false;

          if (m_connectionSelection == NULL)
            m_connectionSelection = new ConnectionSelection(m_data, m_selX1, m_selY1, m_selLayer1);
          m_connectionSelection->setSecond(m_data, m_selX2, m_selY2, m_selLayer2);

          if (!m_connectionSelection->isValid())
          {
            if (m_connectionSelection != NULL) delete m_connectionSelection;
            m_connectionSelection = NULL;
          }else
          {
            const Room *r1 = m_connectionSelection->getFirst().room;
            ExitDirection dir1 = m_connectionSelection->getFirst().direction;
            const Room *r2 = m_connectionSelection->getSecond().room;
            ExitDirection dir2 = m_connectionSelection->getSecond().direction;

            if (r2)
            {
              const RoomSelection *tmpSel = m_data->select();
              m_data->getRoom(r1->getId(), tmpSel);
              m_data->getRoom(r2->getId(), tmpSel);

              if (m_connectionSelection != NULL) delete m_connectionSelection;
              m_connectionSelection = NULL;

              if ( !(r1->exit(dir1).containsOut(r2->getId())) || !(r2->exit(dir2).containsOut(r1->getId())) )
              {
                if (m_canvasMouseMode != CMM_CREATE_ONEWAY_CONNECTIONS)
                {
                  m_data->execute(new AddTwoWayExit(r1->getId(), r2->getId(), dir1, dir2), tmpSel);
                }
                else
                {
                  m_data->execute(new AddOneWayExit(r1->getId(), r2->getId(), dir1), tmpSel);
                }
                m_connectionSelection = new ConnectionSelection();
                m_connectionSelection->setFirst(m_data, r1->getId(), dir1);
                m_connectionSelection->setSecond(m_data, r2->getId(), dir2);
              }

              m_data->unselect(tmpSel);
            }
          }

          emit newConnectionSelection(m_connectionSelection);
        }
        updateGL();
        break;

      case CMM_SELECT_CONNECTIONS:
        if ( m_mouseLeftPressed == true ) {
          m_mouseLeftPressed = false;

          if (m_connectionSelection == NULL)
            m_connectionSelection = new ConnectionSelection(m_data, m_selX1, m_selY1, m_selLayer1);
          m_connectionSelection->setSecond(m_data, m_selX2, m_selY2, m_selLayer2);

          if (!m_connectionSelection->isValid())
          {
            if (m_connectionSelection != NULL) delete m_connectionSelection;
            m_connectionSelection = NULL;
          }else
          {
            const Room *r1 = m_connectionSelection->getFirst().room;
            ExitDirection dir1 = m_connectionSelection->getFirst().direction;
            const Room *r2 = m_connectionSelection->getSecond().room;
            ExitDirection dir2 = m_connectionSelection->getSecond().direction;

            if (r2)
            {
              if ( !(r1->exit(dir1).containsOut(r2->getId())) || !(r2->exit(dir2).containsOut(r1->getId())) )
              {
                if ( dir2 != ED_UNKNOWN)
                {
                  if (m_connectionSelection != NULL) delete m_connectionSelection;
                  m_connectionSelection = NULL;
                }
                else
                  if ( dir2 == ED_UNKNOWN && (!(r1->exit(dir1).containsOut(r2->getId())) || (r1->exit(dir1).containsIn(r2->getId()))) ) //not oneway
                {
                  if (m_connectionSelection != NULL) delete m_connectionSelection;
                  m_connectionSelection = NULL;
                }
              }
            }
          }
          emit newConnectionSelection(m_connectionSelection);
        }
        updateGL();
        break;

      case CMM_CREATE_ROOMS:
        break;


        default: break;
    }

    m_altPressed = false;
    m_ctrlPressed = false;
  }



  QSize MapCanvas::minimumSizeHint() const
  {
    return QSize(100, 100);
  }

  QSize MapCanvas::sizeHint() const
  {
    return QSize(BASESIZEX, BASESIZEY);
  }


  void MapCanvas::setScroll(int x, int y)
  {
    m_scrollX = x;
    m_scrollY = y;

    makeCurrent();
    resizeGL(width(), height());
    updateGL();
  }

  void MapCanvas::setHorizontalScroll(int x)
  {
    m_scrollX = x;

    makeCurrent();
    resizeGL(width(), height());
    updateGL();
  }

  void MapCanvas::setVerticalScroll(int y)
  {
    m_scrollY = y;

    makeCurrent();
    resizeGL(width(), height());
    updateGL();
  }

  void MapCanvas::zoomIn()
  {
    m_scaleFactor += 0.05f;
    if (m_scaleFactor > 2.0f)
      m_scaleFactor -= 0.05f;

    makeCurrent();
    resizeGL(width(), height());
    updateGL();
  }

  void MapCanvas::zoomOut()
  {
    m_scaleFactor -= 0.05f;
    if (m_scaleFactor < 0.04f)
      m_scaleFactor += 0.05f;

    makeCurrent();
    resizeGL(width(), height());
    updateGL();
  }

  void MapCanvas::initializeGL()
  {
    qglClearColor(QColor(110,110,110));
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPolygonStipple(halftone);
        //glPolygonStipple(quadtone);

    makeGlLists();
  }

  void MapCanvas::resizeGL(int width, int height)
  {
    float swp = m_scaleFactor * (1.0f - ((float)(width - BASESIZEX) / width));
    float shp = m_scaleFactor * (1.0f - ((float)(height - BASESIZEY) / height));

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    //glOrtho(-0.5, +0.5, +0.5, -0.5, 0.0, 160.0);
    //glScaled(swp*0.081, shp*0.081, 1.0f);
    glFrustum(-0.5, +0.5, +0.5, -0.5, 5.0, 80.0);
    glScaled(swp, shp, 1.0f);

    glTranslated( -SCROLLFACTOR() * (float)(m_scrollX), -SCROLLFACTOR() * (float)(m_scrollY), -60.0f );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

        //store matrices
    glGetDoublev( GL_MODELVIEW_MATRIX, m_modelview );
    glGetDoublev( GL_PROJECTION_MATRIX, m_projection );
    glGetIntegerv( GL_VIEWPORT, m_viewport );

    GLdouble winX, winY, winZ, tmp;
    winX = (GLdouble)0;
    winY = (GLdouble)m_viewport[3];
    winZ = 0.978f;
    UnProject((GLdouble) winX, (GLdouble) winY, (GLdouble) winZ,
               &m_visibleX1, &m_visibleY1, &tmp);

    winX = (GLdouble)width;
    winY = (GLdouble)m_viewport[3]-(GLdouble)height;
    winZ = 0.978f;
    UnProject((GLdouble) winX, (GLdouble) winY, (GLdouble) winZ,
               &m_visibleX2, &m_visibleY2, &tmp);

  }

  void MapCanvas::dataLoaded()
  {
    emit onCenter( m_data->getPosition().x, m_data->getPosition().y );
    updateGL();
  }

  void MapCanvas::moveMarker(const Coordinate & c) {
    m_data->setPosition(c);
    m_currentLayer = c.z;
    updateGL();
    emit onCenter(c.x, c.y);
  //emit onEnsureVisible(c.x, c.y);
  }

  void MapCanvas::drawGroupCharacters()
  {
    QVector<CGroupChar *> chars;

    chars = m_groupManager->getChars();
    if (chars.isEmpty())
      return;

    for (int charIndex = 0; charIndex < chars.size(); charIndex++) {
      unsigned int id = chars[charIndex]->getPosition();

      if (!m_data->isEmpty() && chars[charIndex]->getName() != Config().m_groupManagerCharName)
      {
        const RoomSelection* selection = m_data->select();
        const Room* r = m_data->getRoom(id, selection);
        if (r)
        {
          qint32 x = r->getPosition().x;
          qint32 y = r->getPosition().y;
          qint32 z = r->getPosition().z;
          qint32 layer = z - m_currentLayer;

          glPushMatrix();
          glTranslated(x-0.5, y-0.5, ROOM_Z_DISTANCE*layer+0.1);
          glColor4d(0.0f, 0.0f, 0.0f, 0.4f);

          glEnable(GL_BLEND);
          glDisable(GL_DEPTH_TEST);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          glCallList(m_room_selection_inner_gllist);
          glDisable(GL_BLEND);

          qglColor(chars[charIndex]->getColor());
          glCallList(m_room_selection_gllist);
          glEnable(GL_DEPTH_TEST);
          glPopMatrix();
        }
        m_data->unselect(id, selection);
      }
    }
  }


  void MapCanvas::paintGL()
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_data->isEmpty())
    {
      m_data->draw(
                   Coordinate((int)(m_visibleX1), (int)(m_visibleY1), m_currentLayer-10 ),
                               Coordinate((int)(m_visibleX2+1), (int)(m_visibleY2+1), m_currentLayer+10),
                                           *this);

      if (m_scaleFactor >= 0.25)
      {
        renderText ( 0.0f, 0.0f, 0.0f, "", *m_glFont );

        MarkerListIterator m(m_data->getMarkersList());
        while (m.hasNext())
          drawInfoMark(m.next());
      }
    }

    GLdouble len = 0.2f;
    if (m_selectedArea || m_infoMarkSelection)
    {
      glEnable(GL_BLEND);
      glDisable(GL_DEPTH_TEST);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glColor4d(0.0f, 0.0f, 0.0f, 0.5f);
      glBegin(GL_POLYGON);
      glVertex3d(m_selX1, m_selY1, 0.005);
      glVertex3d(m_selX2, m_selY1, 0.005);
      glVertex3d(m_selX2, m_selY2, 0.005);
      glVertex3d(m_selX1, m_selY2, 0.005);
      glEnd();
      qglColor(Qt::white);

      glLineStipple(4, 43690);
      glEnable(GL_LINE_STIPPLE);
      glBegin(GL_LINE_LOOP);
      glVertex3d(m_selX1, m_selY1, 0.005);
      glVertex3d(m_selX2, m_selY1, 0.005);
      glVertex3d(m_selX2, m_selY2, 0.005);
      glVertex3d(m_selX1, m_selY2, 0.005);
      glEnd();
      glDisable(GL_LINE_STIPPLE);
      glDisable(GL_BLEND);
      glEnable(GL_DEPTH_TEST);
    }


    if (m_infoMarkSelection)
    {
      glColor4d(1.0f, 1.0f, 0.0f, 1.0f);
      glPointSize (3.0);
      glLineWidth (3.0);

      glBegin(GL_LINES);
      glVertex3d(m_selX1, m_selY1, 0.005);
      glVertex3d(m_selX2, m_selY2, 0.005);
      glEnd();
    }

    //paint selected connection
    if (m_connectionSelection && m_connectionSelection->isFirstValid())
    {
      const Room* r = m_connectionSelection->getFirst().room;

      GLdouble x1p = r->getPosition().x;
      GLdouble y1p = r->getPosition().y;
      GLdouble x2p = m_selX2;
      GLdouble y2p = m_selY2;

      switch (m_connectionSelection->getFirst().direction)
      {
        case ED_NORTH:
          y1p -= 0.4;
          break;
        case ED_SOUTH:
          y1p += 0.4;
          break;
        case ED_EAST:
          x1p += 0.4;
          break;
        case ED_WEST:
          x1p -= 0.4;
          break;
        case ED_UP:
          x1p += 0.3;
          y1p -= 0.3;
          break;
        case ED_DOWN:
          x1p -= 0.3;
          y1p += 0.3;
          break;
        case ED_UNKNOWN:
          break;
          default:;
      }


      if (m_connectionSelection->isSecondValid())
      {
        r = m_connectionSelection->getSecond().room;
        x2p = r->getPosition().x;
        y2p = r->getPosition().y;

        switch (m_connectionSelection->getSecond().direction)
        {
          case ED_NORTH:
            y2p -= 0.4;
            break;
          case ED_SOUTH:
            y2p += 0.4;
            break;
          case ED_EAST:
            x2p += 0.4;
            break;
          case ED_WEST:
            x2p -= 0.4;
            break;
          case ED_UP:
            x2p += 0.3;
            y2p -= 0.3;
            break;
          case ED_DOWN:
            x2p -= 0.3;
            y2p += 0.3;
            break;
          case ED_UNKNOWN:
            break;
            default:;
        }
      }

      qglColor(Qt::red);
      glPointSize (10.0);
      glBegin(GL_POINTS);
      glVertex3d(x1p, y1p, 0.005);
      glVertex3d(x2p, y2p, 0.005);
      glEnd();
      glPointSize (1.0);

      glBegin(GL_LINES);
      glVertex3d(x1p, y1p, 0.005);
      glVertex3d(x2p, y2p, 0.005);
      glEnd();

                /*
      glPushMatrix();
      glTranslated(x2p, y2p, 0.005);
      glRotated(0.0,0.0,1.0,180);
      glBegin(GL_TRIANGLES);
      glVertex3d(0.0, 0.0, 0.0);
      glVertex3d(-0.4, -0.1, 0.0);
      glVertex3d(-0.4, 0.1, 0.0);
      glEnd();
      glPopMatrix();
                */
    }

    // paint selection
    if (m_roomSelection)
    {
      const Room *room;

      QList<const Room*> values = m_roomSelection->values();
      for (int i = 0; i < values.size(); ++i)
      {
        room = values.at(i);

        qint32 x = room->getPosition().x;
        qint32 y = room->getPosition().y;
        qint32 z = room->getPosition().z;
        qint32 layer = z - m_currentLayer;

        glPushMatrix();
                    //glTranslated(x-0.5, y-0.5, ROOM_Z_DISTANCE*z);
        glTranslated(x-0.5, y-0.5, ROOM_Z_DISTANCE*layer);

        glColor4d(0.0f, 0.0f, 0.0f, 0.4f);

        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glCallList(m_room_gllist);

        qglColor(Qt::red);
        glBegin(GL_LINE_STRIP);
        glVertex3d(0+len, 0, 0.005);
        glVertex3d(0, 0, 0.005);
        glVertex3d(0, 0+len, 0.005);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3d(0+len, 1, 0.005);
        glVertex3d(0, 1, 0.005);
        glVertex3d(0, 1-len, 0.005);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3d(1-len, 1, 0.005);
        glVertex3d(1, 1, 0.005);
        glVertex3d(1, 1-len, 0.005);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3d(1-len, 0, 0.005);
        glVertex3d(1, 0, 0.005);
        glVertex3d(1, 0+len, 0.005);
        glEnd();

        if (m_roomSelectionMove)
        {
          if (m_roomSelectionMoveWrongPlace)
            glColor4d(1.0f, 0.0f, 0.0f, 0.4f);
          else
            glColor4d(1.0f, 1.0f, 1.0f, 0.4f);

           //glTranslated(m_roomSelectionMoveX, m_roomSelectionMoveY, ROOM_Z_DISTANCE*z);;
          glTranslated(m_roomSelectionMoveX, m_roomSelectionMoveY, ROOM_Z_DISTANCE*layer);
          glCallList(m_room_gllist);
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        glPopMatrix();
      }

    }

    //draw the characters before the current position
    drawGroupCharacters();

    //paint char current position
    if (!m_data->isEmpty())
    {
      qint32 x = m_data->getPosition().x;
      qint32 y = m_data->getPosition().y;
      qint32 z = m_data->getPosition().z;
      qint32 layer = z - m_currentLayer;

      glPushMatrix();
      glTranslated(x-0.5, y-0.5, ROOM_Z_DISTANCE*layer+0.01);

      glColor4d(0.0f, 0.0f, 0.0f, 0.4f);

      glEnable(GL_BLEND);
      glDisable(GL_DEPTH_TEST);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glCallList(m_room_selection_inner_gllist);
      glDisable(GL_BLEND);

      // Use the player's selected color
      //qglColor(Qt::yellow);
      qglColor(Config().m_groupManagerColor);
      glCallList(m_room_selection_gllist);
      glEnable(GL_DEPTH_TEST);
      glPopMatrix();
    }

    if (!m_prespammedPath->isEmpty())
    {
      QList<Coordinate> path = m_data->getPath(m_prespammedPath->getQueue());
      Coordinate c1, c2;
      double dx,dy,dz;
      bool anypath = false;

      c1 = m_data->getPosition();
      QList<Coordinate>::const_iterator it = path.begin();
      while (it != path.end())
      {
        if (!anypath)
        {
          drawPathStart(c1);
          anypath = true;
        }
        c2 = *it;
        if (!drawPath(c1, c2, dx, dy, dz))
          break;
        ++it;
      }
      if (anypath) drawPathEnd(dx, dy, dz);
    }
  }


  void MapCanvas::drawPathStart(const Coordinate& sc)
  {
    qint32 x1 = sc.x;
    qint32 y1 = sc.y;
    qint32 z1 = sc.z;
    qint32 layer1 = z1 - m_currentLayer;

    glPushMatrix();
    glTranslated(x1, y1, 0);

    // Instead of yellow use the player's color
    //glColor4d(1.0f, 1.0f, 0.0f, 1.0f);
    qglColor(Config().m_groupManagerColor);
    glEnable(GL_BLEND);
    glPointSize (4.0);
    glLineWidth (4.0);

    double srcZ = ROOM_Z_DISTANCE*layer1+0.3;

    glBegin(GL_LINE_STRIP);
    glVertex3d(0.0, 0.0, srcZ);

  }

  bool MapCanvas::drawPath(const Coordinate& sc, const Coordinate& dc, double &dx, double &dy, double &dz)
  {
    qint32 x1 = sc.x;
    qint32 y1 = sc.y;
    //qint32 z1 = sc.z;

    qint32 x2 = dc.x;
    qint32 y2 = dc.y;
    qint32 z2 = dc.z;
    qint32 layer2 = z2 - m_currentLayer;

    dx = x2 - x1;
    dy = y2 - y1;

/*
    if ((z2 !=      m_currentLayer) &&
    (z1 !=  m_currentLayer))
    return false;
*/
    dz = ROOM_Z_DISTANCE*layer2+0.3;

    glVertex3d(dx, dy, dz);

    return true;
  }

  void MapCanvas::drawPathEnd(double dx, double dy, double dz)
  {
    glEnd();

    glPointSize (8.0);
    glBegin(GL_POINTS);
    glVertex3d(dx, dy, dz);
    glEnd();

    glLineWidth (2.0);
    glPointSize (2.0);
    glDisable(GL_BLEND);
    glPopMatrix();
  }

  void MapCanvas::drawInfoMark(InfoMark* marker)
  {
    qreal x1 = marker->getPosition1().x / 100.0f;
    qreal y1 = marker->getPosition1().y / 100.0f;
    int layer = marker->getPosition1().z;
    qreal x2 = marker->getPosition2().x / 100.0f;
    qreal y2 = marker->getPosition2().y / 100.0f;
    qreal dx = x2 - x1;
    qreal dy = y2 - y1;

    qreal width = 0;
    qreal height = 0;

    if (layer != m_currentLayer) return;

    if (marker->getType() == MT_TEXT)
    {
      width = m_glFontMetrics->width(marker->getText()) * 0.022f / m_scaleFactor;
      height = m_glFontMetrics->height() * 0.007f / m_scaleFactor;
      x2 = x1 + width;
      y2 = y1 + height;
    }

    //check if marker is visible
    if ( ( (x1+1 < m_visibleX1) || (x1-1 > m_visibleX2+1) ) &&
            ((x2+1 < m_visibleX1) || (x2-1 > m_visibleX2+1) ) )
      return;
    if ( ( (y1+1 < m_visibleY1) || (y1-1 > m_visibleY2+1) ) &&
            ((y2+1 < m_visibleY1) || (y2-1 > m_visibleY2+1) ) )
      return;

    glPushMatrix();
    glTranslated(x1/*-0.5*/, y1/*-0.5*/, 0.0);

    switch (marker->getType())
    {
      case MT_TEXT:
        glColor4d(0, 0, 0, 0.3);
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_POLYGON);
        glVertex3d(0.0, 0.0, 1.0);
        glVertex3d(0.0, 0.25+height, 1.0);
        glVertex3d(0.2+width, 0.25+height, 1.0);
        glVertex3d(0.2+width, 0.0, 1.0);
        glEnd();
        glDisable(GL_BLEND);
         //glEnable(GL_DEPTH_TEST);
        glColor4d(1.0, 1.0, 1.0, 1.0);

        renderText ( 0.1, 0.25, 1.2f, marker->getText(), *m_glFont );
        glEnable(GL_DEPTH_TEST);
        break;
      case MT_LINE:
        glColor4d(1.0f, 1.0f, 1.0f, 0.70);
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glPointSize (2.0);
        glLineWidth (2.0);
        glBegin(GL_LINES);
        glVertex3d(0.0, 0.0, 0.1);
        glVertex3d(dx, dy, 0.1);
        glEnd();
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        break;
      case MT_ARROW:
        glColor4d(1.0f, 1.0f, 1.0f, 0.70);
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glPointSize (2.0);
        glLineWidth (2.0);
        glBegin(GL_LINE_STRIP);
        glVertex3d(0.0, 0.05, 1.0);
        glVertex3d(dx-0.2, dy+0.1, 1.0);
        glVertex3d(dx-0.1, dy+0.1, 1.0);
        glEnd();
        glBegin(GL_TRIANGLES);
        glVertex3d(dx-0.1, dy+0.1-0.07, 1.0);
        glVertex3d(dx-0.1, dy+0.1+0.07, 1.0);
        glVertex3d(dx+0.1, dy+0.1, 1.0);
        glEnd();
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        break;
    }

    glPopMatrix();
  }

  void MapCanvas::alphaOverlayTexture(const QString &texture)
  {
    bindTexture(QPixmap(texture));
    glCallList(m_room_gllist);
  }

  void MapCanvas::alphaOverlayTexture(QPixmap *pix)
  {
    bindTexture(*pix);
    glCallList(m_room_gllist);
  }


  void MapCanvas::drawRoomDoorName(const Room *sourceRoom, uint sourceDir, const Room *targetRoom, uint targetDir)
  {
    assert(sourceRoom != NULL);
    assert(targetRoom != NULL);

    qint32 srcX = sourceRoom->getPosition().x;
    qint32 srcY = sourceRoom->getPosition().y;
    qint32 tarX = targetRoom->getPosition().x;
    qint32 tarY = targetRoom->getPosition().y;
    qint32 dX = srcX - tarX;
    qint32 dY = srcY - tarY;

    QString name;
    bool together = false;

    if (ISSET(getFlags(targetRoom->exit(targetDir)), EF_DOOR) && // the other room has a door?
        !getDoorName(targetRoom->exit(targetDir)).isEmpty() && // has a door on both sides...
        abs(dX) <= 1 && abs(dY) <= 1) // the door is close by!
    {
      // skip the other direction since we're printing these together
      if (sourceDir % 2 == 1)
        return;

      together = true;

      // no need for duplicating names (its spammy)
      const QString &sourceName = getDoorName(sourceRoom->exit(sourceDir));
      const QString &targetName = getDoorName(targetRoom->exit(targetDir));
      if (sourceName != targetName)
        name =  sourceName + "/" + targetName;
      else
        name = sourceName;
    }
    else
      name = getDoorName(sourceRoom->exit(sourceDir));

    glPushMatrix();

    // box
    qreal width = m_glFontMetrics->width(name) * 0.022f / m_scaleFactor;
    qreal height = m_glFontMetrics->height() * 0.007f / m_scaleFactor;

    if (together)
      glTranslated(srcX-(width/2.0)-(dX*0.5), srcY-0.5-(dY*0.5), 0.0);
    else
    {
      switch (sourceDir)
      {
        case ED_NORTH:
          glTranslated(srcX-(width/2.0), srcY-0.65, 0.0);
          break;
        case ED_SOUTH:
          glTranslated(srcX-(width/2.0), srcY-0.15, 0.0);
          break;
        case ED_WEST:
          glTranslated(srcX-(width/2.0), srcY-0.5, 0.0);
          break;
        case ED_EAST:
          glTranslated(srcX-(width/2.0), srcY-0.35, 0.0);
          break;
        case ED_UP:
          glTranslated(srcX-(width/2.0), srcY-0.85, 0.0);
          break;
        case ED_DOWN:
          glTranslated(srcX-(width/2.0), srcY-0.0, 0.0);
      };
    }
    glColor4d(0, 0, 0, 0.3);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_POLYGON);
    glVertex3d(0.0, 0.0, 1.0);
    glVertex3d(0.0, 0.25+height, 1.0);
    glVertex3d(0.2+width, 0.25+height, 1.0);
    glVertex3d(0.2+width, 0.0, 1.0);
    glEnd();
    glDisable(GL_BLEND);

    // text
    glColor4d(1.0, 1.0, 1.0, 1.0);
    renderText (0.1, 0.25, 1.2f, name, *m_glFont );
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
  }

  void MapCanvas::drawRoom(const Room *room, const std::vector<Room *> & rooms, const std::vector<std::set<RoomRecipient *> > & locks)
  {
    if (!room) return;

    qint32 x = room->getPosition().x;
    qint32 y = room->getPosition().y;
    qint32 z = room->getPosition().z;

    qint32 layer = z - m_currentLayer;

    glPushMatrix();
    glTranslated(x-0.5, y-0.5, ROOM_Z_DISTANCE*layer);

    glLineStipple(2, 43690);

    //terrain texture
    qglColor(Qt::white);

    if (layer > 0)
    {
      if (Config().m_drawUpperLayersTextured)
        glEnable (GL_POLYGON_STIPPLE);
      else
      {
        glDisable (GL_POLYGON_STIPPLE);
        glColor4d(0.3, 0.3, 0.3, 0.6-0.2*layer);
        glEnable(GL_BLEND);
      }
    }
    else if (layer == 0)
    {
      glColor4d(1.0, 1.0, 1.0, 0.7);
      glEnable(GL_BLEND);
    }

    ExitFlags ef_north = getFlags(room->exit(ED_NORTH));
    ExitFlags ef_south = getFlags(room->exit(ED_SOUTH));
    ExitFlags ef_east  = getFlags(room->exit(ED_EAST));
    ExitFlags ef_west  = getFlags(room->exit(ED_WEST));
    ExitFlags ef_up    = getFlags(room->exit(ED_UP));
    ExitFlags ef_down  = getFlags(room->exit(ED_DOWN));

    quint32 roadindex = 0;
    if ( ISSET(ef_north, EF_ROAD)) SET(roadindex, bit1);
    if ( ISSET(ef_south, EF_ROAD)) SET(roadindex, bit2);
    if ( ISSET(ef_east,  EF_ROAD)) SET(roadindex, bit3);
    if ( ISSET(ef_west,  EF_ROAD)) SET(roadindex, bit4);

    if (layer <= 0 || Config().m_drawUpperLayersTextured)
    {
      if ( (getTerrainType(room)) == RTT_ROAD)
        bindTexture(*m_roadPixmaps[roadindex]);
      else
        bindTexture(*m_terrainPixmaps[getTerrainType(room)]);

      glEnable(GL_TEXTURE_2D);
      glCallList(m_room_gllist);

      glEnable(GL_BLEND);

      RoomMobFlags mf = getMobFlags(room);
      RoomLoadFlags lf = getLoadFlags(room);

      // Make dark rooms look dark
      if (getLightType(room)==RLT_DARK)
      {
        GLdouble oldcolour[4];
        glGetDoublev(GL_CURRENT_COLOR, oldcolour);

        glTranslated(0, 0, 0.005);

        glColor4d(0.1f, 0.0f, 0.0f, 0.2f);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glCallList(m_room_gllist);

        glColor4d(oldcolour[0], oldcolour[1], oldcolour[2], oldcolour[3]);
      }

      // Draw a little red cross on noride rooms
      if (getRidableType(room)==RRT_NOTRIDABLE)
      {
        GLdouble oldcolour[4];
        glGetDoublev(GL_CURRENT_COLOR, oldcolour);

        qglColor(Qt::red);
        glBegin(GL_LINES);
        glVertex3d(0.6, 0.2, 0.005);
        glVertex3d(0.8, 0.4, 0.005);
        glVertex3d(0.8, 0.2, 0.005);
        glVertex3d(0.6, 0.4, 0.005);
        glEnd();

        glColor4d(oldcolour[0], oldcolour[1], oldcolour[2], oldcolour[3]);
      }

      // Only display at a certain scale
      if (m_scaleFactor >= 0.15) {
      // Trail Support
      glTranslated(0, 0, 0.005);
      if (roadindex > 0 && (getTerrainType(room)) != RTT_ROAD) {
        alphaOverlayTexture(m_trailPixmaps[roadindex]);
        glTranslated(0, 0, 0.005);
      }
      //RMF_RENT, RMF_SHOP, RMF_WEAPONSHOP, RMF_ARMOURSHOP, RMF_FOODSHOP, RMF_PETSHOP,
      //RMF_GUILD, RMF_SCOUTGUILD, RMF_MAGEGUILD, RMF_CLERICGUILD, RMF_WARRIORGUILD,
      //RMF_RANGERGUILD, RMF_SMOB, RMF_QUEST, RMF_ANY
      if (ISSET(mf,RMF_RENT))
        alphaOverlayTexture(m_mobPixmaps[0]);
      if (ISSET(mf,RMF_SHOP))
        alphaOverlayTexture(m_mobPixmaps[1]);
      if (ISSET(mf,RMF_WEAPONSHOP))
        alphaOverlayTexture(m_mobPixmaps[2]);
      if (ISSET(mf,RMF_ARMOURSHOP))
        alphaOverlayTexture(m_mobPixmaps[3]);
      if (ISSET(mf,RMF_FOODSHOP))
        alphaOverlayTexture(m_mobPixmaps[4]);
      if (ISSET(mf,RMF_PETSHOP))
        alphaOverlayTexture(m_mobPixmaps[5]);
      if (ISSET(mf,RMF_GUILD))
        alphaOverlayTexture(m_mobPixmaps[6]);
      if (ISSET(mf,RMF_SCOUTGUILD))
        alphaOverlayTexture(m_mobPixmaps[7]);
      if (ISSET(mf,RMF_MAGEGUILD))
        alphaOverlayTexture(m_mobPixmaps[8]);
      if (ISSET(mf,RMF_CLERICGUILD))
        alphaOverlayTexture(m_mobPixmaps[9]);
      if (ISSET(mf,RMF_WARRIORGUILD))
        alphaOverlayTexture(m_mobPixmaps[10]);
      if (ISSET(mf,RMF_RANGERGUILD))
        alphaOverlayTexture(m_mobPixmaps[11]);
      if (ISSET(mf,RMF_SMOB))
        alphaOverlayTexture(m_mobPixmaps[12]);
      if (ISSET(mf,RMF_QUEST))
        alphaOverlayTexture(m_mobPixmaps[13]);
      if (ISSET(mf,RMF_ANY))
        alphaOverlayTexture(m_mobPixmaps[14]);

            //RLF_TREASURE, RLF_ARMOUR, RLF_WEAPON, RLF_WATER, RLF_FOOD, RLF_HERB
            //RLF_KEY, RLF_MULE, RLF_HORSE, RLF_PACKHORSE, RLF_TRAINEDHORSE
            //RLF_ROHIRRIM, RLF_WARG, RLF_BOAT
      glTranslated(0, 0, 0.005);
      if (ISSET(lf,RLF_TREASURE))
        alphaOverlayTexture(m_loadPixmaps[0]);
      if (ISSET(lf,RLF_ARMOUR))
        alphaOverlayTexture(m_loadPixmaps[1]);
      if (ISSET(lf,RLF_WEAPON))
        alphaOverlayTexture(m_loadPixmaps[2]);
      if (ISSET(lf,RLF_WATER))
        alphaOverlayTexture(m_loadPixmaps[3]);
      if (ISSET(lf,RLF_FOOD))
        alphaOverlayTexture(m_loadPixmaps[4]);
      if (ISSET(lf,RLF_HERB))
        alphaOverlayTexture(m_loadPixmaps[5]);
      if (ISSET(lf,RLF_KEY))
        alphaOverlayTexture(m_loadPixmaps[6]);
      if (ISSET(lf,RLF_MULE))
        alphaOverlayTexture(m_loadPixmaps[7]);
      if (ISSET(lf,RLF_HORSE))
        alphaOverlayTexture(m_loadPixmaps[8]);
      if (ISSET(lf,RLF_PACKHORSE))
        alphaOverlayTexture(m_loadPixmaps[9]);
      if (ISSET(lf,RLF_TRAINEDHORSE))
        alphaOverlayTexture(m_loadPixmaps[10]);
      if (ISSET(lf,RLF_ROHIRRIM))
        alphaOverlayTexture(m_loadPixmaps[11]);
      if (ISSET(lf,RLF_WARG))
        alphaOverlayTexture(m_loadPixmaps[12]);
      if (ISSET(lf,RLF_BOAT))
        alphaOverlayTexture(m_loadPixmaps[13]);

      glTranslated(0, 0, 0.005);
      if (ISSET(lf,RLF_ATTENTION))
        alphaOverlayTexture(m_loadPixmaps[14]);
      if (ISSET(lf,RLF_TOWER))
        alphaOverlayTexture(m_loadPixmaps[15]);

            //UPDATED?
      glTranslated(0, 0, 0.005);
      if (Config().m_showUpdated && !room->isUpToDate())
        alphaOverlayTexture(m_updatePixmap[0]);
      glDisable(GL_BLEND);
      glDisable(GL_TEXTURE_2D);
  }
    }
    else
    {
      glEnable(GL_BLEND);
      glCallList(m_room_gllist);
      glDisable(GL_BLEND);
    }


        //walls
    glTranslated(0, 0, 0.005);

    if (layer > 0)
    {
      glDisable (GL_POLYGON_STIPPLE);
      glColor4d(0.3, 0.3, 0.3, 0.6);
      glEnable(GL_BLEND);
    }
    else
    {
      qglColor(Qt::black);
    }

    glPointSize (3.0);
    glLineWidth (2.4);

    //exit n
    if (ISSET(ef_north, EF_EXIT) &&
        Config().m_drawNotMappedExits &&
        room->exit(ED_NORTH).outBegin() == room->exit(ED_NORTH).outEnd()) // zero outgoing connections
    {
      glEnable(GL_LINE_STIPPLE);
      glColor4d(1.0, 0.5, 0.0, 0.0);
      glCallList(m_wall_north_gllist);
      glColor4d(0.0, 0.0, 0.0, 0.0);
      glDisable(GL_LINE_STIPPLE);
    }
    else
    {
      if ( ISSET(getFlags(room->exit(ED_NORTH)), EF_CLIMB))
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(0.7, 0.7, 0.7, 0.0);
        glCallList(m_wall_north_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
      if ( ISSET(getFlags(room->exit(ED_NORTH)), EF_RANDOM))
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(1.0, 0.0, 0.0, 0.0);
        glCallList(m_wall_north_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
      if ( ISSET(getFlags(room->exit(ED_NORTH)), EF_SPECIAL))
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(0.8, 0.1, 0.8, 0.0);
        glCallList(m_wall_north_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
    }
    //wall n
    if (ISNOTSET(ef_north, EF_EXIT) || ISSET(ef_north, EF_DOOR))
    {
      if (ISNOTSET(ef_north, EF_DOOR) && room->exit(ED_NORTH).outBegin() != room->exit(ED_NORTH).outEnd())
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(0.2, 0.0, 0.0, 0.0);
        glCallList(m_wall_north_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
      else
        glCallList(m_wall_north_gllist);
    }
    //door n
    if (ISSET(ef_north, EF_DOOR))
      glCallList(m_door_north_gllist);
    //exit s
    if (ISSET(ef_south, EF_EXIT) &&
        Config().m_drawNotMappedExits &&
        room->exit(ED_SOUTH).outBegin() == room->exit(ED_SOUTH).outEnd()) // zero outgoing connections
    {
      glEnable(GL_LINE_STIPPLE);
      glColor4d(1.0, 0.5, 0.0, 0.0);
      glCallList(m_wall_south_gllist);
      glColor4d(0.0, 0.0, 0.0, 0.0);
      glDisable(GL_LINE_STIPPLE);
    }
    else
    {
      if ( ISSET(getFlags(room->exit(ED_SOUTH)), EF_CLIMB))
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(0.7, 0.7, 0.7, 0.0);
        glCallList(m_wall_south_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
      if ( ISSET(getFlags(room->exit(ED_SOUTH)), EF_RANDOM))
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(1.0, 0.0, 0.0, 0.0);
        glCallList(m_wall_south_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
      if ( ISSET(getFlags(room->exit(ED_SOUTH)), EF_SPECIAL))
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(0.8, 0.1, 0.8, 0.0);
        glCallList(m_wall_south_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
    }
    //wall s
    if (ISNOTSET(ef_south, EF_EXIT) || ISSET(ef_south, EF_DOOR))
    {
      if (ISNOTSET(ef_south, EF_DOOR) && room->exit(ED_SOUTH).outBegin() != room->exit(ED_SOUTH).outEnd())
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(0.2, 0.0, 0.0, 0.0);
        glCallList(m_wall_south_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
      else
        glCallList(m_wall_south_gllist);
    }
    //door s
    if (ISSET(ef_south, EF_DOOR))
      glCallList(m_door_south_gllist);

    //exit e
    if (ISSET(ef_east, EF_EXIT) &&
        Config().m_drawNotMappedExits &&
        room->exit(ED_EAST).outBegin() == room->exit(ED_EAST).outEnd()) // zero outgoing connections
    {
      glEnable(GL_LINE_STIPPLE);
      glColor4d(1.0, 0.5, 0.0, 0.0);
      glCallList(m_wall_east_gllist);
      glColor4d(0.0, 0.0, 0.0, 0.0);
      glDisable(GL_LINE_STIPPLE);
    }
    else
    {
      if ( ISSET(getFlags(room->exit(ED_EAST)), EF_CLIMB))
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(0.7, 0.7, 0.7, 0.0);
        glCallList(m_wall_east_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
      if ( ISSET(getFlags(room->exit(ED_EAST)), EF_RANDOM))
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(1.0, 0.0, 0.0, 0.0);
        glCallList(m_wall_east_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
      if ( ISSET(getFlags(room->exit(ED_EAST)), EF_SPECIAL))
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(0.8, 0.1, 0.8, 0.0);
        glCallList(m_wall_east_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
    }
    //wall e
    if (ISNOTSET(ef_east, EF_EXIT) || ISSET(ef_east, EF_DOOR))
    {
      if (ISNOTSET(ef_east, EF_DOOR) && room->exit(ED_EAST).outBegin() != room->exit(ED_EAST).outEnd())
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(0.2, 0.0, 0.0, 0.0);
        glCallList(m_wall_east_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
      else
        glCallList(m_wall_east_gllist);
    }
    //door e
    if (ISSET(ef_east, EF_DOOR))
      glCallList(m_door_east_gllist);

    //exit w
    if (ISSET(ef_west, EF_EXIT) &&
        Config().m_drawNotMappedExits &&
        room->exit(ED_WEST).outBegin() == room->exit(ED_WEST).outEnd()) // zero outgoing connections
    {
      glEnable(GL_LINE_STIPPLE);
      glColor4d(1.0, 0.5, 0.0, 0.0);
      glCallList(m_wall_west_gllist);
      glColor4d(0.0, 0.0, 0.0, 0.0);
      glDisable(GL_LINE_STIPPLE);
    }
    else
    {
      if ( ISSET(getFlags(room->exit(ED_WEST)), EF_CLIMB))
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(0.7, 0.7, 0.7, 0.0);
        glCallList(m_wall_west_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
      if ( ISSET(getFlags(room->exit(ED_WEST)), EF_RANDOM))
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(1.0, 0.0, 0.0, 0.0);
        glCallList(m_wall_west_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
      if ( ISSET(getFlags(room->exit(ED_WEST)), EF_SPECIAL))
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(0.8, 0.1, 0.8, 0.0);
        glCallList(m_wall_west_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
    }
    //wall w
    if (ISNOTSET(ef_west, EF_EXIT) || ISSET(ef_west, EF_DOOR))
    {
      if (ISNOTSET(ef_west, EF_DOOR) && room->exit(ED_WEST).outBegin() != room->exit(ED_WEST).outEnd())
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(0.2, 0.0, 0.0, 0.0);
        glCallList(m_wall_west_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
      else
        glCallList(m_wall_west_gllist);
    }

    //door w
    if (ISSET(ef_west, EF_DOOR))
      glCallList(m_door_west_gllist);

    glPointSize (3.0);
    glLineWidth (2.0);

    //exit u
    if (ISSET(ef_up, EF_EXIT))
    {
      if (Config().m_drawNotMappedExits && room->exit(ED_UP).outBegin() == room->exit(ED_UP).outEnd()) // zero outgoing connections
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(1.0, 0.5, 0.0, 0.0);

        glCallList(m_exit_up_transparent_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
      else
      {
        if ( ISSET(getFlags(room->exit(ED_UP)), EF_CLIMB))
        {
          glEnable(GL_LINE_STIPPLE);
          glColor4d(0.5, 0.5, 0.5, 0.0);
          glCallList(m_exit_up_transparent_gllist);
          glColor4d(0.0, 0.0, 0.0, 0.0);
          glDisable(GL_LINE_STIPPLE);
        }

        if (layer > 0)
          glCallList(m_exit_up_transparent_gllist);
        else
          glCallList(m_exit_up_gllist);

        if (ISSET(ef_up, EF_DOOR))
          glCallList(m_door_up_gllist);
      }
    }

    //exit d
    if (ISSET(ef_down, EF_EXIT))
    {
      if (Config().m_drawNotMappedExits && room->exit(ED_DOWN).outBegin() == room->exit(ED_DOWN).outEnd()) // zero outgoing connections
      {
        glEnable(GL_LINE_STIPPLE);
        glColor4d(1.0, 0.5, 0.0, 0.0);

        glCallList(m_exit_down_transparent_gllist);
        glColor4d(0.0, 0.0, 0.0, 0.0);
        glDisable(GL_LINE_STIPPLE);
      }
      else
      {
        if ( ISSET(getFlags(room->exit(ED_DOWN)), EF_CLIMB))
        {
          glEnable(GL_LINE_STIPPLE);
          glColor4d(0.5, 0.5, 0.5, 0.0);
          glCallList(m_exit_down_transparent_gllist);
          glColor4d(0.0, 0.0, 0.0, 0.0);
          glDisable(GL_LINE_STIPPLE);
        }

        if (layer > 0)
          glCallList(m_exit_down_transparent_gllist);
        else
          glCallList(m_exit_down_gllist);

        if (ISSET(ef_down, EF_DOOR))
          glCallList(m_door_down_gllist);
      }
    }

    if (layer > 0)
    {
      glDisable(GL_BLEND);
    }

    glTranslated(0, 0, 0.0100);
    if (layer < 0)
    {
      glEnable(GL_BLEND);
      glDisable(GL_DEPTH_TEST);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glColor4d(0.0, 0.0, 0.0, 0.5-0.03*layer);
      glCallList(m_room_gllist);
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_BLEND);
    }
    else
      if (layer > 0)
    {
      glDisable(GL_LINE_STIPPLE);

      glEnable(GL_BLEND);
      glDisable(GL_DEPTH_TEST);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glColor4d(1.0, 1.0, 1.0, 0.1);
      glCallList(m_room_gllist);
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_BLEND);
    }

    if (!locks[room->getId()].empty()) //---> room is locked
    {
      glEnable(GL_BLEND);
      glDisable(GL_DEPTH_TEST);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glColor4d(0.6, 0.0, 0.0, 0.2);
      glCallList(m_room_gllist);
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_BLEND);
    }

    glPopMatrix();

    if (m_scaleFactor >= 0.15)
    {
      //draw connections and doors
      uint sourceId = room->getId();
      uint targetId;
      const Room *targetRoom;
      const ExitsList & exitslist = room->getExitsList();
      bool oneway;
      int rx;
      int ry;

      for (int i = 0; i < 7; i++)
      {
        uint targetDir = opposite(i);
        const Exit & sourceExit = exitslist[i];
        //outgoing connections
        std::set<uint>::const_iterator itOut = sourceExit.outBegin();
        while (itOut != sourceExit.outEnd()) {
          targetId = *itOut;
          targetRoom = rooms[targetId];
          rx = targetRoom->getPosition().x;
          ry = targetRoom->getPosition().y;
          if ( (targetId >= sourceId) || // draw exits if targetId >= sourceId ...
               ( ( ( rx < m_visibleX1-1) || (rx > m_visibleX2+1) ) ||  // or if target room is not visible
               ( ( ry < m_visibleY1-1) || (ry > m_visibleY2+1) ) ) )
          {
            if ( targetRoom->exit(targetDir).containsOut(sourceId) ) {
              oneway = false;
            } else {
              oneway = true;
              for (int j = 0; j < 7; ++j) {
                if (targetRoom->exit(j).containsOut(sourceId)) {
                  targetDir = j;
                  oneway = false;
                  break;
                }
              }
            }
            if (oneway)
              drawConnection( room, targetRoom, (ExitDirection)i, (ExitDirection)targetDir, true, ISSET(getFlags(room->exit(i)),EF_EXIT) );
            else
              drawConnection( room, targetRoom, (ExitDirection)i, (ExitDirection)targetDir, false, ISSET(getFlags(room->exit(i)),EF_EXIT) && ISSET(getFlags(targetRoom->exit(targetDir)),EF_EXIT) );
          }  else if (!sourceExit.containsIn(targetId)) { // ... or if they are outgoing oneways
            oneway = true;
            for (int j = 0; j < 7; ++j) {
              if (targetRoom->exit(j).containsOut(sourceId)) {
                targetDir = j;
                oneway = false;
                break;
              }
            }
            if (oneway)
              drawConnection( room, targetRoom, (ExitDirection)i, (ExitDirection)(opposite(i)), true, ISSET(getFlags(sourceExit),EF_EXIT) );
          }

               //incoming connections (only for oneway connections from rooms, that are not visible)
        std::set<uint>::const_iterator itIn = exitslist[i].inBegin();
        while (itIn != exitslist[i].inEnd())
        {
          targetId = *itIn;
          targetRoom = rooms[targetId];
          rx = targetRoom->getPosition().x;
          ry = targetRoom->getPosition().y;

          if ( ( ( rx < m_visibleX1-1) || (rx > m_visibleX2+1) ) ||
                   ( ( ry < m_visibleY1-1) || (ry > m_visibleY2+1) ) )
          {
            if ( !targetRoom->exit(opposite(i)).containsIn(sourceId) )
            {
              drawConnection( targetRoom, room, (ExitDirection)(opposite(i)), (ExitDirection)i, true, ISSET(getFlags(targetRoom->exit(opposite(i))),EF_EXIT) );
            }
          }
          ++itIn;
        }

         // draw door names
        if (Config().m_drawDoorNames && m_scaleFactor >= 0.40 &&
            ISSET(getFlags(room->exit((ExitDirection)i)), EF_DOOR) &&
            !getDoorName(room->exit((ExitDirection)i)).isEmpty())
        {
          if ( targetRoom->exit(opposite(i)).containsOut(sourceId))
            targetDir = opposite(i);
          else
          {
            for (int j = 0; j < 7; ++j) {
              if (targetRoom->exit(j).containsOut(sourceId))
              {
                targetDir = j;
                break;
              }
            }
          }
          drawRoomDoorName(room, i, targetRoom, targetDir);
        }

        ++itOut;
        }
      }
    }
  }

  void MapCanvas::drawConnection( const Room *leftRoom, const Room *rightRoom, ExitDirection connectionStartDirection, ExitDirection connectionEndDirection, bool oneway, bool inExitFlags )
  {
    assert(leftRoom != NULL);
    assert(rightRoom != NULL);

    qint32 leftX = leftRoom->getPosition().x;
    qint32 leftY = leftRoom->getPosition().y;
    qint32 leftZ = leftRoom->getPosition().z;
    qint32 rightX = rightRoom->getPosition().x;
    qint32 rightY = rightRoom->getPosition().y;
    qint32 rightZ = rightRoom->getPosition().z;
    qint32 dX = rightX - leftX;
    qint32 dY = rightY - leftY;
    qint32 dZ = rightZ - leftZ;

    qint32 leftLayer = leftZ - m_currentLayer;
    qint32 rightLayer = rightZ - m_currentLayer;

    if ((rightZ !=  m_currentLayer) &&
         (leftZ !=       m_currentLayer))
      return;

    bool neighbours = false;

    if ((dX == 0) && (dY == -1) && (dZ == 0)  )
    {
      if ( (connectionStartDirection == ED_NORTH) && (connectionEndDirection == ED_SOUTH) && !oneway )
        return;
      neighbours = true;
    }
    if ((dX == 0) && (dY == +1) && (dZ == 0) )
    {
      if ( (connectionStartDirection == ED_SOUTH) && (connectionEndDirection == ED_NORTH) && !oneway )
        return;
      neighbours = true;
    }
    if ((dX == +1) && (dY == 0) && (dZ == 0) )
    {
      if ( (connectionStartDirection == ED_EAST) && (connectionEndDirection == ED_WEST) && !oneway )
        return;
      neighbours = true;
    }
    if ((dX == -1) && (dY == 0) && (dZ == 0) )
    {
      if ( (connectionStartDirection == ED_WEST) && (connectionEndDirection == ED_EAST) && !oneway )
        return;
      neighbours = true;
    }

    glPushMatrix();
    glTranslated(leftX-0.5, leftY-0.5, 0.0);

    if (inExitFlags)
      glColor4d(1.0f, 1.0f, 1.0f, 0.70);
    else
      glColor4d(1.0f, 0.0f, 0.0f, 0.70);

    glEnable(GL_BLEND);
    glPointSize (2.0);
    glLineWidth (2.0);

    double srcZ = ROOM_Z_DISTANCE*leftLayer+0.3;

    switch (connectionStartDirection)
    {
      case ED_NORTH:
        if ( !oneway )
        {
          glBegin(GL_TRIANGLES);
          glVertex3d(0.68, +0.1, srcZ);
          glVertex3d(0.82, +0.1, srcZ);
          glVertex3d(0.75, +0.3, srcZ);
          glEnd();
        }
        glBegin(GL_LINE_STRIP);
        glVertex3d(0.75, 0.1, srcZ);
        glVertex3d(0.75, -0.1, srcZ);
        break;
      case ED_SOUTH:
        if ( !oneway )
        {
          glBegin(GL_TRIANGLES);
          glVertex3d(0.18, 0.9, srcZ);
          glVertex3d(0.32, 0.9, srcZ);
          glVertex3d(0.25, 0.7, srcZ);
          glEnd();
        }
        glBegin(GL_LINE_STRIP);
        glVertex3d(0.25, 0.9, srcZ);
        glVertex3d(0.25, 1.1, srcZ);
        break;
      case ED_EAST:
        if ( !oneway )
        {
          glBegin(GL_TRIANGLES);
          glVertex3d(0.9, 0.18, srcZ);
          glVertex3d(0.9, 0.32, srcZ);
          glVertex3d(0.7, 0.25, srcZ);
          glEnd();
        }
        glBegin(GL_LINE_STRIP);
        glVertex3d(0.9, 0.25, srcZ);
        glVertex3d(1.1, 0.25, srcZ);
        break;
      case ED_WEST:
        if ( !oneway )
        {
          glBegin(GL_TRIANGLES);
          glVertex3d(0.1, 0.68, srcZ);
          glVertex3d(0.1, 0.82, srcZ);
          glVertex3d(0.3, 0.75, srcZ);
          glEnd();
        }
        glBegin(GL_LINE_STRIP);
        glVertex3d(0.1, 0.75, srcZ);
        glVertex3d(-0.1, 0.75, srcZ);
        break;
      case ED_UP:
        if (!neighbours)
        {
          glBegin(GL_LINE_STRIP);
          glVertex3d(0.63, 0.25, srcZ);
          glVertex3d(0.55, 0.25, srcZ);
        }
        else
        {
          glBegin(GL_LINE_STRIP);
          glVertex3d(0.75, 0.25, srcZ);
        }
        break;
      case ED_DOWN:
        if (!neighbours)
        {
          glBegin(GL_LINE_STRIP);
          glVertex3d(0.37, 0.75, srcZ);
          glVertex3d(0.45, 0.75, srcZ);
        }
        else
        {
          glBegin(GL_LINE_STRIP);
          glVertex3d(0.25, 0.75, srcZ);
        }
        break;
      case ED_UNKNOWN:
        break;
      case ED_NONE:
        break;
    }

    double dstZ = ROOM_Z_DISTANCE*rightLayer+0.3;

    switch (connectionEndDirection)
    {
      case ED_NORTH:
        if(!oneway)
        {
          glVertex3d(dX+0.75, dY-0.1, dstZ);
          glVertex3d(dX+0.75, dY+0.1, dstZ);
          glEnd();
          glBegin(GL_TRIANGLES);
          glVertex3d(dX+0.68, dY+0.1, dstZ);
          glVertex3d(dX+0.82, dY+0.1, dstZ);
          glVertex3d(dX+0.75, dY+0.3, dstZ);
          glEnd();
        }
        else
        {
          glVertex3d(dX+0.25, dY-0.1, dstZ);
          glVertex3d(dX+0.25, dY+0.1, dstZ);
          glEnd();
          glBegin(GL_TRIANGLES);
          glVertex3d(dX+0.18, dY+0.1, dstZ);
          glVertex3d(dX+0.32, dY+0.1, dstZ);
          glVertex3d(dX+0.25, dY+0.3, dstZ);
          glEnd();
        }
        break;
      case ED_SOUTH:
        if(!oneway)
        {
          glVertex3d(dX+0.25, dY+1.1, dstZ);
          glVertex3d(dX+0.25, dY+0.9, dstZ);
          glEnd();
          glBegin(GL_TRIANGLES);
          glVertex3d(dX+0.18, dY+0.9, dstZ);
          glVertex3d(dX+0.32, dY+0.9, dstZ);
          glVertex3d(dX+0.25, dY+0.7, dstZ);
          glEnd();
        }
        else
        {
          glVertex3d(dX+0.75, dY+1.1, dstZ);
          glVertex3d(dX+0.75, dY+0.9, dstZ);
          glEnd();
          glBegin(GL_TRIANGLES);
          glVertex3d(dX+0.68, dY+0.9, dstZ);
          glVertex3d(dX+0.82, dY+0.9, dstZ);
          glVertex3d(dX+0.75, dY+0.7, dstZ);
          glEnd();
        }
        break;
      case ED_EAST:
        if(!oneway)
        {
          glVertex3d(dX+1.1, dY+0.25, dstZ);
          glVertex3d(dX+0.9, dY+0.25, dstZ);
          glEnd();
          glBegin(GL_TRIANGLES);
          glVertex3d(dX+0.9, dY+0.18, dstZ);
          glVertex3d(dX+0.9, dY+0.32, dstZ);
          glVertex3d(dX+0.7, dY+0.25, dstZ);
          glEnd();
        }
        else
        {
          glVertex3d(dX+1.1, dY+0.75, dstZ);
          glVertex3d(dX+0.9, dY+0.75, dstZ);
          glEnd();
          glBegin(GL_TRIANGLES);
          glVertex3d(dX+0.9, dY+0.68, dstZ);
          glVertex3d(dX+0.9, dY+0.82, dstZ);
          glVertex3d(dX+0.7, dY+0.75, dstZ);
          glEnd();
        }
        break;
      case ED_WEST:
        if(!oneway)
        {
          glVertex3d(dX-0.1, dY+0.75, dstZ);
          glVertex3d(dX+0.1, dY+0.75, dstZ);
          glEnd();
          glBegin(GL_TRIANGLES);
          glVertex3d(dX+0.1, dY+0.68, dstZ);
          glVertex3d(dX+0.1, dY+0.82, dstZ);
          glVertex3d(dX+0.3, dY+0.75, dstZ);
          glEnd();
        }
        else
        {
          glVertex3d(dX-0.1, dY+0.25, dstZ);
          glVertex3d(dX+0.1, dY+0.25, dstZ);
          glEnd();
          glBegin(GL_TRIANGLES);
          glVertex3d(dX+0.1, dY+0.18, dstZ);
          glVertex3d(dX+0.1, dY+0.32, dstZ);
          glVertex3d(dX+0.3, dY+0.25, dstZ);
          glEnd();
        }
        break;
      case ED_UP:
        if (!oneway)
        {
          if (!neighbours)
          {
            glVertex3d(dX+0.55, dY+0.25, dstZ);
            glVertex3d(dX+0.63, dY+0.25, dstZ);
            glEnd();
          }
          else
          {
            glVertex3d(dX+0.75, dY+0.25, dstZ);
            glEnd();
          }
          break;
        }
      case ED_DOWN:
        if (!oneway)
        {
          if (!neighbours)
          {
            glVertex3d(dX+0.45, dY+0.75, dstZ);
            glVertex3d(dX+0.37, dY+0.75, dstZ);
            glEnd();
          }
          else
          {
            glVertex3d(dX+0.25, dY+0.75, dstZ);
            glEnd();
          }
          break;
        }
      case ED_UNKNOWN:
        glVertex3d(dX+0.75, dY+0.75, dstZ);
        glVertex3d(dX+0.5, dY+0.5, dstZ);
        glEnd();
        glBegin(GL_TRIANGLES);
        glVertex3d(dX+0.5, dY+0.5, dstZ);
        glVertex3d(dX+0.7, dY+0.55, dstZ);
        glVertex3d(dX+0.55, dY+0.7, dstZ);
        glEnd();
        break;
      case ED_NONE:
        glVertex3d(dX+0.75, dY+0.75, dstZ);
        glVertex3d(dX+0.5, dY+0.5, dstZ);
        glEnd();
        glBegin(GL_TRIANGLES);
        glVertex3d(dX+0.5, dY+0.5, dstZ);
        glVertex3d(dX+0.7, dY+0.55, dstZ);
        glVertex3d(dX+0.55, dY+0.7, dstZ);
        glEnd();
        break;
    }

    glDisable(GL_BLEND);
    glColor4d(1.0f, 1.0f, 1.0f, 0.70);

    glPopMatrix();
  }


  void MapCanvas::normalizeAngle(int *angle)
  {
    while (*angle < 0)
      *angle += 360 * 16;
    while (*angle > 360 * 16)
      *angle -= 360 * 16;
  }


  void MapCanvas::makeGlLists()
  {
    m_wall_north_gllist = glGenLists(1);
    glNewList(m_wall_north_gllist, GL_COMPILE);
    glBegin(GL_LINES);
    glVertex3d(0.0, 0.0+ROOM_WALL_ALIGN, 0.0);
    glVertex3d(1.0, 0.0+ROOM_WALL_ALIGN, 0.0);
    glEnd();
    glEndList();

    m_wall_south_gllist = glGenLists(1);
    glNewList(m_wall_south_gllist, GL_COMPILE);
    glBegin(GL_LINES);
    glVertex3d(0.0, 1.0-ROOM_WALL_ALIGN, 0.0);
    glVertex3d(1.0, 1.0-ROOM_WALL_ALIGN, 0.0);
    glEnd();
    glEndList();

    m_wall_east_gllist = glGenLists(1);
    glNewList(m_wall_east_gllist, GL_COMPILE);
    glBegin(GL_LINES);
    glVertex3d(1.0-ROOM_WALL_ALIGN, 0.0, 0.0);
    glVertex3d(1.0-ROOM_WALL_ALIGN, 1.0, 0.0);
    glEnd();
    glEndList();

    m_wall_west_gllist = glGenLists(1);
    glNewList(m_wall_west_gllist, GL_COMPILE);
    glBegin(GL_LINES);
    glVertex3d(0.0+ROOM_WALL_ALIGN, 0.0, 0.0);
    glVertex3d(0.0+ROOM_WALL_ALIGN, 1.0, 0.0);
    glEnd();
    glEndList();

    m_exit_up_gllist = glGenLists(1);
    glNewList(m_exit_up_gllist, GL_COMPILE);
    glColor4d(255, 255, 255, 255);
    glBegin(GL_POLYGON);
    glVertex3d(0.75, 0.13, 0.0);
    glVertex3d(0.83, 0.17, 0.0);
    glVertex3d(0.87, 0.25, 0.0);
    glVertex3d(0.83, 0.33, 0.0);
    glVertex3d(0.75, 0.37, 0.0);
    glVertex3d(0.67, 0.33, 0.0);
    glVertex3d(0.63, 0.25, 0.0);
    glVertex3d(0.67, 0.17, 0.0);
    glEnd();
    glColor4d(0, 0, 0, 255);
    glBegin(GL_LINE_LOOP);
    glVertex3d(0.75, 0.13, 0.01);
    glVertex3d(0.83, 0.17, 0.01);
    glVertex3d(0.87, 0.25, 0.01);
    glVertex3d(0.83, 0.33, 0.01);
    glVertex3d(0.75, 0.37, 0.01);
    glVertex3d(0.67, 0.33, 0.01);
    glVertex3d(0.63, 0.25, 0.01);
    glVertex3d(0.67, 0.17, 0.01);
    glEnd();
    glBegin(GL_POINTS);
    glVertex3d(0.75, 0.25, 0.01);
    glEnd();
    glEndList();

    m_exit_up_transparent_gllist = glGenLists(1);
    glNewList(m_exit_up_transparent_gllist, GL_COMPILE);
    glBegin(GL_LINE_LOOP);
    glVertex3d(0.75, 0.13, 0.01);
    glVertex3d(0.83, 0.17, 0.01);
    glVertex3d(0.87, 0.25, 0.01);
    glVertex3d(0.83, 0.33, 0.01);
    glVertex3d(0.75, 0.37, 0.01);
    glVertex3d(0.67, 0.33, 0.01);
    glVertex3d(0.63, 0.25, 0.01);
    glVertex3d(0.67, 0.17, 0.01);
    glEnd();
    glBegin(GL_POINTS);
    glVertex3d(0.75, 0.25, 0.01);
    glEnd();
    glEndList();

    m_exit_down_gllist = glGenLists(1);
    glNewList(m_exit_down_gllist, GL_COMPILE);
    glColor4d(255, 255, 255, 255);
    glBegin(GL_POLYGON);
    glVertex3d(0.25, 0.63, 0.0);
    glVertex3d(0.33, 0.67, 0.0);
    glVertex3d(0.37, 0.75, 0.0);
    glVertex3d(0.33, 0.83, 0.0);
    glVertex3d(0.25, 0.87, 0.0);
    glVertex3d(0.17, 0.83, 0.0);
    glVertex3d(0.13, 0.75, 0.0);
    glVertex3d(0.17, 0.67, 0.0);
    glEnd();
    glColor4d(0, 0, 0, 255);
    glBegin(GL_LINE_LOOP);
    glVertex3d(0.25, 0.63, 0.01);
    glVertex3d(0.33, 0.67, 0.01);
    glVertex3d(0.37, 0.75, 0.01);
    glVertex3d(0.33, 0.83, 0.01);
    glVertex3d(0.25, 0.87, 0.01);
    glVertex3d(0.17, 0.83, 0.01);
    glVertex3d(0.13, 0.75, 0.01);
    glVertex3d(0.17, 0.67, 0.01);
    glEnd();
    glBegin(GL_LINES);
    glVertex3d(0.33, 0.67, 0.01);
    glVertex3d(0.17, 0.83, 0.01);
    glVertex3d(0.33, 0.83, 0.01);
    glVertex3d(0.17, 0.67, 0.01);
    glEnd();
    glEndList();

    m_exit_down_transparent_gllist = glGenLists(1);
    glNewList(m_exit_down_transparent_gllist, GL_COMPILE);
    glBegin(GL_LINE_LOOP);
    glVertex3d(0.25, 0.63, 0.01);
    glVertex3d(0.33, 0.67, 0.01);
    glVertex3d(0.37, 0.75, 0.01);
    glVertex3d(0.33, 0.83, 0.01);
    glVertex3d(0.25, 0.87, 0.01);
    glVertex3d(0.17, 0.83, 0.01);
    glVertex3d(0.13, 0.75, 0.01);
    glVertex3d(0.17, 0.67, 0.01);
    glEnd();
    glBegin(GL_LINES);
    glVertex3d(0.33, 0.67, 0.01);
    glVertex3d(0.17, 0.83, 0.01);
    glVertex3d(0.33, 0.83, 0.01);
    glVertex3d(0.17, 0.67, 0.01);
    glEnd();
    glEndList();

    m_door_north_gllist = glGenLists(1);
    glNewList(m_door_north_gllist, GL_COMPILE);
    glBegin(GL_LINES);
    glVertex3d(0.5, 0.0, 0.0);
    glVertex3d(0.5, 0.11, 0.0);
    glVertex3d(0.35, 0.11, 0.0);
    glVertex3d(0.65, 0.11, 0.0);
    glEnd();
    glEndList();

    m_door_south_gllist = glGenLists(1);
    glNewList(m_door_south_gllist, GL_COMPILE);
    glBegin(GL_LINES);
    glVertex3d(0.5, 1.0, 0.0);
    glVertex3d(0.5, 0.89, 0.0);
    glVertex3d(0.35, 0.89, 0.0);
    glVertex3d(0.65, 0.89, 0.0);
    glEnd();
    glEndList();

    m_door_east_gllist = glGenLists(1);
    glNewList(m_door_east_gllist, GL_COMPILE);
    glBegin(GL_LINES);
    glVertex3d(0.89, 0.5, 0.0);
    glVertex3d(1.0, 0.5, 0.0);
    glVertex3d(0.89, 0.35, 0.0);
    glVertex3d(0.89, 0.65, 0.0);
    glEnd();
    glEndList();

    m_door_west_gllist = glGenLists(1);
    glNewList(m_door_west_gllist, GL_COMPILE);
    glBegin(GL_LINES);
    glVertex3d(0.11, 0.5, 0.0);
    glVertex3d(0.0, 0.5, 0.0);
    glVertex3d(0.11, 0.35, 0.0);
    glVertex3d(0.11, 0.65, 0.0);
    glEnd();
    glEndList();

    m_door_up_gllist = glGenLists(1);
    glNewList(m_door_up_gllist, GL_COMPILE);
    glLineWidth (3.0);
    glBegin(GL_LINES);
    glVertex3d(0.69, 0.31, 0.0);
    glVertex3d(0.63, 0.37, 0.0);
    glVertex3d(0.57, 0.31, 0.0);
    glVertex3d(0.69, 0.43, 0.0);
    glEnd();
    glLineWidth (2.0);
    glEndList();

    m_door_down_gllist = glGenLists(1);
    glNewList(m_door_down_gllist, GL_COMPILE);
    glLineWidth (3.0);
    glBegin(GL_LINES);
    glVertex3d(0.31, 0.69, 0.0);
    glVertex3d(0.37, 0.63, 0.0);
    glVertex3d(0.31, 0.57, 0.0);
    glVertex3d(0.43, 0.69, 0.0);
    glEnd();
    glLineWidth (2.0);
    glEndList();


    m_room_gllist = glGenLists(1);
    glNewList(m_room_gllist, GL_COMPILE);
    glBegin(GL_QUADS);
#if (QT_VERSION < 0x040700 || QT_VERSION >= 0x040800)
    glTexCoord2d(0, 1);
    glVertex3d(0.0, 0.0, 0.0);
    glTexCoord2d(0, 0);
    glVertex3d(0.0, 1.0, 0.0);
    glTexCoord2d(1, 0);
    glVertex3d(1.0, 1.0, 0.0);
    glTexCoord2d(1, 1);
    glVertex3d(1.0, 0.0, 0.0);
#else
    glTexCoord2d(0, 1);
    glVertex3d(0.0, 1.0, 0.0);
    glTexCoord2d(0, 0);
    glVertex3d(0.0, 0.0, 0.0);
    glTexCoord2d(1, 0);
    glVertex3d(1.0, 0.0, 0.0);
    glTexCoord2d(1, 1);
    glVertex3d(1.0, 1.0, 0.0);
#endif
    glEnd();
    glEndList();

    m_room_selection_gllist = glGenLists(1);
    glNewList(m_room_selection_gllist, GL_COMPILE);
    glBegin(GL_LINE_LOOP);
    glVertex3d(-0.2, -0.2, 0.0);
    glVertex3d(-0.2, 1.2, 0.0);
    glVertex3d(1.2, 1.2, 0.0);
    glVertex3d(1.2, -0.2, 0.0);
    glEnd();
    glEndList();

    m_room_selection_inner_gllist = glGenLists(1);
    glNewList(m_room_selection_inner_gllist, GL_COMPILE);
    glBegin(GL_QUADS);
    glVertex3d(-0.2, -0.2, 0.0);
    glVertex3d(-0.2, 1.2, 0.0);
    glVertex3d(1.2, 1.2, 0.0);
    glVertex3d(1.2, -0.2, 0.0);
    glEnd();
    glEndList();

    return;
  }


  GLint MapCanvas::UnProject(GLdouble winx, GLdouble winy, GLdouble winz, GLdouble *objx, GLdouble *objy, GLdouble *objz)
  {
    double finalMatrix[16];
    double in[4];
    double out[4];

    MultMatricesd(m_modelview, m_projection, finalMatrix);
    if (!InvertMatrixd(finalMatrix, finalMatrix)) return(GL_FALSE);

    in[0]=winx;
    in[1]=winy;
    in[2]=winz;
    in[3]=1.0;

    /* Map x and y from window coordinates */
    in[0] = (in[0] - m_viewport[0]) / m_viewport[2];
    in[1] = (in[1] - m_viewport[1]) / m_viewport[3];

    /* Map to range -1 to 1 */
    in[0] = in[0] * 2 - 1;
    in[1] = in[1] * 2 - 1;
    in[2] = in[2] * 2 - 1;

    MultMatrixVecd(finalMatrix, in, out);
    if (out[3] == 0.0) return(GL_FALSE);
    out[0] /= out[3];
    out[1] /= out[3];
    out[2] /= out[3];
    *objx = out[0];
    *objy = out[1];
    *objz = out[2];
    return(GL_TRUE);
  }

  GLint MapCanvas::Project(GLdouble objx, GLdouble objy, GLdouble objz, GLdouble *winx, GLdouble *winy, GLdouble *winz)
  {
    double in[4];
    double out[4];

    in[0]=objx;
    in[1]=objy;
    in[2]=objz;
    in[3]=1.0;
    MultMatrixVecd(m_modelview, in, out);
    MultMatrixVecd(m_projection, out, in);
    if (in[3] == 0.0) return(GL_FALSE);
    in[0] /= in[3];
    in[1] /= in[3];
    in[2] /= in[3];
    /* Map x, y and z to range 0-1 */
    in[0] = in[0] * 0.5 + 0.5;
    in[1] = in[1] * 0.5 + 0.5;
    in[2] = in[2] * 0.5 + 0.5;

    /* Map x,y to viewport */
    in[0] = in[0] * m_viewport[2] + m_viewport[0];
    in[1] = in[1] * m_viewport[3] + m_viewport[1];

    *winx=in[0];
    *winy=in[1];
    *winz=in[2];
    return(GL_TRUE);
  }



  void MapCanvas::MultMatrixVecd(const GLdouble matrix[16], const GLdouble in[4], GLdouble out[4])
  {
    int i;

    for (i=0; i<4; i++) {
      out[i] =
          in[0] * matrix[0*4+i] +
          in[1] * matrix[1*4+i] +
          in[2] * matrix[2*4+i] +
          in[3] * matrix[3*4+i];
    }
  }

  void MapCanvas::MultMatricesd(const GLdouble a[16], const GLdouble b[16], GLdouble r[16])
  {
    int i, j;

    for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
        r[i*4+j] =
            a[i*4+0]*b[0*4+j] +
            a[i*4+1]*b[1*4+j] +
            a[i*4+2]*b[2*4+j] +
            a[i*4+3]*b[3*4+j];
      }
    }
  }

  void MapCanvas::MakeIdentityd(GLdouble m[16])
  {
    m[0+4*0] = 1; m[0+4*1] = 0; m[0+4*2] = 0; m[0+4*3] = 0;
    m[1+4*0] = 0; m[1+4*1] = 1; m[1+4*2] = 0; m[1+4*3] = 0;
    m[2+4*0] = 0; m[2+4*1] = 0; m[2+4*2] = 1; m[2+4*3] = 0;
    m[3+4*0] = 0; m[3+4*1] = 0; m[3+4*2] = 0; m[3+4*3] = 1;
  }

  int MapCanvas::InvertMatrixd(const GLdouble src[16], GLdouble inverse[16])
  {
    int i, j, k, swap;
    double t;
    GLdouble temp[4][4];

    for (i=0; i<4; i++) {
      for (j=0; j<4; j++) {
        temp[i][j] = src[i*4+j];
      }
    }
    MakeIdentityd(inverse);

    for (i = 0; i < 4; i++) {
        /*
      ** Look for largest element in column
        */
      swap = i;
      for (j = i + 1; j < 4; j++) {
        if (fabs(temp[j][i]) > fabs(temp[i][i])) {
          swap = j;
        }
      }

      if (swap != i) {
            /*
        ** Swap rows.
            */
        for (k = 0; k < 4; k++) {
          t = temp[i][k];
          temp[i][k] = temp[swap][k];
          temp[swap][k] = t;

          t = inverse[i*4+k];
          inverse[i*4+k] = inverse[swap*4+k];
          inverse[swap*4+k] = t;
        }
      }

      if (temp[i][i] == 0) {
            /*
        ** No non-zero pivot.  The matrix is singular, which shouldn't
        ** happen.  This means the user gave us a bad matrix.
            */
        return GL_FALSE;
      }

      t = temp[i][i];
      for (k = 0; k < 4; k++) {
        temp[i][k] /= t;
        inverse[i*4+k] /= t;
      }
      for (j = 0; j < 4; j++) {
        if (j != i) {
          t = temp[j][i];
          for (k = 0; k < 4; k++) {
            temp[j][k] -= temp[i][k]*t;
            inverse[j*4+k] -= inverse[i*4+k]*t;
          }
        }
      }
    }
    return GL_TRUE;
  }

  void MapCanvas::update()
  {
    updateGL();
  }
  
float MapCanvas::getDW() const {
    return ((float)(((float)width() / ((float)BASESIZEX / 12.0f)) ) / (float)m_scaleFactor);
}

float MapCanvas::getDH() const {
    return ((float)(((float)height() / ((float)BASESIZEY / 12.0f)) ) / (float)m_scaleFactor);
}
