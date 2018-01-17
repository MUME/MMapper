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
#include "mmapper2group.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "customaction.h"

#include <assert.h>
#include <math.h>

#include <QWheelEvent>
#include <QPainter>
#include <QOpenGLTexture>
#include <QOpenGLDebugLogger>
#include <QDebug>

#define ROOM_Z_DISTANCE (7.0f)
#define ROOM_WALL_ALIGN (0.008f)
#define CAMERA_Z_DISTANCE (0.978f)
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
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55
};


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
    0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22
};

QColor MapCanvas::m_noFleeColor = QColor(123, 63, 0);

MapCanvas::MapCanvas( MapData *mapData, PrespammedPath *prespammedPath, Mmapper2Group *groupManager,
                      QWidget *parent )
    : QOpenGLWidget(parent),
      m_scrollX(0),
      m_scrollY(0),
      m_currentLayer(0),
      m_selectedArea(false), //no area selected at start time
      m_infoMarkSelection(false),
      m_canvasMouseMode(CMM_MOVE),
      m_roomSelection(NULL),
      m_connectionSelection(NULL),
      m_mouseRightPressed(false),
      m_mouseLeftPressed(false),
      m_altPressed(false),
      m_ctrlPressed(false),
      m_scaleFactor(1.0f), // scale rooms
      m_data(mapData),
      m_prespammedPath(prespammedPath),
      m_groupManager(groupManager)
{
    m_glFont = new QFont(QFont(), this);
    m_glFont->setStyleHint(QFont::System, QFont::OpenGLCompatible);

    m_glFontMetrics = new QFontMetrics(*m_glFont);

    m_infoMarksEditDlg = new InfoMarksEditDlg(mapData, this);
    connect(m_infoMarksEditDlg, SIGNAL(mapChanged()), this, SLOT(update()));
    connect(m_infoMarksEditDlg, SIGNAL(closeEventReceived()), this, SLOT(onInfoMarksEditDlgClose()));
    connect(m_data, SIGNAL(updateCanvas()), this, SLOT(update()));

    memset(m_terrainTextures, 0, sizeof(m_terrainTextures));
    memset(m_roadTextures, 0, sizeof(m_roadTextures));
    memset(m_loadTextures, 0, sizeof(m_loadTextures));
    memset(m_mobTextures, 0, sizeof(m_mobTextures));
    memset(m_trailTextures, 0, sizeof(m_trailTextures));
    m_updateTexture = 0;

    int samples = Config().m_antialiasingSamples;
    if (samples <= 0) samples = 2; // Default to 2 samples to prevent restart
    QSurfaceFormat format;
    format.setVersion(2, 1);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setSamples(samples);
    setFormat(format);
}


void MapCanvas::onInfoMarksEditDlgClose()
{
    m_infoMarkSelection = false;
    makeCurrent();
    update();
}

MapCanvas::~MapCanvas()
{
    // Make sure the context is current and then explicitly
    // destroy all underlying OpenGL resources.
    makeCurrent();

    for (int i = 0; i < 16; i++) {
        if (m_terrainTextures[i]) delete m_terrainTextures[i];
        if (m_roadTextures[i]) delete m_roadTextures[i];
        if (m_loadTextures[i]) delete m_loadTextures[i];
        if (m_trailTextures[i]) delete m_trailTextures[i];
        if (i < 15) {
            if (m_mobTextures[i]) delete m_mobTextures[i];
        }
    }
    delete m_updateTexture;

    if (m_roomSelection) m_data->unselect(m_roomSelection);
    if (m_connectionSelection) delete m_connectionSelection;
    if ( m_glFont ) delete m_glFont;
    if ( m_glFontMetrics ) delete m_glFontMetrics;
    doneCurrent();
}

inline static void loadMatrix(const QMatrix4x4 &m)
{
    // static to prevent glLoadMatrixf to fail on certain drivers
    static GLfloat mat[16];
    const float *data = m.constData();
    for (int index = 0; index < 16; ++index)
        mat[index] = data[index];
    glLoadMatrixf(mat);
}

float MapCanvas::SCROLLFACTOR()
{
    return 0.2f;
}

void MapCanvas::layerUp()
{
    m_currentLayer++;
    update();
}

void MapCanvas::layerDown()
{
    m_currentLayer--;
    update();
}

int inline MapCanvas::GLtoMap(double arg)
{
    if (arg >= 0)
        return (int)(arg + 0.5f);
    return (int)(arg - 0.5f);
}

void MapCanvas::setCanvasMouseMode(CanvasMouseMode mode)
{
    m_canvasMouseMode = mode;

    if (m_canvasMouseMode != CMM_EDIT_INFOMARKS) {
        m_infoMarksEditDlg->hide();
        m_infoMarkSelection = false;
    }

    clearRoomSelection();
    clearConnectionSelection();

    m_selectedArea = false;
    update();
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

void MapCanvas::wheelEvent ( QWheelEvent *event )
{
    if (event->delta() > 100) {
        switch (m_canvasMouseMode) {
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
        default:
            ;
        }
    }

    if (event->delta() < -100) {
        switch (m_canvasMouseMode) {
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
        default:
            ;
        }
    }

}

void MapCanvas::forceMapperToRoom()
{
    const RoomSelection *tmpSel = NULL;
    if (m_roomSelection) {
        tmpSel = m_roomSelection;
        m_roomSelection = 0;
    } else {
        tmpSel = m_data->select(Coordinate(GLtoMap(m_selX1), GLtoMap(m_selY1), m_selLayer1));
    }
    if (tmpSel->size() == 1) {
        if (Config().m_mapMode == 2) {
            const Room *r = tmpSel->values().front();
            emit charMovedEvent(Mmapper2Event::createEvent( CID_UNKNOWN, Mmapper2Room::getName(r),
                                                            Mmapper2Room::getDynamicDescription(r),
                                                            Mmapper2Room::getDescription(r), 0, 0, 0));
        } else
            emit setCurrentRoom(tmpSel->keys().front());
    }
    m_data->unselect(tmpSel);
    update();
}

void MapCanvas::mousePressEvent(QMouseEvent *event)
{
    QVector3D v = QVector3D(event->pos().x(), height() - event->pos().y(), CAMERA_Z_DISTANCE)
                  .unproject(m_model, m_projection, this->rect());
    m_selX1 = v.x();
    m_selY1 = v.y();

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


    switch (m_canvasMouseMode) {
    case CMM_EDIT_INFOMARKS:
        m_infoMarkSelection = true;
        m_infoMarksEditDlg->hide();
        update();
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
                (event->modifiers() & Qt::ALT ) ) {
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

            if (event->modifiers() != Qt::CTRL) {
                const RoomSelection *tmpSel = m_data->select(Coordinate(GLtoMap(m_selX1), GLtoMap(m_selY1),
                                                                        m_selLayer1));
                if (m_roomSelection && tmpSel->size() > 0 && m_roomSelection->contains( tmpSel->keys().front() )) {
                    m_roomSelectionMove = true;
                    m_roomSelectionMoveX = 0;
                    m_roomSelectionMoveY = 0;
                    m_roomSelectionMoveWrongPlace = false;
                } else {
                    m_roomSelectionMove = false;
                    m_selectedArea = false;
                    if (m_roomSelection != NULL) m_data->unselect(m_roomSelection);
                    m_roomSelection = NULL;
                    emit newRoomSelection(m_roomSelection);
                }
                m_data->unselect(tmpSel);
            } else {
                m_ctrlPressed = true;
            }
        }
        update();
        break;

    case CMM_CREATE_ONEWAY_CONNECTIONS:
    case CMM_CREATE_CONNECTIONS:
        if (event->buttons() & Qt::LeftButton) {
            if (m_connectionSelection != NULL) delete m_connectionSelection;
            m_connectionSelection = new ConnectionSelection(m_data, m_selX1, m_selY1, m_selLayer1);
            if (!m_connectionSelection->isFirstValid()) {
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
        update();
        break;

    case CMM_SELECT_CONNECTIONS:
        if (event->buttons() & Qt::LeftButton) {
            if (m_connectionSelection != NULL) delete m_connectionSelection;
            m_connectionSelection = new ConnectionSelection(m_data, m_selX1, m_selY1, m_selLayer1);
            if (!m_connectionSelection->isFirstValid()) {
                if (m_connectionSelection != NULL) delete m_connectionSelection;
                m_connectionSelection = NULL;
            } else {
                const Room *r1 = m_connectionSelection->getFirst().room;
                ExitDirection dir1 = m_connectionSelection->getFirst().direction;

                if ( r1->exit(dir1).outBegin() == r1->exit(dir1).outEnd() ) {
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
        update();
        break;

    case CMM_CREATE_ROOMS: {
        const RoomSelection *tmpSel = m_data->select(Coordinate(GLtoMap(m_selX1), GLtoMap(m_selY1),
                                                                m_selLayer1));
        if (tmpSel->size() == 0) {
            //Room * r = new Room(m_data);
            Coordinate c = Coordinate(GLtoMap(m_selX1), GLtoMap(m_selY1), m_currentLayer);
            m_data->createEmptyRoom(c);
            //m_data->execute(new SingleRoomAction(new ConnectToNeighbours, id));
        }
        m_data->unselect(tmpSel);
    }
    update();
    break;


    default:
        break;
    }
}


void MapCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (m_canvasMouseMode != CMM_MOVE) {
        qint8 hScroll, vScroll;
        if (event->pos().y() < 100 )
            vScroll = -1;
        else if (event->pos().y() > height() - 100 )
            vScroll = 1;
        else
            vScroll = 0;

        if (event->pos().x() < 100 )
            hScroll = -1;
        else if (event->pos().x() > width() - 100 )
            hScroll = 1;
        else
            hScroll = 0;

        continuousScroll(hScroll, vScroll);
    }

    QVector3D v = QVector3D(event->pos().x(), height() - event->pos().y(), CAMERA_Z_DISTANCE)
                  .unproject(m_model, m_projection, this->rect());
    m_selX2 = v.x();
    m_selY2 = v.y();
    m_selLayer2 = m_currentLayer;

    switch (m_canvasMouseMode) {
    case CMM_EDIT_INFOMARKS:
        update();
        break;
    case CMM_MOVE:
        if (event->buttons() & Qt::LeftButton && m_mouseLeftPressed) {

            int idx = (int)((m_selX2 - m_moveX1backup) / SCROLLFACTOR());
            int idy = (int)((m_selY2 - m_moveY1backup) / SCROLLFACTOR());

            emit mapMove(-idx, -idy);

            if (idx != 0) m_moveX1backup = m_selX2 - idx * SCROLLFACTOR();
            if (idy != 0) m_moveY1backup = m_selY2 - idy * SCROLLFACTOR();
        }
        break;

    case CMM_SELECT_ROOMS:
        if (event->buttons() & Qt::LeftButton) {

            if (m_roomSelectionMove) {
                m_roomSelectionMoveX = GLtoMap(m_selX2) - GLtoMap(m_selX1);
                m_roomSelectionMoveY = GLtoMap(m_selY2) - GLtoMap(m_selY1);

                Coordinate c(m_roomSelectionMoveX, m_roomSelectionMoveY, 0);
                if (m_data->isMovable(c, m_roomSelection))
                    m_roomSelectionMoveWrongPlace = false;
                else
                    m_roomSelectionMoveWrongPlace = true;
            } else {
                m_selectedArea = true;
            }
        }
        update();
        break;

    case CMM_CREATE_ONEWAY_CONNECTIONS:
    case CMM_CREATE_CONNECTIONS:
        if (event->buttons() & Qt::LeftButton) {

            if (m_connectionSelection != NULL) {
                m_connectionSelection->setSecond(m_data, m_selX2, m_selY2, m_selLayer2);

                const Room *r1 = m_connectionSelection->getFirst().room;
                ExitDirection dir1 = m_connectionSelection->getFirst().direction;
                const Room *r2 = m_connectionSelection->getSecond().room;
                ExitDirection dir2 = m_connectionSelection->getSecond().direction;

                if (r2) {
                    if ( (r1->exit(dir1).containsOut(r2->getId())) && (r2->exit(dir2).containsOut(r1->getId())) ) {
                        m_connectionSelection->removeSecond();
                    }
                }
                update();
            }
        }
        break;

    case CMM_SELECT_CONNECTIONS:
        if (event->buttons() & Qt::LeftButton) {

            if (m_connectionSelection != NULL) {
                m_connectionSelection->setSecond(m_data, m_selX2, m_selY2, m_selLayer2);

                const Room *r1 = m_connectionSelection->getFirst().room;
                ExitDirection dir1 = m_connectionSelection->getFirst().direction;
                const Room *r2 = m_connectionSelection->getSecond().room;
                ExitDirection dir2 = m_connectionSelection->getSecond().direction;

                if (r2) {
                    if ( !(r1->exit(dir1).containsOut(r2->getId()))
                            || !(r2->exit(dir2).containsOut(r1->getId())) ) { //not two ways
                        if ( dir2 != ED_UNKNOWN)
                            m_connectionSelection->removeSecond();
                        else if ( dir2 == ED_UNKNOWN && (!(r1->exit(dir1).containsOut(r2->getId()))
                                                         || (r1->exit(dir1).containsIn(r2->getId()))) ) //not oneway
                            m_connectionSelection->removeSecond();
                    }
                }
                update();
            }
        }
        break;

    case CMM_CREATE_ROOMS:
        break;


    default:
        break;
    }
}

void MapCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    continuousScroll(0, 0);
    QVector3D v = QVector3D(event->pos().x(), height() - event->pos().y(), CAMERA_Z_DISTANCE)
                  .unproject(m_model, m_projection, this->rect());
    m_selX2 = v.x();
    m_selY2 = v.y();
    m_selLayer2 = m_currentLayer;

    switch (m_canvasMouseMode) {
    case CMM_EDIT_INFOMARKS:
        if ( m_mouseLeftPressed == true ) {
            m_mouseLeftPressed = false;
            m_infoMarksEditDlg->setPoints(m_selX1, m_selY1, m_selX2, m_selY2, m_selLayer1);
            m_infoMarksEditDlg->show();
        } else {
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

            if (m_roomSelectionMove) {
                m_roomSelectionMove = false;
                if (m_roomSelectionMoveWrongPlace == false && m_roomSelection) {

                    Coordinate moverel(m_roomSelectionMoveX, m_roomSelectionMoveY, 0);
                    m_data->execute(new GroupAction(new MoveRelative(moverel), m_roomSelection), m_roomSelection);
                }
            } else {
                if (m_roomSelection == NULL) {
                    //add rooms to default selections
                    m_roomSelection = m_data->select(Coordinate(GLtoMap(m_selX1), GLtoMap(m_selY1), m_selLayer1),
                                                     Coordinate(GLtoMap(m_selX2), GLtoMap(m_selY2), m_selLayer2));
                } else {
                    //add or remove rooms to/from default selection
                    const RoomSelection *tmpSel = m_data->select(Coordinate(GLtoMap(m_selX1), GLtoMap(m_selY1),
                                                                            m_selLayer1), Coordinate(GLtoMap(m_selX2), GLtoMap(m_selY2), m_selLayer2));
                    QList<uint> keys = tmpSel->keys();
                    for (int i = 0; i < keys.size(); ++i) {
                        if ( m_roomSelection->contains( keys.at(i) ) ) {
                            m_data->unselect(keys.at(i), m_roomSelection);
                        } else {
                            m_data->getRoom(keys.at(i), m_roomSelection);
                        }
                    }
                    m_data->unselect(tmpSel);
                }

                if (m_roomSelection->size() < 1) {
                    m_data->unselect(m_roomSelection);
                    m_roomSelection = NULL;
                } else {
                    if (m_roomSelection->size() == 1) {
                        const Room *r = m_roomSelection->values().front();
                        qint32 x = r->getPosition().x;
                        qint32 y = r->getPosition().y;

                        QString etmp = "Exits:";
                        for (int j = 0; j < 7; j++) {

                            bool door = false;
                            if (ISSET(Mmapper2Exit::getFlags(r->exit(j)), EF_DOOR)) {
                                door = true;
                                etmp += " (";
                            }

                            if (ISSET(Mmapper2Exit::getFlags(r->exit(j)), EF_EXIT)) {
                                if (!door) etmp += " ";

                                switch (j) {
                                case 0:
                                    etmp += "north";
                                    break;
                                case 1:
                                    etmp += "south";
                                    break;
                                case 2:
                                    etmp += "east";
                                    break;
                                case 3:
                                    etmp += "west";
                                    break;
                                case 4:
                                    etmp += "up";
                                    break;
                                case 5:
                                    etmp += "down";
                                    break;
                                case 6:
                                    etmp += "unknown";
                                    break;
                                }
                            }

                            if (door) {
                                if (Mmapper2Exit::getDoorName(r->exit(j)) != "")
                                    etmp += "/" + Mmapper2Exit::getDoorName(r->exit(j)) + ")";
                                else
                                    etmp += ")";
                            }
                        }
                        etmp += ".\n";
                        QString ctemp = QString("Selected Room Coordinates: %1 %2").arg(x).arg(y);
                        emit log( "MapCanvas", ctemp + "\n" + Mmapper2Room::getName(r) + "\n" +
                                  Mmapper2Room::getDescription(r) + Mmapper2Room::getDynamicDescription(
                                      r) + etmp);

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
        update();
        break;

    case CMM_CREATE_ONEWAY_CONNECTIONS:
    case CMM_CREATE_CONNECTIONS:
        if ( m_mouseLeftPressed == true ) {
            m_mouseLeftPressed = false;

            if (m_connectionSelection == NULL)
                m_connectionSelection = new ConnectionSelection(m_data, m_selX1, m_selY1, m_selLayer1);
            m_connectionSelection->setSecond(m_data, m_selX2, m_selY2, m_selLayer2);

            if (!m_connectionSelection->isValid()) {
                if (m_connectionSelection != NULL) delete m_connectionSelection;
                m_connectionSelection = NULL;
            } else {
                const Room *r1 = m_connectionSelection->getFirst().room;
                ExitDirection dir1 = m_connectionSelection->getFirst().direction;
                const Room *r2 = m_connectionSelection->getSecond().room;
                ExitDirection dir2 = m_connectionSelection->getSecond().direction;

                if (r2) {
                    const RoomSelection *tmpSel = m_data->select();
                    m_data->getRoom(r1->getId(), tmpSel);
                    m_data->getRoom(r2->getId(), tmpSel);

                    if (m_connectionSelection != NULL) delete m_connectionSelection;
                    m_connectionSelection = NULL;

                    if ( !(r1->exit(dir1).containsOut(r2->getId())) || !(r2->exit(dir2).containsOut(r1->getId())) ) {
                        if (m_canvasMouseMode != CMM_CREATE_ONEWAY_CONNECTIONS) {
                            m_data->execute(new AddTwoWayExit(r1->getId(), r2->getId(), dir1, dir2), tmpSel);
                        } else {
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
        update();
        break;

    case CMM_SELECT_CONNECTIONS:
        if ( m_mouseLeftPressed == true ) {
            m_mouseLeftPressed = false;

            if (m_connectionSelection == NULL)
                m_connectionSelection = new ConnectionSelection(m_data, m_selX1, m_selY1, m_selLayer1);
            m_connectionSelection->setSecond(m_data, m_selX2, m_selY2, m_selLayer2);

            if (!m_connectionSelection->isValid()) {
                if (m_connectionSelection != NULL) delete m_connectionSelection;
                m_connectionSelection = NULL;
            } else {
                const Room *r1 = m_connectionSelection->getFirst().room;
                ExitDirection dir1 = m_connectionSelection->getFirst().direction;
                const Room *r2 = m_connectionSelection->getSecond().room;
                ExitDirection dir2 = m_connectionSelection->getSecond().direction;

                if (r2) {
                    if ( !(r1->exit(dir1).containsOut(r2->getId())) || !(r2->exit(dir2).containsOut(r1->getId())) ) {
                        if ( dir2 != ED_UNKNOWN) {
                            if (m_connectionSelection != NULL) delete m_connectionSelection;
                            m_connectionSelection = NULL;
                        } else if ( dir2 == ED_UNKNOWN && (!(r1->exit(dir1).containsOut(r2->getId()))
                                                           || (r1->exit(dir1).containsIn(r2->getId()))) ) { //not oneway
                            if (m_connectionSelection != NULL) delete m_connectionSelection;
                            m_connectionSelection = NULL;
                        }
                    }
                }
            }
            emit newConnectionSelection(m_connectionSelection);
        }
        update();
        break;

    case CMM_CREATE_ROOMS:
        break;


    default:
        break;
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

    resizeGL(width(), height());
}

void MapCanvas::setHorizontalScroll(int x)
{
    m_scrollX = x;

    resizeGL(width(), height());
}

void MapCanvas::setVerticalScroll(int y)
{
    m_scrollY = y;

    resizeGL(width(), height());
}

void MapCanvas::zoomIn()
{
    m_scaleFactor += 0.05f;
    if (m_scaleFactor > 2.0f)
        m_scaleFactor -= 0.05f;

    resizeGL(width(), height());
}

void MapCanvas::zoomOut()
{
    m_scaleFactor -= 0.05f;
    if (m_scaleFactor < 0.04f)
        m_scaleFactor += 0.05f;

    resizeGL(width(), height());
}

void MapCanvas::initializeGL()
{
    initializeOpenGLFunctions();

    QByteArray version((const char *)glGetString(GL_VERSION));
    QByteArray renderer((const char *)glGetString(GL_RENDERER));
    QByteArray vendor((const char *)glGetString(GL_VENDOR));
    qInfo() << "OpenGL Version: " << version;
    qInfo() << "OpenGL Renderer: " << renderer;
    qInfo() << "OpenGL Vendor: " << vendor;
    emit log("MapCanvas", "OpenGL Version: " + version);
    emit log("MapCanvas", "OpenGL Renderer: " + renderer);
    emit log("MapCanvas", "OpenGL Vendor: " + vendor);

    QString contextStr = QString("%1.%2 ")
                         .arg(context()->format().majorVersion())
                         .arg(context()->format().minorVersion());
    contextStr.append((context()->isValid() ? "(valid)" : "(invalid)"));
    qInfo() << "Current OpenGL Context: " << contextStr;
    emit log("MapCanvas", "Current OpenGL Context: " + contextStr);

    m_logger = new QOpenGLDebugLogger(this);
    connect(m_logger, SIGNAL(messageLogged(QOpenGLDebugMessage)),
            this, SLOT(onMessageLogged(QOpenGLDebugMessage)),
            Qt::DirectConnection);
    if (m_logger->initialize()) {
        m_logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
        m_logger->disableMessages();
        m_logger->enableMessages(QOpenGLDebugMessage::AnySource,
                                 (QOpenGLDebugMessage::ErrorType | QOpenGLDebugMessage::UndefinedBehaviorType),
                                 QOpenGLDebugMessage::AnySeverity);
    }

    if (Config().m_antialiasingSamples > 0)
        glEnable(GL_MULTISAMPLE);

    // Load textures
    for (int i = 0; i < 16; i++) {
        m_terrainTextures[i] = new QOpenGLTexture(QImage(QString(":/pixmaps/terrain%1.png").arg(
                                                             i)).mirrored());
        m_roadTextures[i] = new QOpenGLTexture(QImage(QString(":/pixmaps/road%1.png").arg(i)).mirrored());
        m_loadTextures[i] = new QOpenGLTexture(QImage(QString(":/pixmaps/load%1.png").arg(i)).mirrored());
        m_trailTextures[i] = new QOpenGLTexture(QImage(QString(":/pixmaps/trail%1.png").arg(i)).mirrored());
        if (i < 15) {
            m_mobTextures[i] = new QOpenGLTexture(QImage(QString(":/pixmaps/mob%1.png").arg(i)).mirrored());
        }
    }
    m_updateTexture = new QOpenGLTexture(QImage(QString(":/pixmaps/update0.png")).mirrored());

    if (Config().m_trilinearFiltering) {
        for (int i = 0; i < 16; i++) {
            m_terrainTextures[i]->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
            m_roadTextures[i]->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
            m_loadTextures[i]->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
            m_trailTextures[i]->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
            if (i < 15) {
                m_mobTextures[i]->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
            }
        }
        m_updateTexture->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
    }
    m_view.setToIdentity();

    // <= OpenGL 3.0
    makeGlLists(); // TODO: Convert these GlLists into shaders
    glShadeModel(GL_FLAT);
    glPolygonStipple(halftone);
    //glPolygonStipple(quadtone);

    // >= OpenGL 3.0
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void MapCanvas::resizeGL(int width, int height)
{
    if (m_updateTexture == 0) {
        // resizeGL called but initializeGL was not called yet
        return;
    }

    float swp = m_scaleFactor * (1.0f - ((float)(width - BASESIZEX) / width));
    float shp = m_scaleFactor * (1.0f - ((float)(height - BASESIZEY) / height));

    makeCurrent();
    glViewport(0, 0, width, height);
    m_view.setToIdentity();
    m_projection.setToIdentity();

    // >= OpenGL 3.1
    m_projection.setToIdentity();
    m_projection.frustum(-0.5, +0.5, +0.5, -0.5, 5.0, 80.0);
    m_projection.scale(swp, shp, 1.0f);
    m_projection.translate(-SCROLLFACTOR() * (float)(m_scrollX),
                           -SCROLLFACTOR() * (float)(m_scrollY),
                           -60.0f);
    m_model.setToIdentity();

    // <= OpenGL 3.0
    glMatrixMode(GL_PROJECTION);
    loadMatrix(m_projection);
    glMatrixMode(GL_MODELVIEW);
    loadMatrix(m_model);

    QVector3D v1 = QVector3D(0, height, CAMERA_Z_DISTANCE).unproject(m_model, m_projection,
                                                                     this->rect());
    m_visibleX1 = v1.x();
    m_visibleY1 = v1.y();
    QVector3D v2 = QVector3D(width, 0, CAMERA_Z_DISTANCE).unproject(m_model, m_projection,
                                                                    this->rect());
    m_visibleX2 = v2.x();
    m_visibleY2 = v2.y();

    // Render
    update();
}

void MapCanvas::dataLoaded()
{
    m_currentLayer = m_data->getPosition().z;
    emit onCenter( m_data->getPosition().x, m_data->getPosition().y );
    makeCurrent();
    update();
}

void MapCanvas::moveMarker(const Coordinate &c)
{
    m_data->setPosition(c);
    m_currentLayer = c.z;
    makeCurrent();
    update();
    emit onCenter(c.x, c.y);
    //emit onEnsureVisible(c.x, c.y);
}

void MapCanvas::drawGroupCharacters()
{
    CGroup *group = m_groupManager->getGroup();
    if (!group || Config().m_groupManagerState == Mmapper2Group::Off || m_data->isEmpty())
        return;

    GroupSelection *selection = group->selectAll();
    foreach (CGroupChar *character, selection->values()) {
        uint id = character->getPosition();
        if (character->getName() != Config().m_groupManagerCharName) {
            const RoomSelection *selection = m_data->select();
            const Room *r = m_data->getRoom(id, selection);
            if (r) {
                drawCharacter(r->getPosition(), character->getColor());
            }
            m_data->unselect(id, selection);
        }
    }
    group->unselect(selection);
}

void MapCanvas::drawCharacter(const Coordinate &c, QColor color)
{
    qint32 x = c.x;
    qint32 y = c.y;
    qint32 z = c.z;
    qint32 layer = z - m_currentLayer;

    glPushMatrix();
    glColor4d(0.0f, 0.0f, 0.0f, 0.4f);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if ( ( ( x < m_visibleX1 - 1) || (x > m_visibleX2 + 1) ) ||
            ( ( y < m_visibleY1 - 1) || (y > m_visibleY2 + 1) ) ) {
        // Player is distant
        double cameraCenterX = (m_visibleX1 + m_visibleX2) / 2;
        double cameraCenterY = (m_visibleY1 + m_visibleY2) / 2;

        // Calculate degrees from camera center to character
        double adjacent = cameraCenterY - y;
        double opposite = cameraCenterX - x;
        double radians = atan2(adjacent, opposite);
        double degrees = radians * 180 / M_PI;

        // Identify character hint coordinates using an elipse to represent the screen
        double radiusX = ((m_visibleX2 - m_visibleX1) / 2) - 0.75f;
        double radiusY = ((m_visibleY2 - m_visibleY1) / 2) - 0.75f;
        double characterHintX = cameraCenterX + (cos(radians) * radiusX * -1);
        double characterHintY = cameraCenterY + (sin(radians) * radiusY * -1);

        // Draw character and rotate according to angle
        glTranslated(characterHintX, characterHintY, m_currentLayer + 0.1);
        glRotatef(degrees, 0, 0, 1);

        glCallList(m_character_hint_inner_gllist);
        glDisable(GL_BLEND);

        qglColor(color);
        glCallList(m_character_hint_gllist);

    } else if (z != m_currentLayer) {
        // Player is not on the same layer
        glTranslated(x, y - 0.5, m_currentLayer + 0.1);
        glRotatef(270, 0, 0, 1);

        glCallList(m_character_hint_inner_gllist);
        glDisable(GL_BLEND);

        qglColor(color);
        glCallList(m_character_hint_gllist);
    } else {
        // Player is on the same layer and visible
        glTranslated(x - 0.5, y - 0.5, ROOM_Z_DISTANCE * layer + 0.1);

        glCallList(m_room_selection_inner_gllist);
        glDisable(GL_BLEND);

        qglColor(color);
        glCallList(m_room_selection_gllist);
    }
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
}

void MapCanvas::paintGL()
{
    // Background Color
    qglClearColor(Config().m_backgroundColor);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_data->isEmpty()) {

        // Draw room tiles
        m_data->draw(
            Coordinate((int)(m_visibleX1), (int)(m_visibleY1), m_currentLayer - 10 ),
            Coordinate((int)(m_visibleX2 + 1), (int)(m_visibleY2 + 1), m_currentLayer + 10),
            *this);

        // Draw infomarks
        if (m_scaleFactor >= 0.25) {
            MarkerListIterator m(m_data->getMarkersList());
            while (m.hasNext())
                drawInfoMark(m.next());
        }
    } else {
        renderText((m_visibleX1 + m_visibleX2) / 2, (m_visibleY1 + m_visibleY2) / 2, "No map loaded");
    }

    GLdouble len = 0.2f;
    if (m_selectedArea || m_infoMarkSelection) {
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


    if (m_infoMarkSelection) {
        glColor4d(1.0f, 1.0f, 0.0f, 1.0f);
        glPointSize (devicePixelRatio() * 3.0);
        glLineWidth (devicePixelRatio() * 3.0);

        glBegin(GL_LINES);
        glVertex3d(m_selX1, m_selY1, 0.005);
        glVertex3d(m_selX2, m_selY2, 0.005);
        glEnd();
    }

    //paint selected connection
    if (m_connectionSelection && m_connectionSelection->isFirstValid()) {
        const Room *r = m_connectionSelection->getFirst().room;

        GLdouble x1p = r->getPosition().x;
        GLdouble y1p = r->getPosition().y;
        GLdouble x2p = m_selX2;
        GLdouble y2p = m_selY2;

        switch (m_connectionSelection->getFirst().direction) {
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
        default:
            ;
        }


        if (m_connectionSelection->isSecondValid()) {
            r = m_connectionSelection->getSecond().room;
            x2p = r->getPosition().x;
            y2p = r->getPosition().y;

            switch (m_connectionSelection->getSecond().direction) {
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
            default:
                ;
            }
        }

        qglColor(Qt::red);
        glPointSize (devicePixelRatio() * 10.0);
        glBegin(GL_POINTS);
        glVertex3d(x1p, y1p, 0.005);
        glVertex3d(x2p, y2p, 0.005);
        glEnd();
        glPointSize (devicePixelRatio() * 1.0);

        glBegin(GL_LINES);
        glVertex3d(x1p, y1p, 0.005);
        glVertex3d(x2p, y2p, 0.005);
        glEnd();
        glDisable(GL_BLEND);

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
    if (m_roomSelection) {
        const Room *room;

        QList<const Room *> values = m_roomSelection->values();
        for (int i = 0; i < values.size(); ++i) {
            room = values.at(i);

            qint32 x = room->getPosition().x;
            qint32 y = room->getPosition().y;
            qint32 z = room->getPosition().z;
            qint32 layer = z - m_currentLayer;

            glPushMatrix();
            //glTranslated(x-0.5, y-0.5, ROOM_Z_DISTANCE*z);
            glTranslated(x - 0.5, y - 0.5, ROOM_Z_DISTANCE * layer);

            glColor4d(0.0f, 0.0f, 0.0f, 0.4f);

            glEnable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glCallList(m_room_gllist);

            qglColor(Qt::red);
            glBegin(GL_LINE_STRIP);
            glVertex3d(0 + len, 0, 0.005);
            glVertex3d(0, 0, 0.005);
            glVertex3d(0, 0 + len, 0.005);
            glEnd();
            glBegin(GL_LINE_STRIP);
            glVertex3d(0 + len, 1, 0.005);
            glVertex3d(0, 1, 0.005);
            glVertex3d(0, 1 - len, 0.005);
            glEnd();
            glBegin(GL_LINE_STRIP);
            glVertex3d(1 - len, 1, 0.005);
            glVertex3d(1, 1, 0.005);
            glVertex3d(1, 1 - len, 0.005);
            glEnd();
            glBegin(GL_LINE_STRIP);
            glVertex3d(1 - len, 0, 0.005);
            glVertex3d(1, 0, 0.005);
            glVertex3d(1, 0 + len, 0.005);
            glEnd();

            if (m_roomSelectionMove) {
                if (m_roomSelectionMoveWrongPlace)
                    glColor4d(1.0f, 0.0f, 0.0f, 0.4f);
                else
                    glColor4d(1.0f, 1.0f, 1.0f, 0.4f);

                glTranslated(m_roomSelectionMoveX, m_roomSelectionMoveY, ROOM_Z_DISTANCE * layer);
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
    if (!m_data->isEmpty()) {
        // Use the player's selected color
        QColor color = Config().m_groupManagerColor;
        drawCharacter(m_data->getPosition(), color);
    }

    if (!m_prespammedPath->isEmpty()) {
        QList<Coordinate> path = m_data->getPath(m_prespammedPath->getQueue());
        Coordinate c1, c2;
        double dx, dy, dz;
        bool anypath = false;

        c1 = m_data->getPosition();
        QList<Coordinate>::const_iterator it = path.begin();
        while (it != path.end()) {
            if (!anypath) {
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


void MapCanvas::drawPathStart(const Coordinate &sc)
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
    glPointSize (devicePixelRatio() * 4.0);
    glLineWidth (devicePixelRatio() * 4.0);

    double srcZ = ROOM_Z_DISTANCE * layer1 + 0.3;

    glBegin(GL_LINE_STRIP);
    glVertex3d(0.0, 0.0, srcZ);

}

bool MapCanvas::drawPath(const Coordinate &sc, const Coordinate &dc, double &dx, double &dy,
                         double &dz)
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
    dz = ROOM_Z_DISTANCE * layer2 + 0.3;

    glVertex3d(dx, dy, dz);

    return true;
}

void MapCanvas::drawPathEnd(double dx, double dy, double dz)
{
    glEnd();

    glPointSize (devicePixelRatio() * 8.0);
    glBegin(GL_POINTS);
    glVertex3d(dx, dy, dz);
    glEnd();

    glLineWidth (devicePixelRatio() * 2.0);
    glPointSize (devicePixelRatio() * 2.0);
    glDisable(GL_BLEND);
    glPopMatrix();
}

void MapCanvas::drawInfoMark(InfoMark *marker)
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

    // Color depends of the class of the InfoMark
    QColor color = Qt::white; // Default color
    uint fontFormatFlag = FFF_NONE;
    switch (marker->getClass()) {
    case MC_GENERIC:
        color = Qt::white;
        break;
    case MC_HERB:
        color = QColor(0, 200, 0); // Dark green
        break;
    case MC_RIVER:
        color = QColor(76, 216, 255); // Cyan-like
        break;
    case MC_PLACE:
        color = Qt::white;
        break;
    case MC_MOB:
        color = QColor(177, 27, 27); // Dark red
        break;
    case MC_COMMENT:
        color = Qt::lightGray;
        break;
    case MC_ROAD:
        color = QColor(140, 83, 58); // Maroonish
        break;
    case MC_OBJECT:
        color = Qt::yellow;
        break;
    case MC_ACTION:
        color = Qt::white;
        fontFormatFlag = FFF_ITALICS;
        break;
    case MC_LOCALITY:
        color = Qt::white;
        fontFormatFlag = FFF_UNDERLINE;
    }

    if (marker->getType() == MT_TEXT) {
        width = m_glFontMetrics->width(marker->getText()) * 0.022f / m_scaleFactor;
        height = m_glFontMetrics->height() * 0.007f / m_scaleFactor;
        x2 = x1 + width;
        y2 = y1 + height;
    }

    //check if marker is visible
    if ( ( (x1 + 1 < m_visibleX1) || (x1 - 1 > m_visibleX2 + 1) ) &&
            ((x2 + 1 < m_visibleX1) || (x2 - 1 > m_visibleX2 + 1) ) )
        return;
    if ( ( (y1 + 1 < m_visibleY1) || (y1 - 1 > m_visibleY2 + 1) ) &&
            ((y2 + 1 < m_visibleY1) || (y2 - 1 > m_visibleY2 + 1) ) )
        return;

    glPushMatrix();
    glTranslated(x1/*-0.5*/, y1/*-0.5*/, 0.0);

    switch (marker->getType()) {
    case MT_TEXT:
        // Render background
        glColor4d(0, 0, 0, 0.3);
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_POLYGON);
        glVertex3d(0.0, 0.0, 1.0);
        glVertex3d(0.0, 0.25 + height, 1.0);
        glVertex3d(0.2 + width, 0.25 + height, 1.0);
        glVertex3d(0.2 + width, 0.0, 1.0);
        glEnd();
        glDisable(GL_BLEND);

        // Render text proper
        glTranslated(-x1 / 2, -y1 / 2, 0);
        renderText(x1 + 0.1, y1 + 0.25, marker->getText(), color, fontFormatFlag,
                   marker->getRotationAngle());
        glEnable(GL_DEPTH_TEST);
        break;
    case MT_LINE:
        glColor4d(color.redF(), color.greenF(), color.blueF(), 0.70);
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glPointSize (devicePixelRatio() * 2.0);
        glLineWidth (devicePixelRatio() * 2.0);
        glBegin(GL_LINES);
        glVertex3d(0.0, 0.0, 0.1);
        glVertex3d(dx, dy, 0.1);
        glEnd();
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        break;
    case MT_ARROW:
        glColor4d(color.redF(), color.greenF(), color.blueF(), 0.70);
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glPointSize (devicePixelRatio() * 2.0);
        glLineWidth (devicePixelRatio() * 2.0);
        glBegin(GL_LINE_STRIP);
        glVertex3d(0.0, 0.05, 1.0);
        glVertex3d(dx - 0.2, dy + 0.1, 1.0);
        glVertex3d(dx - 0.1, dy + 0.1, 1.0);
        glEnd();
        glBegin(GL_TRIANGLES);
        glVertex3d(dx - 0.1, dy + 0.1 - 0.07, 1.0);
        glVertex3d(dx - 0.1, dy + 0.1 + 0.07, 1.0);
        glVertex3d(dx + 0.1, dy + 0.1, 1.0);
        glEnd();
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        break;
    }

    glPopMatrix();
}

void MapCanvas::alphaOverlayTexture(QOpenGLTexture *texture)
{
    if (texture) texture->bind();
    glCallList(m_room_gllist);
}


void MapCanvas::drawRoomDoorName(const Room *sourceRoom, uint sourceDir, const Room *targetRoom,
                                 uint targetDir)
{
    assert(sourceRoom != NULL);
    assert(targetRoom != NULL);

    qint32 srcX = sourceRoom->getPosition().x;
    qint32 srcY = sourceRoom->getPosition().y;
    qint32 srcZ = sourceRoom->getPosition().z;
    qint32 tarX = targetRoom->getPosition().x;
    qint32 tarY = targetRoom->getPosition().y;
    qint32 tarZ = targetRoom->getPosition().z;
    qint32 dX = srcX - tarX;
    qint32 dY = srcY - tarY;

    if (srcZ != m_currentLayer && tarZ != m_currentLayer) return;

    // Look at door flags to code postfixes
    DoorFlags sourceDoorFlags = Mmapper2Exit::getDoorFlags(sourceRoom->exit(sourceDir));
    QString sourcePostFix;
    if (ISSET(sourceDoorFlags, DF_HIDDEN)) {
        sourcePostFix = "h";
    }
    if (ISSET(sourceDoorFlags, DF_NEEDKEY)) {
        sourcePostFix += "L";
    }
    if (ISSET(sourceDoorFlags, DF_NOPICK)) {
        sourcePostFix += "/NP";
    }
    if (ISSET(sourceDoorFlags, DF_DELAYED)) {
        sourcePostFix += "d";
    }
    if (sourcePostFix.length() > 0) {
        sourcePostFix = " [" + sourcePostFix + "]";
    }
    DoorFlags targetDoorFlags = Mmapper2Exit::getDoorFlags(targetRoom->exit(targetDir));
    QString targetPostFix;
    if (ISSET(targetDoorFlags, DF_HIDDEN)) {
        targetPostFix = "h";
    }
    if (ISSET(targetDoorFlags, DF_NEEDKEY)) {
        targetPostFix += "L";
    }
    if (ISSET(targetDoorFlags, DF_NOPICK)) {
        targetPostFix += "/NP";
    }
    if (ISSET(targetDoorFlags, DF_DELAYED)) {
        targetPostFix += "d";
    }
    if (targetPostFix.length() > 0) {
        targetPostFix = " [" + targetPostFix + "]";
    }

    QString name;
    bool together = false;

    if (ISSET(Mmapper2Exit::getFlags(targetRoom->exit(targetDir)), EF_DOOR)
            && // the other room has a door?
            !Mmapper2Exit::getDoorName(targetRoom->exit(targetDir)).isEmpty() && // has a door on both sides...
            abs(dX) <= 1 && abs(dY) <= 1) { // the door is close by!
        // skip the other direction since we're printing these together
        if (sourceDir % 2 == 1)
            return;

        together = true;

        // no need for duplicating names (its spammy)
        const QString &sourceName = Mmapper2Exit::getDoorName(sourceRoom->exit(sourceDir)) + sourcePostFix;
        const QString &targetName = Mmapper2Exit::getDoorName(targetRoom->exit(targetDir)) + targetPostFix;
        if (sourceName != targetName)
            name =  sourceName + "/" + targetName;
        else
            name = sourceName;
    } else
        name = Mmapper2Exit::getDoorName(sourceRoom->exit(sourceDir));

    qreal width = m_glFontMetrics->width(name) * 0.022f / m_scaleFactor;
    qreal height = m_glFontMetrics->height() * 0.007f / m_scaleFactor;

    qreal boxX = 0, boxY = 0;
    if (together) {
        boxX = srcX - (width / 2.0) - (dX * 0.5);
        boxY = srcY - 0.5 - (dY * 0.5);
    } else {
        boxX = srcX - (width / 2.0);
        switch (sourceDir) {
        case ED_NORTH:
            boxY = srcY - 0.65;
            break;
        case ED_SOUTH:
            boxY = srcY - 0.15;
            break;
        case ED_WEST:
            boxY = srcY - 0.5;
            break;
        case ED_EAST:
            boxY = srcY - 0.35;
            break;
        case ED_UP:
            boxY = srcY - 0.85;
            break;
        case ED_DOWN:
            boxY = srcY;
        };
    }

    qreal boxX2 = boxX + width;
    qreal boxY2 = boxY + height;

    //check if box is visible
    if ( ( (boxX + 1 < m_visibleX1) || (boxX - 1 > m_visibleX2 + 1) ) &&
            ((boxX2 + 1 < m_visibleX1) || (boxX2 - 1 > m_visibleX2 + 1) ) )
        return;
    if ( ( (boxY + 1 < m_visibleY1) || (boxY - 1 > m_visibleY2 + 1) ) &&
            ((boxY2 + 1 < m_visibleY1) || (boxY2 - 1 > m_visibleY2 + 1) ) )
        return;

    glPushMatrix();

    glTranslated(boxX/*-0.5*/, boxY/*-0.5*/, 0);

    // Render background
    glColor4d(0, 0, 0, 0.3);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_POLYGON);
    glVertex3d(0.0, 0.0, 1.0);
    glVertex3d(0.0, 0.25 + height, 1.0);
    glVertex3d(0.2 + width, 0.25 + height, 1.0);
    glVertex3d(0.2 + width, 0.0, 1.0);
    glEnd();
    glDisable(GL_BLEND);

    // text
    glTranslated(-boxX / 2, -boxY / 2, 0);
    renderText(boxX + 0.1, boxY + 0.25, name);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();

}

void MapCanvas::drawFlow(const Room *room, const std::vector<Room *> &rooms,
                         ExitDirection exitDirection)
{
    // Start drawing
    glPushMatrix();

    // Prepare pen
    qglColor(QColor(76, 216, 255));
    glEnable(GL_BLEND);
    glPointSize (devicePixelRatio() * 4.0);
    glLineWidth (devicePixelRatio() * 1.0);

    // Draw part in this room
    if (room->getPosition().z == m_currentLayer) {
        glCallList(m_flow_begin_gllist[exitDirection]);
    }

    // Draw part in adjacent room
    uint targetDir = Mmapper2Exit::opposite(exitDirection);
    uint targetId;
    const Room *targetRoom;
    const ExitsList &exitslist = room->getExitsList();
    const Exit &sourceExit = exitslist[exitDirection];

    //For each outgoing connections
    std::set<uint>::const_iterator itOut = sourceExit.outBegin();
    while (itOut != sourceExit.outEnd()) {
        targetId = *itOut;
        targetRoom = rooms[targetId];
        if (targetRoom->getPosition().z == m_currentLayer) {
            QMatrix4x4 model;
            model.setToIdentity();
            model.translate((float)targetRoom->getPosition().x, (float)targetRoom->getPosition().y, 0);
            loadMatrix(model);
            glCallList(m_flow_end_gllist[targetDir]);
        }
        ++itOut;
    }

    // Finish pen
    glLineWidth (devicePixelRatio() * 2.0);
    glPointSize (devicePixelRatio() * 2.0);
    glDisable(GL_BLEND);

    // Terminate drawing
    qglColor(Qt::black);
    glPopMatrix();
}

void MapCanvas::drawRoom(const Room *room, const std::vector<Room *> &rooms,
                         const std::vector<std::set<RoomRecipient *> > &locks)
{
    if (!room) return;

    qint32 x = room->getPosition().x;
    qint32 y = room->getPosition().y;
    qint32 z = room->getPosition().z;

    qint32 layer = z - m_currentLayer;

    glPushMatrix();
    glTranslated(x - 0.5, y - 0.5, ROOM_Z_DISTANCE * layer);

    // TODO: https://stackoverflow.com/questions/6017176/gllinestipple-deprecated-in-opengl-3-1
    glLineStipple(2, 43690);

    //terrain texture
    glColor4d(1.0f, 1.0f, 1.0f, 1.0f);

    // Enable blending for the textures
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (layer > 0) {
        if (Config().m_drawUpperLayersTextured) {
            glEnable(GL_POLYGON_STIPPLE);
        } else {
            glDisable(GL_POLYGON_STIPPLE);
            glColor4d(0.3f, 0.3f, 0.3f, 0.6f - 0.2f * layer);
            glEnable(GL_BLEND);
        }
    } else if (layer == 0) {
        glColor4d(1.0f, 1.0f, 1.0f, 0.9f);
        glEnable(GL_BLEND);
    }

    ExitFlags ef_north = Mmapper2Exit::getFlags(room->exit(ED_NORTH));
    ExitFlags ef_south = Mmapper2Exit::getFlags(room->exit(ED_SOUTH));
    ExitFlags ef_east  = Mmapper2Exit::getFlags(room->exit(ED_EAST));
    ExitFlags ef_west  = Mmapper2Exit::getFlags(room->exit(ED_WEST));
    ExitFlags ef_up    = Mmapper2Exit::getFlags(room->exit(ED_UP));
    ExitFlags ef_down  = Mmapper2Exit::getFlags(room->exit(ED_DOWN));

    quint32 roadindex = 0;
    if ( ISSET(ef_north, EF_ROAD)) SET(roadindex, bit1);
    if ( ISSET(ef_south, EF_ROAD)) SET(roadindex, bit2);
    if ( ISSET(ef_east,  EF_ROAD)) SET(roadindex, bit3);
    if ( ISSET(ef_west,  EF_ROAD)) SET(roadindex, bit4);

    QOpenGLTexture *texture;
    if (layer <= 0 || Config().m_drawUpperLayersTextured) {
        if ( (Mmapper2Room::getTerrainType(room)) == RTT_ROAD)
            texture = m_roadTextures[roadindex];
        else
            texture = m_terrainTextures[Mmapper2Room::getTerrainType(room)];

        glEnable(GL_BLEND);

        RoomMobFlags mf = Mmapper2Room::getMobFlags(room);
        RoomLoadFlags lf = Mmapper2Room::getLoadFlags(room);

        glEnable(GL_TEXTURE_2D);
        texture->bind();
        glCallList(m_room_gllist);

        // Make dark and troll safe rooms look dark
        if (Mmapper2Room::getSundeathType(room) == RST_NOSUNDEATH
                || Mmapper2Room::getLightType(room) == RLT_DARK) {
            GLdouble oldcolour[4];
            glGetDoublev(GL_CURRENT_COLOR, oldcolour);

            glTranslated(0, 0, 0.005);

            if (Mmapper2Room::getLightType(room) == RLT_DARK)
                glColor4d(0.1f, 0.0f, 0.0f, 0.4f);
            else
                glColor4d(0.1f, 0.0f, 0.0f, 0.2f);

            glCallList(m_room_gllist);

            glColor4d(oldcolour[0], oldcolour[1], oldcolour[2], oldcolour[3]);
        }

        // Only display at a certain scale
        if (m_scaleFactor >= 0.15) {

            // Draw a little dark red cross on noride rooms
            if (Mmapper2Room::getRidableType(room) == RRT_NOTRIDABLE) {
                GLdouble oldcolour[4];
                glGetDoublev(GL_CURRENT_COLOR, oldcolour);
                glDisable(GL_TEXTURE_2D);

                glColor4d(0.5f, 0.0f, 0.0f, 0.9f);
                glLineWidth(devicePixelRatio() * 3.0);
                glBegin(GL_LINES);
                glVertex3d(0.6, 0.2, 0.005);
                glVertex3d(0.8, 0.4, 0.005);
                glVertex3d(0.8, 0.2, 0.005);
                glVertex3d(0.6, 0.4, 0.005);
                glEnd();

                glColor4d(oldcolour[0], oldcolour[1], oldcolour[2], oldcolour[3]);
                glEnable(GL_TEXTURE_2D);
            }

            // Trail Support
            glTranslated(0, 0, 0.005);
            if (roadindex > 0 && (Mmapper2Room::getTerrainType(room)) != RTT_ROAD) {
                alphaOverlayTexture(m_trailTextures[roadindex]);
                glTranslated(0, 0, 0.005);
            }
            //RMF_RENT, RMF_SHOP, RMF_WEAPONSHOP, RMF_ARMOURSHOP, RMF_FOODSHOP, RMF_PETSHOP,
            //RMF_GUILD, RMF_SCOUTGUILD, RMF_MAGEGUILD, RMF_CLERICGUILD, RMF_WARRIORGUILD,
            //RMF_RANGERGUILD, RMF_SMOB, RMF_QUEST, RMF_ANY
            if (ISSET(mf, RMF_RENT))
                alphaOverlayTexture(m_mobTextures[0]);
            if (ISSET(mf, RMF_SHOP))
                alphaOverlayTexture(m_mobTextures[1]);
            if (ISSET(mf, RMF_WEAPONSHOP))
                alphaOverlayTexture(m_mobTextures[2]);
            if (ISSET(mf, RMF_ARMOURSHOP))
                alphaOverlayTexture(m_mobTextures[3]);
            if (ISSET(mf, RMF_FOODSHOP))
                alphaOverlayTexture(m_mobTextures[4]);
            if (ISSET(mf, RMF_PETSHOP))
                alphaOverlayTexture(m_mobTextures[5]);
            if (ISSET(mf, RMF_GUILD))
                alphaOverlayTexture(m_mobTextures[6]);
            if (ISSET(mf, RMF_SCOUTGUILD))
                alphaOverlayTexture(m_mobTextures[7]);
            if (ISSET(mf, RMF_MAGEGUILD))
                alphaOverlayTexture(m_mobTextures[8]);
            if (ISSET(mf, RMF_CLERICGUILD))
                alphaOverlayTexture(m_mobTextures[9]);
            if (ISSET(mf, RMF_WARRIORGUILD))
                alphaOverlayTexture(m_mobTextures[10]);
            if (ISSET(mf, RMF_RANGERGUILD))
                alphaOverlayTexture(m_mobTextures[11]);
            if (ISSET(mf, RMF_SMOB))
                alphaOverlayTexture(m_mobTextures[12]);
            if (ISSET(mf, RMF_QUEST))
                alphaOverlayTexture(m_mobTextures[13]);
            if (ISSET(mf, RMF_ANY))
                alphaOverlayTexture(m_mobTextures[14]);

            //RLF_TREASURE, RLF_ARMOUR, RLF_WEAPON, RLF_WATER, RLF_FOOD, RLF_HERB
            //RLF_KEY, RLF_MULE, RLF_HORSE, RLF_PACKHORSE, RLF_TRAINEDHORSE
            //RLF_ROHIRRIM, RLF_WARG, RLF_BOAT
            glTranslated(0, 0, 0.005);
            if (ISSET(lf, RLF_TREASURE))
                alphaOverlayTexture(m_loadTextures[0]);
            if (ISSET(lf, RLF_ARMOUR))
                alphaOverlayTexture(m_loadTextures[1]);
            if (ISSET(lf, RLF_WEAPON))
                alphaOverlayTexture(m_loadTextures[2]);
            if (ISSET(lf, RLF_WATER))
                alphaOverlayTexture(m_loadTextures[3]);
            if (ISSET(lf, RLF_FOOD))
                alphaOverlayTexture(m_loadTextures[4]);
            if (ISSET(lf, RLF_HERB))
                alphaOverlayTexture(m_loadTextures[5]);
            if (ISSET(lf, RLF_KEY))
                alphaOverlayTexture(m_loadTextures[6]);
            if (ISSET(lf, RLF_MULE))
                alphaOverlayTexture(m_loadTextures[7]);
            if (ISSET(lf, RLF_HORSE))
                alphaOverlayTexture(m_loadTextures[8]);
            if (ISSET(lf, RLF_PACKHORSE))
                alphaOverlayTexture(m_loadTextures[9]);
            if (ISSET(lf, RLF_TRAINEDHORSE))
                alphaOverlayTexture(m_loadTextures[10]);
            if (ISSET(lf, RLF_ROHIRRIM))
                alphaOverlayTexture(m_loadTextures[11]);
            if (ISSET(lf, RLF_WARG))
                alphaOverlayTexture(m_loadTextures[12]);
            if (ISSET(lf, RLF_BOAT))
                alphaOverlayTexture(m_loadTextures[13]);

            glTranslated(0, 0, 0.005);
            if (ISSET(lf, RLF_ATTENTION))
                alphaOverlayTexture(m_loadTextures[14]);
            if (ISSET(lf, RLF_TOWER))
                alphaOverlayTexture(m_loadTextures[15]);

            //UPDATED?
            glTranslated(0, 0, 0.005);
            if (Config().m_showUpdated && !room->isUpToDate())
                alphaOverlayTexture(m_updateTexture);
            glDisable(GL_BLEND);
            glDisable(GL_TEXTURE_2D);
        }
    } else {
        glEnable(GL_BLEND);
        glCallList(m_room_gllist);
        glDisable(GL_BLEND);
    }

    //walls
    glTranslated(0, 0, 0.005);

    if (layer > 0) {
        glDisable (GL_POLYGON_STIPPLE);
        glColor4d(0.3, 0.3, 0.3, 0.6);
        glEnable(GL_BLEND);
    } else {
        qglColor(Qt::black);
    }

    glPointSize (devicePixelRatio() * 3.0);
    glLineWidth (devicePixelRatio() * 2.4);

    //exit n
    if (ISSET(ef_north, EF_EXIT) &&
            Config().m_drawNotMappedExits &&
            room->exit(ED_NORTH).outBegin() == room->exit(ED_NORTH).outEnd()) { // zero outgoing connections
        glEnable(GL_LINE_STIPPLE);
        glColor4d(1.0, 0.5, 0.0, 0.0);
        glCallList(m_wall_north_gllist);
        qglColor(Qt::black);
        glDisable(GL_LINE_STIPPLE);
    } else {
        if ( ISSET(ef_north, EF_NO_FLEE)) {
            glEnable(GL_LINE_STIPPLE);
            qglColor(m_noFleeColor);
            glCallList(m_wall_north_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if ( ISSET(ef_north, EF_RANDOM)) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(1.0, 0.0, 0.0, 0.0);
            glCallList(m_wall_north_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if ( ISSET(ef_north, EF_SPECIAL)) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(0.8, 0.1, 0.8, 0.0);
            glCallList(m_wall_north_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if ( ISSET(ef_north, EF_CLIMB)) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(0.7, 0.7, 0.7, 0.0);
            glCallList(m_wall_north_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if (Config().m_drawNoMatchExits && ISSET(ef_north, EF_NO_MATCH)) {
            glEnable(GL_LINE_STIPPLE);
            qglColor(Qt::blue);
            glCallList(m_wall_north_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        }
        if ( ISSET(ef_north, EF_FLOW)) {
            drawFlow(room, rooms, ED_NORTH);
        }
    }
    //wall n
    if (ISNOTSET(ef_north, EF_EXIT) || ISSET(ef_north, EF_DOOR)) {
        if (ISNOTSET(ef_north, EF_DOOR)
                && room->exit(ED_NORTH).outBegin() != room->exit(ED_NORTH).outEnd()) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(0.2, 0.0, 0.0, 0.0);
            glCallList(m_wall_north_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else
            glCallList(m_wall_north_gllist);
    }
    //door n
    if (ISSET(ef_north, EF_DOOR))
        glCallList(m_door_north_gllist);
    //exit s
    if (ISSET(ef_south, EF_EXIT) &&
            Config().m_drawNotMappedExits &&
            room->exit(ED_SOUTH).outBegin() == room->exit(ED_SOUTH).outEnd()) { // zero outgoing connections
        glEnable(GL_LINE_STIPPLE);
        glColor4d(1.0, 0.5, 0.0, 0.0);
        glCallList(m_wall_south_gllist);
        qglColor(Qt::black);
        glDisable(GL_LINE_STIPPLE);
    } else {
        if ( ISSET(ef_south, EF_NO_FLEE)) {
            glEnable(GL_LINE_STIPPLE);
            qglColor(m_noFleeColor);
            glCallList(m_wall_south_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if ( ISSET(ef_south, EF_RANDOM)) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(1.0, 0.0, 0.0, 0.0);
            glCallList(m_wall_south_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if ( ISSET(ef_south, EF_SPECIAL)) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(0.8, 0.1, 0.8, 0.0);
            glCallList(m_wall_south_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if ( ISSET(ef_south, EF_CLIMB)) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(0.7, 0.7, 0.7, 0.0);
            glCallList(m_wall_south_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if (Config().m_drawNoMatchExits && ISSET(ef_south, EF_NO_MATCH)) {
            glEnable(GL_LINE_STIPPLE);
            qglColor(Qt::blue);
            glCallList(m_wall_south_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        }
        if ( ISSET(ef_south, EF_FLOW)) {
            drawFlow(room, rooms, ED_SOUTH);
        }
    }
    //wall s
    if (ISNOTSET(ef_south, EF_EXIT) || ISSET(ef_south, EF_DOOR)) {
        if (ISNOTSET(ef_south, EF_DOOR)
                && room->exit(ED_SOUTH).outBegin() != room->exit(ED_SOUTH).outEnd()) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(0.2, 0.0, 0.0, 0.0);
            glCallList(m_wall_south_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else
            glCallList(m_wall_south_gllist);
    }
    //door s
    if (ISSET(ef_south, EF_DOOR))
        glCallList(m_door_south_gllist);

    //exit e
    if (ISSET(ef_east, EF_EXIT) &&
            Config().m_drawNotMappedExits &&
            room->exit(ED_EAST).outBegin() == room->exit(ED_EAST).outEnd()) { // zero outgoing connections
        glEnable(GL_LINE_STIPPLE);
        glColor4d(1.0, 0.5, 0.0, 0.0);
        glCallList(m_wall_east_gllist);
        qglColor(Qt::black);
        glDisable(GL_LINE_STIPPLE);
    } else {
        if ( ISSET(ef_east, EF_NO_FLEE)) {
            glEnable(GL_LINE_STIPPLE);
            qglColor(m_noFleeColor);
            glCallList(m_wall_east_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if ( ISSET(ef_east, EF_RANDOM)) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(1.0, 0.0, 0.0, 0.0);
            glCallList(m_wall_east_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if ( ISSET(ef_east, EF_SPECIAL)) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(0.8, 0.1, 0.8, 0.0);
            glCallList(m_wall_east_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if ( ISSET(ef_east, EF_CLIMB)) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(0.7, 0.7, 0.7, 0.0);
            glCallList(m_wall_east_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if (Config().m_drawNoMatchExits && ISSET(ef_east, EF_NO_MATCH)) {
            glEnable(GL_LINE_STIPPLE);
            qglColor(Qt::blue);
            glCallList(m_wall_east_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        }
        if ( ISSET(ef_east, EF_FLOW)) {
            drawFlow(room, rooms, ED_EAST);
        }
    }
    //wall e
    if (ISNOTSET(ef_east, EF_EXIT) || ISSET(ef_east, EF_DOOR)) {
        if (ISNOTSET(ef_east, EF_DOOR) && room->exit(ED_EAST).outBegin() != room->exit(ED_EAST).outEnd()) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(0.2, 0.0, 0.0, 0.0);
            glCallList(m_wall_east_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else
            glCallList(m_wall_east_gllist);
    }
    //door e
    if (ISSET(ef_east, EF_DOOR))
        glCallList(m_door_east_gllist);

    //exit w
    if (ISSET(ef_west, EF_EXIT) &&
            Config().m_drawNotMappedExits &&
            room->exit(ED_WEST).outBegin() == room->exit(ED_WEST).outEnd()) { // zero outgoing connections
        glEnable(GL_LINE_STIPPLE);
        glColor4d(1.0, 0.5, 0.0, 0.0);
        glCallList(m_wall_west_gllist);
        qglColor(Qt::black);
        glDisable(GL_LINE_STIPPLE);
    } else {
        if ( ISSET(ef_west, EF_NO_FLEE)) {
            glEnable(GL_LINE_STIPPLE);
            qglColor(m_noFleeColor);
            glCallList(m_wall_west_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if ( ISSET(ef_west, EF_RANDOM)) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(1.0, 0.0, 0.0, 0.0);
            glCallList(m_wall_west_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if ( ISSET(ef_west, EF_SPECIAL)) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(0.8, 0.1, 0.8, 0.0);
            glCallList(m_wall_west_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if ( ISSET(ef_west, EF_CLIMB)) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(0.7, 0.7, 0.7, 0.0);
            glCallList(m_wall_west_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else if (Config().m_drawNoMatchExits && ISSET(ef_west, EF_NO_MATCH)) {
            glEnable(GL_LINE_STIPPLE);
            qglColor(Qt::blue);
            glCallList(m_wall_west_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        }
        if ( ISSET(ef_west, EF_FLOW)) {
            drawFlow(room, rooms, ED_WEST);
        }
    }
    //wall w
    if (ISNOTSET(ef_west, EF_EXIT) || ISSET(ef_west, EF_DOOR)) {
        if (ISNOTSET(ef_west, EF_DOOR) && room->exit(ED_WEST).outBegin() != room->exit(ED_WEST).outEnd()) {
            glEnable(GL_LINE_STIPPLE);
            glColor4d(0.2, 0.0, 0.0, 0.0);
            glCallList(m_wall_west_gllist);
            qglColor(Qt::black);
            glDisable(GL_LINE_STIPPLE);
        } else
            glCallList(m_wall_west_gllist);
    }

    //door w
    if (ISSET(ef_west, EF_DOOR))
        glCallList(m_door_west_gllist);

    glPointSize (devicePixelRatio() * 3.0);
    glLineWidth (devicePixelRatio() * 2.0);

    //exit u
    if (ISSET(ef_up, EF_EXIT)) {
        if (Config().m_drawNotMappedExits
                && room->exit(ED_UP).outBegin() == room->exit(ED_UP).outEnd()) { // zero outgoing connections
            glEnable(GL_LINE_STIPPLE);
            glColor4d(1.0, 0.5, 0.0, 0.0);

            glCallList(m_exit_up_transparent_gllist);
            glColor4d(0.0, 0.0, 0.0, 0.0);
            glDisable(GL_LINE_STIPPLE);
        } else {
            if ( ISSET(ef_up, EF_NO_FLEE)) {
                glEnable(GL_LINE_STIPPLE);
                qglColor(m_noFleeColor);
                glCallList(m_exit_up_transparent_gllist);
                glColor4d(0.0, 0.0, 0.0, 0.0);
                glDisable(GL_LINE_STIPPLE);
            } else if ( ISSET(ef_up, EF_RANDOM)) {
                glEnable(GL_LINE_STIPPLE);
                glColor4d(1.0, 0.0, 0.0, 0.0);
                glCallList(m_exit_up_transparent_gllist);
                glColor4d(0.0, 0.0, 0.0, 0.0);
                glDisable(GL_LINE_STIPPLE);
            } else if ( ISSET(ef_up, EF_SPECIAL)) {
                glEnable(GL_LINE_STIPPLE);
                glColor4d(0.8, 0.1, 0.8, 0.0);
                glCallList(m_exit_up_transparent_gllist);
                glColor4d(0.0, 0.0, 0.0, 0.0);
                glDisable(GL_LINE_STIPPLE);
            } else if ( ISSET(ef_up, EF_CLIMB)) {
                glEnable(GL_LINE_STIPPLE);
                glColor4d(0.5, 0.5, 0.5, 0.0);
                glCallList(m_exit_up_transparent_gllist);
                glColor4d(0.0, 0.0, 0.0, 0.0);
                glDisable(GL_LINE_STIPPLE);
            } else if (Config().m_drawNoMatchExits && ISSET(ef_up, EF_NO_MATCH)) {
                glEnable(GL_LINE_STIPPLE);
                qglColor(Qt::blue);
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

            if ( ISSET(ef_up, EF_FLOW)) {
                drawFlow(room, rooms, ED_UP);
            }
        }
    }

    //exit d
    if (ISSET(ef_down, EF_EXIT)) {
        if (Config().m_drawNotMappedExits
                && room->exit(ED_DOWN).outBegin() == room->exit(ED_DOWN).outEnd()) { // zero outgoing connections
            glEnable(GL_LINE_STIPPLE);
            glColor4d(1.0, 0.5, 0.0, 0.0);

            glCallList(m_exit_down_transparent_gllist);
            glColor4d(0.0, 0.0, 0.0, 0.0);
            glDisable(GL_LINE_STIPPLE);
        } else {
            if (ISSET(ef_down, EF_NO_FLEE)) {
                glEnable(GL_LINE_STIPPLE);
                qglColor(m_noFleeColor);
                glCallList(m_exit_down_transparent_gllist);
                glColor4d(0.0, 0.0, 0.0, 0.0);
                glDisable(GL_LINE_STIPPLE);
            } else if ( ISSET(ef_down, EF_RANDOM)) {
                glEnable(GL_LINE_STIPPLE);
                glColor4d(1.0, 0.0, 0.0, 0.0);
                glCallList(m_exit_down_transparent_gllist);
                glColor4d(0.0, 0.0, 0.0, 0.0);
                glDisable(GL_LINE_STIPPLE);
            } else if ( ISSET(ef_down, EF_SPECIAL)) {
                glEnable(GL_LINE_STIPPLE);
                glColor4d(0.8, 0.1, 0.8, 0.0);
                glCallList(m_exit_down_transparent_gllist);
                glColor4d(0.0, 0.0, 0.0, 0.0);
                glDisable(GL_LINE_STIPPLE);
            } else if ( ISSET(ef_down, EF_CLIMB)) {
                glEnable(GL_LINE_STIPPLE);
                glColor4d(0.5, 0.5, 0.5, 0.0);
                glCallList(m_exit_down_transparent_gllist);
                glColor4d(0.0, 0.0, 0.0, 0.0);
                glDisable(GL_LINE_STIPPLE);
            } else if (Config().m_drawNoMatchExits && ISSET(ef_down, EF_NO_MATCH)) {
                glEnable(GL_LINE_STIPPLE);
                qglColor(Qt::blue);
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

            if ( ISSET(ef_down, EF_FLOW)) {
                drawFlow(room, rooms, ED_DOWN);
            }
        }
    }

    if (layer > 0) {
        glDisable(GL_BLEND);
    }

    glTranslated(0, 0, 0.0100);
    if (layer < 0) {
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4d(0.0f, 0.0f, 0.0f, 0.5f - 0.03f * layer);
        glCallList(m_room_gllist);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    } else if (layer > 0) {
        glDisable(GL_LINE_STIPPLE);

        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4d(1.0f, 1.0f, 1.0f, 0.1f);
        glCallList(m_room_gllist);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

    if (!locks[room->getId()].empty()) { //---> room is locked
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4d(0.6f, 0.0f, 0.0f, 0.2f);
        glCallList(m_room_gllist);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

    glPopMatrix();

    if (m_scaleFactor >= 0.15) {
        //draw connections and doors
        uint sourceId = room->getId();
        uint targetId;
        const Room *targetRoom;
        const ExitsList &exitslist = room->getExitsList();
        bool oneway;
        int rx;
        int ry;

        for (int i = 0; i < 7; i++) {
            uint targetDir = Mmapper2Exit::opposite(i);
            const Exit &sourceExit = exitslist[i];
            //outgoing connections
            std::set<uint>::const_iterator itOut = sourceExit.outBegin();
            while (itOut != sourceExit.outEnd()) {
                targetId = *itOut;
                targetRoom = rooms[targetId];
                rx = targetRoom->getPosition().x;
                ry = targetRoom->getPosition().y;
                if ( (targetId >= sourceId) || // draw exits if targetId >= sourceId ...
                        ( ( ( rx < m_visibleX1 - 1) || (rx > m_visibleX2 + 1) ) || // or if target room is not visible
                          ( ( ry < m_visibleY1 - 1) || (ry > m_visibleY2 + 1) ) ) ) {
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
                        drawConnection( room, targetRoom, (ExitDirection)i, (ExitDirection)targetDir, true,
                                        ISSET(Mmapper2Exit::getFlags(room->exit(i)), EF_EXIT) );
                    else
                        drawConnection( room, targetRoom, (ExitDirection)i, (ExitDirection)targetDir, false,
                                        ISSET(Mmapper2Exit::getFlags(room->exit(i)), EF_EXIT)
                                        && ISSET(Mmapper2Exit::getFlags(targetRoom->exit(targetDir)), EF_EXIT) );
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
                        drawConnection( room, targetRoom, (ExitDirection)i, (ExitDirection)(Mmapper2Exit::opposite(i)),
                                        true, ISSET(Mmapper2Exit::getFlags(sourceExit), EF_EXIT) );
                }

                //incoming connections (only for oneway connections from rooms, that are not visible)
                std::set<uint>::const_iterator itIn = exitslist[i].inBegin();
                while (itIn != exitslist[i].inEnd()) {
                    targetId = *itIn;
                    targetRoom = rooms[targetId];
                    rx = targetRoom->getPosition().x;
                    ry = targetRoom->getPosition().y;

                    if ( ( ( rx < m_visibleX1 - 1) || (rx > m_visibleX2 + 1) ) ||
                            ( ( ry < m_visibleY1 - 1) || (ry > m_visibleY2 + 1) ) ) {
                        if ( !targetRoom->exit(Mmapper2Exit::opposite(i)).containsIn(sourceId) ) {
                            drawConnection( targetRoom, room, (ExitDirection)(Mmapper2Exit::opposite(i)), (ExitDirection)i,
                                            true, ISSET(Mmapper2Exit::getFlags(targetRoom->exit(Mmapper2Exit::opposite(i))), EF_EXIT) );
                        }
                    }
                    ++itIn;
                }

                // draw door names
                if (Config().m_drawDoorNames && m_scaleFactor >= 0.40 &&
                        ISSET(Mmapper2Exit::getFlags(room->exit((ExitDirection)i)), EF_DOOR) &&
                        !Mmapper2Exit::getDoorName(room->exit((ExitDirection)i)).isEmpty()) {
                    if ( targetRoom->exit(Mmapper2Exit::opposite(i)).containsOut(sourceId))
                        targetDir = Mmapper2Exit::opposite(i);
                    else {
                        for (int j = 0; j < 7; ++j) {
                            if (targetRoom->exit(j).containsOut(sourceId)) {
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

void MapCanvas::drawConnection( const Room *leftRoom, const Room *rightRoom,
                                ExitDirection connectionStartDirection, ExitDirection connectionEndDirection, bool oneway,
                                bool inExitFlags )
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

    if ((dX == 0) && (dY == -1) && (dZ == 0)  ) {
        if ( (connectionStartDirection == ED_NORTH) && (connectionEndDirection == ED_SOUTH) && !oneway )
            return;
        neighbours = true;
    }
    if ((dX == 0) && (dY == +1) && (dZ == 0) ) {
        if ( (connectionStartDirection == ED_SOUTH) && (connectionEndDirection == ED_NORTH) && !oneway )
            return;
        neighbours = true;
    }
    if ((dX == +1) && (dY == 0) && (dZ == 0) ) {
        if ( (connectionStartDirection == ED_EAST) && (connectionEndDirection == ED_WEST) && !oneway )
            return;
        neighbours = true;
    }
    if ((dX == -1) && (dY == 0) && (dZ == 0) ) {
        if ( (connectionStartDirection == ED_WEST) && (connectionEndDirection == ED_EAST) && !oneway )
            return;
        neighbours = true;
    }

    glPushMatrix();
    glTranslated(leftX - 0.5, leftY - 0.5, 0.0);

    if (inExitFlags)
        glColor4d(1.0f, 1.0f, 1.0f, 0.70);
    else
        glColor4d(1.0f, 0.0f, 0.0f, 0.70);

    glEnable(GL_BLEND);
    glPointSize (devicePixelRatio() * 2.0);
    glLineWidth (devicePixelRatio() * 2.0);

    double srcZ = ROOM_Z_DISTANCE * leftLayer + 0.3;

    bool glBeginOpen = false;
    switch (connectionStartDirection) {
    case ED_NORTH:
        if ( !oneway ) {
            glBegin(GL_TRIANGLES);
            glVertex3d(0.68, +0.1, srcZ);
            glVertex3d(0.82, +0.1, srcZ);
            glVertex3d(0.75, +0.3, srcZ);
            glEnd();
        }
        glBegin(GL_LINE_STRIP);
        glVertex3d(0.75, 0.1, srcZ);
        glVertex3d(0.75, -0.1, srcZ);
        glBeginOpen = true;
        break;
    case ED_SOUTH:
        if ( !oneway ) {
            glBegin(GL_TRIANGLES);
            glVertex3d(0.18, 0.9, srcZ);
            glVertex3d(0.32, 0.9, srcZ);
            glVertex3d(0.25, 0.7, srcZ);
            glEnd();
        }
        glBegin(GL_LINE_STRIP);
        glVertex3d(0.25, 0.9, srcZ);
        glVertex3d(0.25, 1.1, srcZ);
        glBeginOpen = true;
        break;
    case ED_EAST:
        if ( !oneway ) {
            glBegin(GL_TRIANGLES);
            glVertex3d(0.9, 0.18, srcZ);
            glVertex3d(0.9, 0.32, srcZ);
            glVertex3d(0.7, 0.25, srcZ);
            glEnd();
        }
        glBegin(GL_LINE_STRIP);
        glVertex3d(0.9, 0.25, srcZ);
        glVertex3d(1.1, 0.25, srcZ);
        glBeginOpen = true;
        break;
    case ED_WEST:
        if ( !oneway ) {
            glBegin(GL_TRIANGLES);
            glVertex3d(0.1, 0.68, srcZ);
            glVertex3d(0.1, 0.82, srcZ);
            glVertex3d(0.3, 0.75, srcZ);
            glEnd();
        }
        glBegin(GL_LINE_STRIP);
        glVertex3d(0.1, 0.75, srcZ);
        glVertex3d(-0.1, 0.75, srcZ);
        glBeginOpen = true;
        break;
    case ED_UP:
        if (!neighbours) {
            glBegin(GL_LINE_STRIP);
            glVertex3d(0.63, 0.25, srcZ);
            glVertex3d(0.55, 0.25, srcZ);
        } else {
            glBegin(GL_LINE_STRIP);
            glVertex3d(0.75, 0.25, srcZ);
        }
        glBeginOpen = true;
        break;
    case ED_DOWN:
        if (!neighbours) {
            glBegin(GL_LINE_STRIP);
            glVertex3d(0.37, 0.75, srcZ);
            glVertex3d(0.45, 0.75, srcZ);
        } else {
            glBegin(GL_LINE_STRIP);
            glVertex3d(0.25, 0.75, srcZ);
        }
        glBeginOpen = true;
        break;
    case ED_UNKNOWN:
        break;
    case ED_NONE:
        break;
    }

    double dstZ = ROOM_Z_DISTANCE * rightLayer + 0.3;

    switch (connectionEndDirection) {
    case ED_NORTH:
        if (!oneway) {
            if (glBeginOpen) {
                glVertex3d(dX + 0.75, dY - 0.1, dstZ);
                glVertex3d(dX + 0.75, dY + 0.1, dstZ);
                glEnd();
            }
            glBegin(GL_TRIANGLES);
            glVertex3d(dX + 0.68, dY + 0.1, dstZ);
            glVertex3d(dX + 0.82, dY + 0.1, dstZ);
            glVertex3d(dX + 0.75, dY + 0.3, dstZ);
            glEnd();
        } else {
            if (glBeginOpen) {
                glVertex3d(dX + 0.25, dY - 0.1, dstZ);
                glVertex3d(dX + 0.25, dY + 0.1, dstZ);
                glEnd();
            }
            glBegin(GL_TRIANGLES);
            glVertex3d(dX + 0.18, dY + 0.1, dstZ);
            glVertex3d(dX + 0.32, dY + 0.1, dstZ);
            glVertex3d(dX + 0.25, dY + 0.3, dstZ);
            glEnd();
        }
        break;
    case ED_SOUTH:
        if (!oneway) {
            if (glBeginOpen) {
                glVertex3d(dX + 0.25, dY + 1.1, dstZ);
                glVertex3d(dX + 0.25, dY + 0.9, dstZ);
                glEnd();
            }
            glBegin(GL_TRIANGLES);
            glVertex3d(dX + 0.18, dY + 0.9, dstZ);
            glVertex3d(dX + 0.32, dY + 0.9, dstZ);
            glVertex3d(dX + 0.25, dY + 0.7, dstZ);
            glEnd();
        } else {
            if (glBeginOpen) {
                glVertex3d(dX + 0.75, dY + 1.1, dstZ);
                glVertex3d(dX + 0.75, dY + 0.9, dstZ);
                glEnd();
            }
            glBegin(GL_TRIANGLES);
            glVertex3d(dX + 0.68, dY + 0.9, dstZ);
            glVertex3d(dX + 0.82, dY + 0.9, dstZ);
            glVertex3d(dX + 0.75, dY + 0.7, dstZ);
            glEnd();
        }
        break;
    case ED_EAST:
        if (!oneway) {
            if (glBeginOpen) {
                glVertex3d(dX + 1.1, dY + 0.25, dstZ);
                glVertex3d(dX + 0.9, dY + 0.25, dstZ);
                glEnd();
            }
            glBegin(GL_TRIANGLES);
            glVertex3d(dX + 0.9, dY + 0.18, dstZ);
            glVertex3d(dX + 0.9, dY + 0.32, dstZ);
            glVertex3d(dX + 0.7, dY + 0.25, dstZ);
            glEnd();
        } else {
            if (glBeginOpen) {
                glVertex3d(dX + 1.1, dY + 0.75, dstZ);
                glVertex3d(dX + 0.9, dY + 0.75, dstZ);
                glEnd();
            }
            glBegin(GL_TRIANGLES);
            glVertex3d(dX + 0.9, dY + 0.68, dstZ);
            glVertex3d(dX + 0.9, dY + 0.82, dstZ);
            glVertex3d(dX + 0.7, dY + 0.75, dstZ);
            glEnd();
        }
        break;
    case ED_WEST:
        if (!oneway) {
            if (glBeginOpen) {
                glVertex3d(dX - 0.1, dY + 0.75, dstZ);
                glVertex3d(dX + 0.1, dY + 0.75, dstZ);
                glEnd();
            }
            glBegin(GL_TRIANGLES);
            glVertex3d(dX + 0.1, dY + 0.68, dstZ);
            glVertex3d(dX + 0.1, dY + 0.82, dstZ);
            glVertex3d(dX + 0.3, dY + 0.75, dstZ);
            glEnd();
        } else {
            if (glBeginOpen) {
                glVertex3d(dX - 0.1, dY + 0.25, dstZ);
                glVertex3d(dX + 0.1, dY + 0.25, dstZ);
                glEnd();
            }
            glBegin(GL_TRIANGLES);
            glVertex3d(dX + 0.1, dY + 0.18, dstZ);
            glVertex3d(dX + 0.1, dY + 0.32, dstZ);
            glVertex3d(dX + 0.3, dY + 0.25, dstZ);
            glEnd();
        }
        break;
    case ED_UP:
        if (!oneway && glBeginOpen) {
            if (!neighbours) {
                glVertex3d(dX + 0.55, dY + 0.25, dstZ);
                glVertex3d(dX + 0.63, dY + 0.25, dstZ);
                glEnd();
            } else {
                glVertex3d(dX + 0.75, dY + 0.25, dstZ);
                glEnd();
            }
            break;
        }
    case ED_DOWN:
        if (!oneway && glBeginOpen) {
            if (!neighbours) {
                glVertex3d(dX + 0.45, dY + 0.75, dstZ);
                glVertex3d(dX + 0.37, dY + 0.75, dstZ);
                glEnd();
            } else {
                glVertex3d(dX + 0.25, dY + 0.75, dstZ);
                glEnd();
            }
            break;
        }
    case ED_UNKNOWN:
        if (glBeginOpen) {
            glVertex3d(dX + 0.75, dY + 0.75, dstZ);
            glVertex3d(dX + 0.5, dY + 0.5, dstZ);
            glEnd();
        }
        glBegin(GL_TRIANGLES);
        glVertex3d(dX + 0.5, dY + 0.5, dstZ);
        glVertex3d(dX + 0.7, dY + 0.55, dstZ);
        glVertex3d(dX + 0.55, dY + 0.7, dstZ);
        glEnd();
        break;
    case ED_NONE:
        if (glBeginOpen) {
            glVertex3d(dX + 0.75, dY + 0.75, dstZ);
            glVertex3d(dX + 0.5, dY + 0.5, dstZ);
            glEnd();
        }
        glBegin(GL_TRIANGLES);
        glVertex3d(dX + 0.5, dY + 0.5, dstZ);
        glVertex3d(dX + 0.7, dY + 0.55, dstZ);
        glVertex3d(dX + 0.55, dY + 0.7, dstZ);
        glEnd();
        break;
    }

    glDisable(GL_BLEND);
    glColor4d(1.0f, 1.0f, 1.0f, 0.70);

    glPopMatrix();
}

void MapCanvas::makeGlLists()
{
    m_wall_north_gllist = glGenLists(1);
    glNewList(m_wall_north_gllist, GL_COMPILE);
    glBegin(GL_LINES);
    glVertex3d(0.0, 0.0 + ROOM_WALL_ALIGN, 0.0);
    glVertex3d(1.0, 0.0 + ROOM_WALL_ALIGN, 0.0);
    glEnd();
    glEndList();

    m_wall_south_gllist = glGenLists(1);
    glNewList(m_wall_south_gllist, GL_COMPILE);
    glBegin(GL_LINES);
    glVertex3d(0.0, 1.0 - ROOM_WALL_ALIGN, 0.0);
    glVertex3d(1.0, 1.0 - ROOM_WALL_ALIGN, 0.0);
    glEnd();
    glEndList();

    m_wall_east_gllist = glGenLists(1);
    glNewList(m_wall_east_gllist, GL_COMPILE);
    glBegin(GL_LINES);
    glVertex3d(1.0 - ROOM_WALL_ALIGN, 0.0, 0.0);
    glVertex3d(1.0 - ROOM_WALL_ALIGN, 1.0, 0.0);
    glEnd();
    glEndList();

    m_wall_west_gllist = glGenLists(1);
    glNewList(m_wall_west_gllist, GL_COMPILE);
    glBegin(GL_LINES);
    glVertex3d(0.0 + ROOM_WALL_ALIGN, 0.0, 0.0);
    glVertex3d(0.0 + ROOM_WALL_ALIGN, 1.0, 0.0);
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
    glLineWidth (devicePixelRatio() * 3.0);
    glBegin(GL_LINES);
    glVertex3d(0.69, 0.31, 0.0);
    glVertex3d(0.63, 0.37, 0.0);
    glVertex3d(0.57, 0.31, 0.0);
    glVertex3d(0.69, 0.43, 0.0);
    glEnd();
    glLineWidth (devicePixelRatio() * 2.0);
    glEndList();

    m_door_down_gllist = glGenLists(1);
    glNewList(m_door_down_gllist, GL_COMPILE);
    glLineWidth (devicePixelRatio() * 3.0);
    glBegin(GL_LINES);
    glVertex3d(0.31, 0.69, 0.0);
    glVertex3d(0.37, 0.63, 0.0);
    glVertex3d(0.31, 0.57, 0.0);
    glVertex3d(0.43, 0.69, 0.0);
    glEnd();
    glLineWidth (devicePixelRatio() * 2.0);
    glEndList();

    m_room_gllist = glGenLists(1);
    glNewList(m_room_gllist, GL_COMPILE);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2d(0, 0);
    glVertex3d(0.0, 1.0, 0.0);
    glTexCoord2d(0, 1);
    glVertex3d(0.0, 0.0, 0.0);
    glTexCoord2d(1, 0);
    glVertex3d(1.0, 1.0, 0.0);
    glTexCoord2d(1, 1);
    glVertex3d(1.0, 0.0, 0.0);
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
    glBegin(GL_TRIANGLE_STRIP);
    glVertex3d(-0.2, 1.2, 0.0);
    glVertex3d(-0.2, -0.2, 0.0);
    glVertex3d(1.2, 1.2, 0.0);
    glVertex3d(1.2, -0.2, 0.0);
    glEnd();
    glEndList();

    m_character_hint_gllist = glGenLists(1);
    glNewList(m_character_hint_gllist, GL_COMPILE);
    glBegin(GL_LINE_LOOP);
    glVertex3d(-0.5, 0.0, 0.0);
    glVertex3d(0.75, 0.5, 0.0);
    glVertex3d(0.25, 0.0, 0.0);
    glVertex3d(0.75, -0.5, 0.0);
    glEnd();
    glEndList();

    m_character_hint_inner_gllist = glGenLists(1);
    glNewList(m_character_hint_inner_gllist, GL_COMPILE);
    glBegin(GL_TRIANGLE_STRIP);
    glVertex3d(0.75, 0.5, 0.0);
    glVertex3d(-0.5, 0.0, 0.0);
    glVertex3d(0.25, 0.0, 0.0);
    glVertex3d(0.75, -0.5, 0.0);
    glEnd();
    glEndList();

    m_flow_begin_gllist[ED_NORTH] = glGenLists(1);
    glNewList(m_flow_begin_gllist[ED_NORTH], GL_COMPILE);
    glBegin(GL_LINE_STRIP);
    glVertex3d(0.5, 0.5, 0.1);
    glVertex3d(0.5, 0.0, 0.1);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex3d(0.44, +0.20, 0.1);
    glVertex3d(0.50, +0.00, 0.1);
    glVertex3d(0.56, +0.20, 0.1);
    glEnd();
    glEndList();

    m_flow_begin_gllist[ED_SOUTH] = glGenLists(1);
    glNewList(m_flow_begin_gllist[ED_SOUTH], GL_COMPILE);
    glBegin(GL_LINE_STRIP);
    glVertex3d(0.5, 0.5, 0.1);
    glVertex3d(0.5, 1.0, 0.1);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex3d(0.44, +0.80, 0.1);
    glVertex3d(0.50, +1.00, 0.1);
    glVertex3d(0.56, +0.80, 0.1);
    glEnd();
    glEndList();

    m_flow_begin_gllist[ED_EAST] = glGenLists(1);
    glNewList(m_flow_begin_gllist[ED_EAST], GL_COMPILE);
    glBegin(GL_LINE_STRIP);
    glVertex3d(0.5, 0.5, 0.1);
    glVertex3d(1.0, 0.5, 0.1);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex3d(0.80, +0.44, 0.1);
    glVertex3d(1.00, +0.50, 0.1);
    glVertex3d(0.80, +0.56, 0.1);
    glEnd();
    glEndList();

    m_flow_begin_gllist[ED_WEST] = glGenLists(1);
    glNewList(m_flow_begin_gllist[ED_WEST], GL_COMPILE);
    glBegin(GL_LINE_STRIP);
    glVertex3d(0.5, 0.5, 0.1);
    glVertex3d(0.0, 0.5, 0.1);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex3d(0.20, +0.44, 0.1);
    glVertex3d(0.00, +0.50, 0.1);
    glVertex3d(0.20, +0.56, 0.1);
    glEnd();
    glEndList();

    m_flow_begin_gllist[ED_UP] = glGenLists(1);
    glNewList(m_flow_begin_gllist[ED_UP], GL_COMPILE);
    glBegin(GL_LINE_STRIP);
    glVertex3d(0.5, 0.5, 0.1);
    glVertex3d(0.75, 0.25, 0.1);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex3d(0.51, 0.42, 0.1);
    glVertex3d(0.64, 0.37, 0.1);
    glVertex3d(0.60, 0.48, 0.1);
    glEnd();
    glEndList();

    m_flow_begin_gllist[ED_DOWN] = glGenLists(1);
    glNewList(m_flow_begin_gllist[ED_DOWN], GL_COMPILE);
    glBegin(GL_LINE_STRIP);
    glVertex3d(0.5, 0.5, 0.1);
    glVertex3d(0.25, 0.75, 0.1);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex3d(0.36, 0.57, 0.1);
    glVertex3d(0.33, 0.67, 0.1);
    glVertex3d(0.44, 0.63, 0.1);
    glEnd();
    glEndList();

    m_flow_end_gllist[ED_SOUTH] = glGenLists(1);
    glNewList(m_flow_end_gllist[ED_SOUTH], GL_COMPILE);
    glBegin(GL_LINE_STRIP);
    glVertex3d(0.0, 0.0, 0.1);
    glVertex3d(0.0,  0.5, 0.1);
    glEnd();
    glEndList();

    m_flow_end_gllist[ED_NORTH] = glGenLists(1);
    glNewList(m_flow_end_gllist[ED_NORTH], GL_COMPILE);
    glBegin(GL_LINE_STRIP);
    glVertex3d(0.0, -0.5, 0.1);
    glVertex3d(0.0, 0.0, 0.1);
    glEnd();
    glEndList();

    m_flow_end_gllist[ED_WEST] = glGenLists(1);
    glNewList(m_flow_end_gllist[ED_WEST], GL_COMPILE);
    glBegin(GL_LINE_STRIP);
    glVertex3d(-0.5, 0.0, 0.1);
    glVertex3d( 0.0, 0.0, 0.1);
    glEnd();
    glEndList();

    m_flow_end_gllist[ED_EAST] = glGenLists(1);
    glNewList(m_flow_end_gllist[ED_EAST], GL_COMPILE);
    glBegin(GL_LINE_STRIP);
    glVertex3d(0.5, 0.0, 0.1);
    glVertex3d(0.0, 0.0, 0.1);
    glEnd();
    glEndList();

    m_flow_end_gllist[ED_DOWN] = glGenLists(1);
    glNewList(m_flow_end_gllist[ED_DOWN], GL_COMPILE);
    glBegin(GL_LINE_STRIP);
    glVertex3d(-0.25, 0.25, 0.1);
    glVertex3d(0.0, 0.0, 0.1);
    glEnd();
    glEndList();

    m_flow_end_gllist[ED_UP] = glGenLists(1);
    glNewList(m_flow_end_gllist[ED_UP], GL_COMPILE);
    glBegin(GL_LINE_STRIP);
    glVertex3d(0.25, -0.25, 0.1);
    glVertex3d(0.0, 0.0, 0.1);
    glEnd();
    glEndList();

    return;
}

float MapCanvas::getDW() const
{
    return ((float)width() / (float)BASESIZEX / 12.0f / (float)m_scaleFactor);
}

float MapCanvas::getDH() const
{
    return ((float)height() / (float)BASESIZEY / 12.0f / (float)m_scaleFactor);
}

void MapCanvas::renderText(float x, float y, const QString &text, QColor color,
                           uint fontFormatFlag, double rotationAngle)
{
    // http://stackoverflow.com/questions/28216001/how-to-render-text-with-qopenglwidget/28517897
    QVector3D vectorIn = {x, y, CAMERA_Z_DISTANCE};
    QVector3D projected = vectorIn.project(m_model, m_projection, this->rect());
    float textPosX = projected.x();
    float textPosY = height() - projected.y(); // y is inverted

    QPainter painter(this);
    painter.translate(textPosX, textPosY);
    painter.rotate(rotationAngle);
    painter.setPen(color);
    if (ISSET(fontFormatFlag, FFF_ITALICS)) {
        m_glFont->setItalic(true);
    }
    if (ISSET(fontFormatFlag, FFF_UNDERLINE)) {
        m_glFont->setUnderline(true);
    }
    painter.setFont(*m_glFont);
    if (Config().m_antialiasingSamples > 0)
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.drawText(0, 0, text);
    m_glFont->setItalic(false);
    m_glFont->setUnderline(false);
    painter.end();
}

void MapCanvas::onMessageLogged(QOpenGLDebugMessage message)
{
    qWarning() << message;
}
