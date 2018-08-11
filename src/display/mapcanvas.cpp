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

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <QMatrix4x4>
#include <QMessageLogContext>
#include <QOpenGLDebugMessage>
#include <QSize>
#include <QString>
#include <QtGui>

#include "../configuration/configuration.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/room.h"
#include "../global/EnumIndexedArray.h"
#include "../global/roomid.h"
#include "../mainwindow/infomarkseditdlg.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/customaction.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/mmapper2room.h"
#include "../mapdata/roomselection.h"
#include "../pandoragroup/CGroup.h"
#include "../pandoragroup/CGroupChar.h"
#include "../pandoragroup/groupselection.h"
#include "../pandoragroup/mmapper2group.h"
#include "../parser/CommandId.h"
#include "../parser/ConnectedRoomFlags.h"
#include "../parser/ExitsFlags.h"
#include "../parser/PromptFlags.h"
#include "../parser/abstractparser.h"
#include "MapCanvasData.h"
#include "MapCanvasRoomDrawer.h"
#include "OpenGL.h"
#include "RoadIndex.h"
#include "connectionselection.h"
#include "prespammedpath.h"

static auto loadTexture(const QString &name)
{
    const auto texture = new QOpenGLTexture(QImage(name).mirrored());
    if (texture == nullptr)
        throw std::runtime_error(("failed to load: " + name).toStdString());

    if (!texture->isCreated()) {
        qWarning() << "failed to create: " << name << "\n";
        texture->setSize(1);
        texture->create();

        if (!texture->isCreated())
            throw std::runtime_error(("failed to create: " + name).toStdString());
    }

    return texture;
}
static auto loadPixmap(const char *const name, const uint i)
{
    return loadTexture(QString::asprintf(":/pixmaps/%s%u.png", name, i));
}

template<typename E, size_t N>
static void loadPixmapArray(EnumIndexedArray<QOpenGLTexture *, E, N> &textures,
                            const char *const name)
{
    for (uint i = 0u; i < N; ++i)
        textures[static_cast<E>(i)] = loadPixmap(name, i);
}

static constexpr const int BASESIZEX = 528; // base canvas size
static constexpr const int BASESIZEY = BASESIZEX;

enum class StippleType { HalfTone, QuadTone };
static const GLubyte *getStipple(StippleType mode)
{
    static const std::array<GLubyte, 129>
        halftone{0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55,
                 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA,
                 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55,
                 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA,
                 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA,
                 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55,
                 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA,
                 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
                 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55,
                 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0x00};

    static const std::array<GLubyte, 129>
        quadtone{0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22, 0x88, 0x88, 0x88, 0x88, 0x22,
                 0x22, 0x22, 0x22, 0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22, 0x88, 0x88,
                 0x88, 0x88, 0x22, 0x22, 0x22, 0x22, 0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22,
                 0x22, 0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22, 0x88, 0x88, 0x88, 0x88,
                 0x22, 0x22, 0x22, 0x22, 0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22, 0x88,
                 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22, 0x88, 0x88, 0x88, 0x88, 0x22, 0x22,
                 0x22, 0x22, 0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22, 0x88, 0x88, 0x88,
                 0x88, 0x22, 0x22, 0x22, 0x22, 0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22,
                 0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22, 0x88, 0x88, 0x88, 0x88, 0x22,
                 0x22, 0x22, 0x22, 0x88, 0x88, 0x88, 0x88, 0x22, 0x22, 0x22, 0x22, 0x00};
    switch (mode) {
    case StippleType::QuadTone:
        return quadtone.data();

    case StippleType::HalfTone:
    default:
        return halftone.data();
    }
}

QColor MapCanvasData::g_noFleeColor = QColor(123, 63, 0);

MapCanvas::MapCanvas(MapData *mapData,
                     PrespammedPath *prespammedPath,
                     Mmapper2Group *groupManager,
                     QWidget *parent)
    : QOpenGLWidget(parent)
    , MapCanvasData(mapData, prespammedPath, *this)
    , m_groupManager(groupManager)
{
    setCursor(Qt::OpenHandCursor);

    m_opengl.initFont(static_cast<QPaintDevice *>(this));

    setContextMenuPolicy(Qt::CustomContextMenu);

    m_infoMarksEditDlg = new InfoMarksEditDlg(mapData, this);
    connect(m_infoMarksEditDlg, SIGNAL(mapChanged()), this, SLOT(update()));
    connect(m_infoMarksEditDlg,
            &InfoMarksEditDlg::closeEventReceived,
            this,
            &MapCanvas::onInfoMarksEditDlgClose);
    connect(m_data, SIGNAL(updateCanvas()), this, SLOT(update()));

    int samples = Config().canvas.antialiasingSamples;
    if (samples <= 0) {
        samples = 2; // Default to 2 samples to prevent restart
    }
    QSurfaceFormat format;
    format.setVersion(1, 0);
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

    /* TODO: if you make these smart pointers,
     * then you won't have to delete them manually */

    for (auto &x : m_textures.terrain)
        delete std::exchange(x, nullptr);
    for (auto &x : m_textures.road)
        delete std::exchange(x, nullptr);
    for (auto &x : m_textures.trail)
        delete std::exchange(x, nullptr);
    for (auto &x : m_textures.load)
        delete std::exchange(x, nullptr);
    for (auto &x : m_textures.mob)
        delete std::exchange(x, nullptr);
    delete std::exchange(m_textures.update, nullptr);

    if (auto tmp = std::exchange(m_roomSelection, nullptr)) {
        m_data->unselect(tmp);
    }

    delete std::exchange(m_connectionSelection, nullptr);

    doneCurrent();
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
    if (arg >= 0) {
        return static_cast<int>(arg + 0.5);
    }
    return static_cast<int>(arg - 0.5);
}

void MapCanvas::setCanvasMouseMode(CanvasMouseMode mode)
{
    m_canvasMouseMode = mode;

    if (m_canvasMouseMode != CanvasMouseMode::EDIT_INFOMARKS) {
        m_infoMarksEditDlg->hide();
        m_infoMarkSelection = false;
    }

    clearRoomSelection();
    clearConnectionSelection();

    switch (m_canvasMouseMode) {
    case CanvasMouseMode::MOVE:
        setCursor(Qt::OpenHandCursor);
        break;
    default:
    case CanvasMouseMode::SELECT_CONNECTIONS:
    case CanvasMouseMode::EDIT_INFOMARKS:
        setCursor(Qt::CrossCursor);
        break;
    case CanvasMouseMode::SELECT_ROOMS:
    case CanvasMouseMode::CREATE_ROOMS:
    case CanvasMouseMode::CREATE_CONNECTIONS:
    case CanvasMouseMode::CREATE_ONEWAY_CONNECTIONS:
        setCursor(Qt::ArrowCursor);
        break;
    }

    m_selectedArea = false;
    update();
}

void MapCanvas::clearRoomSelection()
{
    if (m_roomSelection != nullptr) {
        m_data->unselect(m_roomSelection);
    }
    m_roomSelection = nullptr;
    emit newRoomSelection(m_roomSelection);
}

void MapCanvas::clearConnectionSelection()
{
    delete m_connectionSelection;
    m_connectionSelection = nullptr;
    emit newConnectionSelection(m_connectionSelection);
}

void MapCanvas::wheelEvent(QWheelEvent *event)
{
    if (event->delta() > 100) {
        switch (m_canvasMouseMode) {
        case CanvasMouseMode::MOVE:
            if ((event->modifiers() & Qt::CTRL) != 0u) {
                layerDown();
            } else {
                zoomIn();
            }
            break;
        case CanvasMouseMode::EDIT_INFOMARKS:
        case CanvasMouseMode::SELECT_ROOMS:
        case CanvasMouseMode::SELECT_CONNECTIONS:
        case CanvasMouseMode::CREATE_ROOMS:
        case CanvasMouseMode::CREATE_CONNECTIONS:
        case CanvasMouseMode::CREATE_ONEWAY_CONNECTIONS:
            layerDown();
            break;
        default:;
        }
    }

    if (event->delta() < -100) {
        switch (m_canvasMouseMode) {
        case CanvasMouseMode::MOVE:
            if ((event->modifiers() & Qt::CTRL) != 0u) {
                layerUp();
            } else {
                zoomOut();
            }
            break;
        case CanvasMouseMode::EDIT_INFOMARKS:
        case CanvasMouseMode::SELECT_ROOMS:
        case CanvasMouseMode::SELECT_CONNECTIONS:
        case CanvasMouseMode::CREATE_ROOMS:
        case CanvasMouseMode::CREATE_CONNECTIONS:
        case CanvasMouseMode::CREATE_ONEWAY_CONNECTIONS:
            layerUp();
            break;
        default:;
        }
    }
}

void MapCanvas::forceMapperToRoom()
{
    const RoomSelection *tmpSel = nullptr;
    if (m_roomSelection != nullptr) {
        tmpSel = m_roomSelection;
        m_roomSelection = nullptr;
    } else {
        tmpSel = m_data->select(Coordinate(GLtoMap(m_sel1.x), GLtoMap(m_sel1.y), m_sel1.layer));
    }
    if (tmpSel->size() == 1) {
        if (Config().general.mapMode == MapMode::OFFLINE) {
            const Room *r = tmpSel->values().front();
            auto ev = ParseEvent::createEvent(CommandIdType::UNKNOWN,
                                              r->getName(),
                                              r->getDynamicDescription(),
                                              r->getStaticDescription(),
                                              ExitsFlagsType{},
                                              PromptFlagsType{},
                                              ConnectedRoomFlagsType{});

            // FIXME: need to force MainWindow's Mmapper2PathMachine's PathMachine's state to SYNCING,
            // because this is ignored when the state is APPROVED (it alternates between the two).
            // Alternate hack workaround: just trigger the event twice.
            for (auto hack = 0; hack < 2; ++hack) {
                emit charMovedEvent(SigParseEvent{ev});
            }
        } else {
            emit setCurrentRoom(tmpSel->keys().front());
        }
    }
    m_data->unselect(tmpSel);
    update();
}

void MapCanvas::createRoom()
{
    const RoomSelection *tmpSel = m_data->select(
        Coordinate(GLtoMap(m_sel1.x), GLtoMap(m_sel1.y), m_sel1.layer));
    if (tmpSel->isEmpty()) {
        Coordinate c = Coordinate(GLtoMap(m_sel1.x), GLtoMap(m_sel1.y), m_currentLayer);
        m_data->createEmptyRoom(c);
    }
    m_data->unselect(tmpSel);
    update();
}

void MapCanvas::mousePressEvent(QMouseEvent *const event)
{
    QVector3D v = unproject(event);
    m_sel1.x = v.x();
    m_sel1.y = v.y();

    m_sel2.x = m_sel1.x;
    m_sel2.y = m_sel1.y;
    m_sel1.layer = m_currentLayer;
    m_sel2.layer = m_currentLayer;

    if ((event->buttons() & Qt::LeftButton) != 0u) {
        m_mouseLeftPressed = true;
    }
    if ((event->buttons() & Qt::RightButton) != 0u) {
        m_mouseRightPressed = true;
    }

    if (!m_mouseLeftPressed && m_mouseRightPressed) {
        if (m_canvasMouseMode == CanvasMouseMode::MOVE) {
            // Select the room under the cursor
            Coordinate c = Coordinate(GLtoMap(m_sel1.x), GLtoMap(m_sel1.y), m_sel1.layer);
            if (m_roomSelection != nullptr) {
                m_data->unselect(m_roomSelection);
                m_roomSelection = nullptr;
            }
            m_roomSelection = m_data->select(c);
            if (m_roomSelection->isEmpty()) {
                m_data->unselect(m_roomSelection);
                m_roomSelection = nullptr;
            }
            emit newRoomSelection(m_roomSelection);
            update();
        }
        m_mouseRightPressed = false;
        event->accept();
        return;
    }

    switch (m_canvasMouseMode) {
    case CanvasMouseMode::EDIT_INFOMARKS:
        m_infoMarkSelection = true;
        m_infoMarksEditDlg->hide();
        update();
        break;
    case CanvasMouseMode::MOVE:
        if ((event->buttons() & Qt::LeftButton) != 0u) {
            setCursor(Qt::ClosedHandCursor);
            m_moveBackup.x = m_sel1.x;
            m_moveBackup.y = m_sel1.y;
        }
        break;

    case CanvasMouseMode::SELECT_ROOMS:
        // Force mapper to room shortcut
        if (((event->buttons() & Qt::LeftButton) != 0u) && ((event->modifiers() & Qt::CTRL) != 0u)
            && ((event->modifiers() & Qt::ALT) != 0u)) {
            if (m_roomSelection != nullptr) {
                m_data->unselect(m_roomSelection);
                m_roomSelection = nullptr;
            }
            m_ctrlPressed = true;
            m_altPressed = true;
            forceMapperToRoom();
            break;
        }
        // Cancel
        if ((event->buttons() & Qt::RightButton) != 0u) {
            m_selectedArea = false;
            if (m_roomSelection != nullptr) {
                m_data->unselect(m_roomSelection);
            }
            m_roomSelection = nullptr;
            emit newRoomSelection(m_roomSelection);
        }
        // Select rooms
        if ((event->buttons() & Qt::LeftButton) != 0u) {
            if (event->modifiers() != Qt::CTRL) {
                const RoomSelection *tmpSel = m_data->select(
                    Coordinate(GLtoMap(m_sel1.x), GLtoMap(m_sel1.y), m_sel1.layer));
                if ((m_roomSelection != nullptr) && !tmpSel->empty()
                    && m_roomSelection->contains(tmpSel->keys().front())) {
                    m_roomSelectionMove.inUse = true;
                    m_roomSelectionMove.x = 0;
                    m_roomSelectionMove.y = 0;
                    m_roomSelectionMove.wrongPlace = false;
                } else {
                    m_roomSelectionMove.inUse = false;
                    m_selectedArea = false;
                    if (m_roomSelection != nullptr) {
                        m_data->unselect(m_roomSelection);
                    }
                    m_roomSelection = nullptr;
                    emit newRoomSelection(m_roomSelection);
                }
                m_data->unselect(tmpSel);
            } else {
                m_ctrlPressed = true;
            }
        }
        update();
        break;

    case CanvasMouseMode::CREATE_ONEWAY_CONNECTIONS:
    case CanvasMouseMode::CREATE_CONNECTIONS:
        // Select connection
        if ((event->buttons() & Qt::LeftButton) != 0u) {
            delete m_connectionSelection;
            m_connectionSelection = new ConnectionSelection(m_data,
                                                            m_sel1.x,
                                                            m_sel1.y,
                                                            m_sel1.layer);
            if (!m_connectionSelection->isFirstValid()) {
                delete m_connectionSelection;
                m_connectionSelection = nullptr;
            }
            emit newConnectionSelection(nullptr);
        }
        // Cancel
        if ((event->buttons() & Qt::RightButton) != 0u) {
            delete m_connectionSelection;
            m_connectionSelection = nullptr;
            emit newConnectionSelection(m_connectionSelection);
        }
        update();
        break;

    case CanvasMouseMode::SELECT_CONNECTIONS:
        if ((event->buttons() & Qt::LeftButton) != 0u) {
            delete m_connectionSelection;
            m_connectionSelection = new ConnectionSelection(m_data,
                                                            m_sel1.x,
                                                            m_sel1.y,
                                                            m_sel1.layer);
            if (!m_connectionSelection->isFirstValid()) {
                delete m_connectionSelection;
                m_connectionSelection = nullptr;
            } else {
                const Room *r1 = m_connectionSelection->getFirst().room;
                ExitDirection dir1 = m_connectionSelection->getFirst().direction;

                if (r1->exit(dir1).outIsEmpty()) {
                    delete m_connectionSelection;
                    m_connectionSelection = nullptr;
                }
            }
            emit newConnectionSelection(nullptr);
        }
        // Cancel
        if ((event->buttons() & Qt::RightButton) != 0u) {
            delete m_connectionSelection;
            m_connectionSelection = nullptr;
            emit newConnectionSelection(m_connectionSelection);
        }
        update();
        break;

    case CanvasMouseMode::CREATE_ROOMS:
        createRoom();
        break;

    default:
        break;
    }
}

void MapCanvas::mouseMoveEvent(QMouseEvent *const event)
{
    if (m_canvasMouseMode != CanvasMouseMode::MOVE) {
        qint8 hScroll, vScroll;
        if (event->pos().y() < 100) {
            vScroll = -1;
        } else if (event->pos().y() > height() - 100) {
            vScroll = 1;
        } else {
            vScroll = 0;
        }

        if (event->pos().x() < 100) {
            hScroll = -1;
        } else if (event->pos().x() > width() - 100) {
            hScroll = 1;
        } else {
            hScroll = 0;
        }

        continuousScroll(hScroll, vScroll);
    }

    QVector3D v = unproject(event);
    m_sel2.x = v.x();
    m_sel2.y = v.y();
    m_sel2.layer = m_currentLayer;

    switch (m_canvasMouseMode) {
    case CanvasMouseMode::EDIT_INFOMARKS:
        update();
        break;
    case CanvasMouseMode::MOVE:
        if (((event->buttons() & Qt::LeftButton) != 0u) && m_mouseLeftPressed) {
            const float scrollfactor = SCROLLFACTOR();
            auto idx = static_cast<int>((m_sel2.x - static_cast<float>(m_moveBackup.x))
                                        / scrollfactor);
            auto idy = static_cast<int>((m_sel2.y - static_cast<float>(m_moveBackup.y))
                                        / scrollfactor);

            emit mapMove(-idx, -idy);

            if (idx != 0) {
                m_moveBackup.x = m_sel2.x - static_cast<float>(idx) * scrollfactor;
            }
            if (idy != 0) {
                m_moveBackup.y = m_sel2.y - static_cast<float>(idy) * scrollfactor;
            }
        }
        break;

    case CanvasMouseMode::SELECT_ROOMS:
        if ((event->buttons() & Qt::LeftButton) != 0u) {
            if (m_roomSelectionMove.inUse) {
                m_roomSelectionMove.x = GLtoMap(m_sel2.x) - GLtoMap(m_sel1.x);
                m_roomSelectionMove.y = GLtoMap(m_sel2.y) - GLtoMap(m_sel1.y);

                Coordinate c(m_roomSelectionMove.x, m_roomSelectionMove.y, 0);
                if (m_data->isMovable(c, m_roomSelection)) {
                    m_roomSelectionMove.wrongPlace = false;
                    setCursor(Qt::ClosedHandCursor);
                } else {
                    m_roomSelectionMove.wrongPlace = true;
                    setCursor(Qt::ForbiddenCursor);
                }
            } else {
                m_selectedArea = true;
            }
        }
        update();
        break;

    case CanvasMouseMode::CREATE_ONEWAY_CONNECTIONS:
    case CanvasMouseMode::CREATE_CONNECTIONS:
        if ((event->buttons() & Qt::LeftButton) != 0u) {
            if (m_connectionSelection != nullptr) {
                m_connectionSelection->setSecond(m_data, m_sel2.x, m_sel2.y, m_sel2.layer);

                const Room *r1 = m_connectionSelection->getFirst().room;
                ExitDirection dir1 = m_connectionSelection->getFirst().direction;
                const Room *r2 = m_connectionSelection->getSecond().room;
                ExitDirection dir2 = m_connectionSelection->getSecond().direction;

                if (r2 != nullptr) {
                    if ((r1->exit(dir1).containsOut(r2->getId()))
                        && (r2->exit(dir2).containsOut(r1->getId()))) {
                        m_connectionSelection->removeSecond();
                    }
                }
                update();
            }
        }
        break;

    case CanvasMouseMode::SELECT_CONNECTIONS:
        if ((event->buttons() & Qt::LeftButton) != 0u) {
            if (m_connectionSelection != nullptr) {
                m_connectionSelection->setSecond(m_data, m_sel2.x, m_sel2.y, m_sel2.layer);

                const Room *r1 = m_connectionSelection->getFirst().room;
                ExitDirection dir1 = m_connectionSelection->getFirst().direction;
                const Room *r2 = m_connectionSelection->getSecond().room;
                ExitDirection dir2 = m_connectionSelection->getSecond().direction;

                if (r2 != nullptr) {
                    if (!(r1->exit(dir1).containsOut(r2->getId()))
                        || !(r2->exit(dir2).containsOut(r1->getId()))) { //not two ways
                        if (dir2 != ExitDirection::UNKNOWN) {
                            m_connectionSelection->removeSecond();
                        } else if (dir2 == ExitDirection::UNKNOWN
                                   && (!(r1->exit(dir1).containsOut(r2->getId()))
                                       || (r1->exit(dir1).containsIn(r2->getId())))) { //not oneway
                            m_connectionSelection->removeSecond();
                        }
                    }
                }
                update();
            }
        }
        break;

    case CanvasMouseMode::CREATE_ROOMS:
        break;

    default:
        break;
    }
}

void MapCanvas::mouseReleaseEvent(QMouseEvent *const event)
{
    continuousScroll(0, 0);
    QVector3D v = unproject(event);
    m_sel2.x = v.x();
    m_sel2.y = v.y();
    m_sel2.layer = m_currentLayer;

    if (m_mouseRightPressed) {
        m_mouseRightPressed = false;
    }

    switch (m_canvasMouseMode) {
    case CanvasMouseMode::EDIT_INFOMARKS:
        if (m_mouseLeftPressed) {
            m_mouseLeftPressed = false;
            m_infoMarksEditDlg->setPoints(m_sel1.x, m_sel1.y, m_sel2.x, m_sel2.y, m_sel1.layer);
            m_infoMarksEditDlg->show();
        } else {
            m_infoMarkSelection = false;
            m_infoMarksEditDlg->hide();
        }
        break;
    case CanvasMouseMode::MOVE:
        setCursor(Qt::OpenHandCursor);
        if (m_mouseLeftPressed) {
            m_mouseLeftPressed = false;
        }
        break;
    case CanvasMouseMode::SELECT_ROOMS:
        setCursor(Qt::ArrowCursor);
        if (m_ctrlPressed && m_altPressed) {
            break;
        }

        if (m_mouseLeftPressed) {
            m_mouseLeftPressed = false;

            if (m_roomSelectionMove.inUse) {
                m_roomSelectionMove.inUse = false;
                if (!m_roomSelectionMove.wrongPlace && (m_roomSelection != nullptr)) {
                    Coordinate moverel(m_roomSelectionMove.x, m_roomSelectionMove.y, 0);
                    m_data->execute(new GroupAction(new MoveRelative(moverel), m_roomSelection),
                                    m_roomSelection);
                }
            } else {
                if (m_roomSelection == nullptr) {
                    //add rooms to default selections
                    m_roomSelection = m_data->select(Coordinate(GLtoMap(m_sel1.x),
                                                                GLtoMap(m_sel1.y),
                                                                m_sel1.layer),
                                                     Coordinate(GLtoMap(m_sel2.x),
                                                                GLtoMap(m_sel2.y),
                                                                m_sel2.layer));
                } else {
                    //add or remove rooms to/from default selection
                    const RoomSelection *tmpSel = m_data->select(Coordinate(GLtoMap(m_sel1.x),
                                                                            GLtoMap(m_sel1.y),
                                                                            m_sel1.layer),
                                                                 Coordinate(GLtoMap(m_sel2.x),
                                                                            GLtoMap(m_sel2.y),
                                                                            m_sel2.layer));
                    QList<RoomId> keys = tmpSel->keys();
                    for (RoomId key : keys) {
                        if (m_roomSelection->contains(key)) {
                            m_data->unselect(key, m_roomSelection);
                        } else {
                            m_data->getRoom(key, m_roomSelection);
                        }
                    }
                    m_data->unselect(tmpSel);
                }

                if (m_roomSelection->empty()) {
                    m_data->unselect(m_roomSelection);
                    m_roomSelection = nullptr;
                } else {
                    if (m_roomSelection->size() == 1) {
                        const Room *r = m_roomSelection->values().front();
                        qint32 x = r->getPosition().x;
                        qint32 y = r->getPosition().y;

                        QString etmp = "Exits:";
                        for (const auto j : ALL_EXITS7) {
                            bool door = false;
                            if (r->exit(j).isDoor()) {
                                door = true;
                                etmp += " (";
                            }

                            if (r->exit(j).isExit()) {
                                if (!door) {
                                    etmp += " ";
                                }

                                etmp += lowercaseDirection(j);
                            }

                            if (door) {
                                const auto doorName = r->exit(j).getDoorName();
                                if (!doorName.isEmpty()) {
                                    etmp += "/" + doorName + ")";
                                } else {
                                    etmp += ")";
                                }
                            }
                        }
                        etmp += ".\n";
                        QString ctemp = QString("Selected Room Coordinates: %1 %2").arg(x).arg(y);
                        emit log("MapCanvas",
                                 ctemp + "\n" + r->getName() + "\n" + r->getStaticDescription()
                                     + r->getDynamicDescription() + etmp);

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

    case CanvasMouseMode::CREATE_ONEWAY_CONNECTIONS:
    case CanvasMouseMode::CREATE_CONNECTIONS:
        if (m_mouseLeftPressed) {
            m_mouseLeftPressed = false;

            if (m_connectionSelection == nullptr) {
                m_connectionSelection = new ConnectionSelection(m_data,
                                                                m_sel1.x,
                                                                m_sel1.y,
                                                                m_sel1.layer);
            }
            m_connectionSelection->setSecond(m_data, m_sel2.x, m_sel2.y, m_sel2.layer);

            if (!m_connectionSelection->isValid()) {
                delete m_connectionSelection;
                m_connectionSelection = nullptr;
            } else {
                const Room *r1 = m_connectionSelection->getFirst().room;
                ExitDirection dir1 = m_connectionSelection->getFirst().direction;
                const Room *r2 = m_connectionSelection->getSecond().room;
                ExitDirection dir2 = m_connectionSelection->getSecond().direction;

                if (r2 != nullptr) {
                    const RoomSelection *tmpSel = m_data->select();
                    m_data->getRoom(r1->getId(), tmpSel);
                    m_data->getRoom(r2->getId(), tmpSel);

                    delete std::exchange(m_connectionSelection, nullptr);

                    if (!(r1->exit(dir1).containsOut(r2->getId()))
                        || !(r2->exit(dir2).containsOut(r1->getId()))) {
                        if (m_canvasMouseMode != CanvasMouseMode::CREATE_ONEWAY_CONNECTIONS) {
                            m_data->execute(new AddTwoWayExit(r1->getId(), r2->getId(), dir1, dir2),
                                            tmpSel);
                        } else {
                            m_data->execute(new AddOneWayExit(r1->getId(), r2->getId(), dir1),
                                            tmpSel);
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

    case CanvasMouseMode::SELECT_CONNECTIONS:
        if (m_mouseLeftPressed) {
            m_mouseLeftPressed = false;

            if (m_connectionSelection == nullptr) {
                m_connectionSelection = new ConnectionSelection(m_data,
                                                                m_sel1.x,
                                                                m_sel1.y,
                                                                m_sel1.layer);
            }
            m_connectionSelection->setSecond(m_data, m_sel2.x, m_sel2.y, m_sel2.layer);

            if (!m_connectionSelection->isValid()) {
                delete m_connectionSelection;
                m_connectionSelection = nullptr;
            } else {
                const Room *r1 = m_connectionSelection->getFirst().room;
                ExitDirection dir1 = m_connectionSelection->getFirst().direction;
                const Room *r2 = m_connectionSelection->getSecond().room;
                ExitDirection dir2 = m_connectionSelection->getSecond().direction;

                if (r2 != nullptr) {
                    if (!(r1->exit(dir1).containsOut(r2->getId()))
                        || !(r2->exit(dir2).containsOut(r1->getId()))) {
                        if (dir2 != ExitDirection::UNKNOWN) {
                            delete m_connectionSelection;
                            m_connectionSelection = nullptr;
                        } else if (dir2 == ExitDirection::UNKNOWN
                                   && (!(r1->exit(dir1).containsOut(r2->getId()))
                                       || (r1->exit(dir1).containsIn(r2->getId())))) { //not oneway
                            delete m_connectionSelection;
                            m_connectionSelection = nullptr;
                        }
                    }
                }
            }
            emit newConnectionSelection(m_connectionSelection);
        }
        update();
        break;

    case CanvasMouseMode::CREATE_ROOMS:
        if (m_mouseLeftPressed) {
            m_mouseLeftPressed = false;
        }
        break;

    default:
        break;
    }

    m_altPressed = false;
    m_ctrlPressed = false;
}

QSize MapCanvas::minimumSizeHint() const
{
    return {100, 100};
}

QSize MapCanvas::sizeHint() const
{
    return {BASESIZEX, BASESIZEY};
}

void MapCanvas::setScroll(int x, int y)
{
    m_scroll.x = x;
    m_scroll.y = y;

    resizeGL(width(), height());
}

void MapCanvas::setHorizontalScroll(int x)
{
    m_scroll.x = x;

    resizeGL(width(), height());
}

void MapCanvas::setVerticalScroll(int y)
{
    m_scroll.y = y;

    resizeGL(width(), height());
}

void MapCanvas::zoomIn()
{
    m_scaleFactor += 0.05f;
    if (m_scaleFactor > 2.0f) {
        m_scaleFactor -= 0.05f;
    }

    resizeGL(width(), height());
}

void MapCanvas::zoomOut()
{
    m_scaleFactor -= 0.05f;
    if (m_scaleFactor < 0.04f) {
        m_scaleFactor += 0.05f;
    }

    resizeGL(width(), height());
}

void MapCanvas::initializeGL()
{
    m_opengl.initializeOpenGLFunctions();

    QByteArray version(reinterpret_cast<const char *>(m_opengl.glGetString(GL_VERSION)));
    QByteArray renderer(reinterpret_cast<const char *>(m_opengl.glGetString(GL_RENDERER)));
    QByteArray vendor(reinterpret_cast<const char *>(m_opengl.glGetString(GL_VENDOR)));
    QByteArray isOpenGLES((context()->isOpenGLES() ? "true" : "false"));
    qInfo() << "OpenGL Version: " << version;
    qInfo() << "OpenGL Renderer: " << renderer;
    qInfo() << "OpenGL Vendor: " << vendor;
    qInfo() << "OpenGLES: " << isOpenGLES;
    emit log("MapCanvas", "OpenGL Version: " + version);
    emit log("MapCanvas", "OpenGL Renderer: " + renderer);
    emit log("MapCanvas", "OpenGL Vendor: " + vendor);
    emit log("MapCanvas", "OpenGLES: " + isOpenGLES);

    QString contextStr = QString("%1.%2 ")
                             .arg(context()->format().majorVersion())
                             .arg(context()->format().minorVersion());
    contextStr.append((context()->isValid() ? "(valid)" : "(invalid)"));
    qInfo() << "Current OpenGL Context: " << contextStr;
    emit log("MapCanvas", "Current OpenGL Context: " + contextStr);

    m_logger = new QOpenGLDebugLogger(this);
    connect(m_logger,
            &QOpenGLDebugLogger::messageLogged,
            this,
            &MapCanvas::slot_onMessageLoggedDirect,
            Qt::DirectConnection /* NOTE: executed in emitter's thread */);
    if (m_logger->initialize()) {
        m_logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
        m_logger->disableMessages();
        m_logger->enableMessages(QOpenGLDebugMessage::AnySource,
                                 (QOpenGLDebugMessage::ErrorType
                                  | QOpenGLDebugMessage::UndefinedBehaviorType),
                                 QOpenGLDebugMessage::AnySeverity);
    }

    if (Config().canvas.antialiasingSamples > 0) {
        m_opengl.apply(XEnable{XOption::MULTISAMPLE});
    }

    initTextures();

    // <= OpenGL 3.0
    makeGlLists(); // TODO(nschimme): Convert these GlLists into shaders
    m_opengl.glShadeModel(GL_FLAT);
    m_opengl.glPolygonStipple(getStipple(StippleType::HalfTone));

    // >= OpenGL 3.0
    m_opengl.apply(XEnable{XOption::DEPTH_TEST});
    m_opengl.apply(XEnable{XOption::NORMALIZE});
    m_opengl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void MapCanvas::resizeGL(int width, int height)
{
    if (m_textures.update == nullptr) {
        // resizeGL called but initializeGL was not called yet
        return;
    }

    const float swp = m_scaleFactor
                      * (1.0f - (static_cast<float>(width - BASESIZEX) / static_cast<float>(width)));
    const float shp = m_scaleFactor
                      * (1.0f
                         - (static_cast<float>(height - BASESIZEY) / static_cast<float>(height)));

    makeCurrent();
    m_opengl.glViewport(0, 0, width, height);

    // >= OpenGL 3.1
    m_projection.setToIdentity();
    m_projection.frustum(-0.5f, +0.5f, +0.5f, -0.5f, 5.0f, 80.0f);
    m_projection.scale(swp, shp, 1.0f);
    m_projection.translate(-SCROLLFACTOR() * static_cast<float>(m_scroll.x),
                           -SCROLLFACTOR() * static_cast<float>(m_scroll.y),
                           -60.0f);
    m_modelview.setToIdentity();

    // <= OpenGL 3.0
    m_opengl.setMatrix(MatrixType::PROJECTION, m_projection);
    m_opengl.setMatrix(MatrixType::MODELVIEW, m_modelview);

    QVector3D v1 = unproject(QVector3D(0.0f, static_cast<float>(height), CAMERA_Z_DISTANCE));
    m_visible1.x = v1.x();
    m_visible1.y = v1.y();
    QVector3D v2 = unproject(QVector3D(static_cast<float>(width), 0.0f, CAMERA_Z_DISTANCE));
    m_visible2.x = v2.x();
    m_visible2.y = v2.y();

    // Render
    update();
}

void MapCanvas::setTrilinear(QOpenGLTexture *const x) const
{
    if (x != nullptr)
        x->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
}

void MapCanvas::dataLoaded()
{
    m_currentLayer = static_cast<qint16>(m_data->getPosition().z);
    emit onCenter(m_data->getPosition().x, m_data->getPosition().y);
    makeCurrent();
    update();
}

void MapCanvas::moveMarker(const Coordinate &c)
{
    m_data->setPosition(c);
    m_currentLayer = static_cast<qint16>(c.z);
    makeCurrent();
    update();
    emit onCenter(c.x, c.y);
    //emit onEnsureVisible(c.x, c.y);
}

void MapCanvas::drawGroupCharacters()
{
    CGroup *const group = m_groupManager->getGroup();
    if ((group == nullptr) || Config().groupManager.state == GroupManagerState::Off
        || m_data->isEmpty()) {
        return;
    }

    GroupSelection *const selection = group->selectAll();
    for (auto character : *selection) {
        const RoomId id = character->getPosition();
        if (character->getName() != Config().groupManager.charName) {
            const RoomSelection *const roomSelection = m_data->select();
            if (const Room *const r = m_data->getRoom(id, roomSelection)) {
                drawCharacter(r->getPosition(), character->getColor());
            }
            m_data->unselect(id, roomSelection);
        }
    }
    group->unselect(selection);
}

void MapCanvas::drawCharacter(const Coordinate &c, const QColor &color)
{
    qint32 x = c.x;
    qint32 y = c.y;
    qint32 z = c.z;
    qint32 layer = z - m_currentLayer;

    m_opengl.glPushMatrix();
    m_opengl.apply(XColor4d{Qt::black, 0.4f});
    m_opengl.apply(XEnable{XOption::BLEND});
    m_opengl.apply(XDisable{XOption::DEPTH_TEST});

    if (((static_cast<float>(x) < m_visible1.x - 1.0f)
         || (static_cast<float>(x) > m_visible2.x + 1.0f))
        || ((static_cast<float>(y) < m_visible1.y - 1.0f)
            || (static_cast<float>(y) > m_visible2.y + 1.0f))) {
        // Player is distant
        const double cameraCenterX = (m_visible1.x + m_visible2.x) / 2.0f;
        const double cameraCenterY = (m_visible1.y + m_visible2.y) / 2.0f;

        // Calculate degrees from camera center to character
        const double adjacent = cameraCenterY - y;
        const double opposite = cameraCenterX - x;
        const double radians = std::atan2(adjacent, opposite);
        const double degrees = radians * 180.0 / M_PI;

        // Identify character hint coordinates using an elipse to represent the screen
        const auto radiusX = static_cast<double>((m_visible2.x - m_visible1.x) / 2.0f - 0.75f);
        const auto radiusY = static_cast<double>((m_visible2.y - m_visible1.y) / 2.0f - 0.75f);
        const double characterHintX = cameraCenterX + (std::cos(radians) * radiusX * -1);
        const double characterHintY = cameraCenterY + (std::sin(radians) * radiusY * -1);

        // Draw character and rotate according to angle
        m_opengl.glTranslated(characterHintX, characterHintY, m_currentLayer + 0.1);
        m_opengl.glRotatef(static_cast<float>(degrees), 0.0f, 0.0f, 1.0f);

        m_opengl.callList(m_gllist.character_hint.inner);
        m_opengl.apply(XDisable{XOption::BLEND});

        m_opengl.apply(XColor4d{color});
        m_opengl.callList(m_gllist.character_hint.outer);

    } else if (z != m_currentLayer) {
        // Player is not on the same layer
        m_opengl.glTranslated(x, y - 0.5, m_currentLayer + 0.1);
        m_opengl.glRotatef(270.0f, 0.0f, 0.0f, 1.0f);

        m_opengl.callList(m_gllist.character_hint.inner);
        m_opengl.apply(XDisable{XOption::BLEND});

        m_opengl.apply(XColor4d{color});
        m_opengl.callList(m_gllist.character_hint.outer);
    } else {
        // Player is on the same layer and visible
        m_opengl.glTranslated(x - 0.5, y - 0.5, ROOM_Z_DISTANCE * layer + 0.1);

        m_opengl.callList(m_gllist.room_selection.inner);
        m_opengl.apply(XDisable{XOption::BLEND});

        m_opengl.apply(XColor4d{color});
        m_opengl.callList(m_gllist.room_selection.outer);
    }
    m_opengl.apply(XEnable{XOption::DEPTH_TEST});
    m_opengl.glPopMatrix();
}

void MapCanvas::paintGL()
{
    // Background Color
    const auto backgroundColor = Config().canvas.backgroundColor;
    m_opengl.glClearColor(static_cast<float>(backgroundColor.redF()),
                          static_cast<float>(backgroundColor.greenF()),
                          static_cast<float>(backgroundColor.blueF()),
                          static_cast<float>(backgroundColor.alphaF()));

    m_opengl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    MapCanvasRoomDrawer drawer{*static_cast<MapCanvasData *>(this), this->m_opengl};

    if (!m_data->isEmpty()) {
        drawRooms(drawer);

    } else {
        drawer.renderText((m_visible1.x + m_visible2.x) / 2,
                          (m_visible1.y + m_visible2.y) / 2,
                          "No map loaded");
    }

    GLdouble len = 0.2f;
    if (m_selectedArea || m_infoMarkSelection) {
        m_opengl.apply(XEnable{XOption::BLEND});
        m_opengl.apply(XDisable{XOption::DEPTH_TEST});
        m_opengl.apply(XColor4d{Qt::black, 0.5f});
        m_opengl.draw(DrawType::POLYGON,
                      std::vector<Vec3d>{Vec3d{m_sel1.x, m_sel1.y, 0.005},
                                         Vec3d{m_sel2.x, m_sel1.y, 0.005},
                                         Vec3d{m_sel2.x, m_sel2.y, 0.005},
                                         Vec3d{m_sel1.x, m_sel2.y, 0.005}});

        m_opengl.apply(XColor4d{(Qt::white)});
        m_opengl.apply(LineStippleType::FOUR);
        m_opengl.apply(XEnable{XOption::LINE_STIPPLE});
        m_opengl.draw(DrawType::LINE_LOOP,
                      std::vector<Vec3d>{Vec3d{m_sel1.x, m_sel1.y, 0.005},
                                         Vec3d{m_sel2.x, m_sel1.y, 0.005},
                                         Vec3d{m_sel2.x, m_sel2.y, 0.005},
                                         Vec3d{m_sel1.x, m_sel2.y, 0.005}});
        m_opengl.apply(XDisable{XOption::LINE_STIPPLE});
        m_opengl.apply(XDisable{XOption::BLEND});
        m_opengl.apply(XEnable{XOption::DEPTH_TEST});
    }

    if (m_infoMarkSelection) {
        m_opengl.apply(XColor4d{Qt::yellow, 1.0f});
        m_opengl.apply(XDevicePointSize{3.0});
        m_opengl.apply(XDeviceLineWidth{3.0});

        m_opengl.draw(DrawType::LINES,
                      std::vector<Vec3d>{
                          Vec3d{m_sel1.x, m_sel1.y, 0.005},
                          Vec3d{m_sel2.x, m_sel2.y, 0.005},
                      });
    }

    //paint selected connection
    paintSelectedConnection();

    // paint selection
    paintSelection(len);

    //draw the characters before the current position
    drawGroupCharacters();

    //paint char current position
    if (!m_data->isEmpty()) {
        // Use the player's selected color
        const QColor color = Config().groupManager.color;
        drawCharacter(m_data->getPosition(), color);
    }

    drawPreSpammedPath();
}

void MapCanvas::drawRooms(/* TODO: make this const */ MapCanvasRoomDrawer &drawer)
{
    m_data->draw(Coordinate(static_cast<int>(m_visible1.x),
                            static_cast<int>(m_visible1.y),
                            m_currentLayer - 10),
                 Coordinate(static_cast<int>(m_visible2.x + 1),
                            static_cast<int>(m_visible2.y + 1),
                            m_currentLayer + 10),
                 drawer);

    const auto wantInfoMarks = (m_scaleFactor >= 0.25f);
    if (wantInfoMarks) {
        drawer.drawInfoMarks();
    }
}

void MapCanvas::paintSelectedConnection()
{
    if (m_connectionSelection == nullptr || !m_connectionSelection->isFirstValid())
        return;

    /* WARNING: r is reassigned below */
    const Room *r = m_connectionSelection->getFirst().room;

    GLdouble x1p = r->getPosition().x;
    GLdouble y1p = r->getPosition().y;
    GLdouble x2p = m_sel2.x;
    GLdouble y2p = m_sel2.y;

    /* TODO: factor duplicate code using vec2 return value */
    switch (m_connectionSelection->getFirst().direction) {
    case ExitDirection::NORTH:
        y1p -= 0.4;
        break;
    case ExitDirection::SOUTH:
        y1p += 0.4;
        break;
    case ExitDirection::EAST:
        x1p += 0.4;
        break;
    case ExitDirection::WEST:
        x1p -= 0.4;
        break;
    case ExitDirection::UP:
        x1p += 0.3;
        y1p -= 0.3;
        break;
    case ExitDirection::DOWN:
        x1p -= 0.3;
        y1p += 0.3;
        break;
    case ExitDirection::UNKNOWN:
        break;
    default:;
    }

    if (m_connectionSelection->isSecondValid()) {
        /* WARNING: reassignment of r */
        r = m_connectionSelection->getSecond().room;
        x2p = r->getPosition().x;
        y2p = r->getPosition().y;

        switch (m_connectionSelection->getSecond().direction) {
        case ExitDirection::NORTH:
            y2p -= 0.4;
            break;
        case ExitDirection::SOUTH:
            y2p += 0.4;
            break;
        case ExitDirection::EAST:
            x2p += 0.4;
            break;
        case ExitDirection::WEST:
            x2p -= 0.4;
            break;
        case ExitDirection::UP:
            x2p += 0.3;
            y2p -= 0.3;
            break;
        case ExitDirection::DOWN:
            x2p -= 0.3;
            y2p += 0.3;
            break;
        case ExitDirection::UNKNOWN:
            break;
        default:;
        }
    }

    m_opengl.apply(XColor4d{(Qt::red)});
    m_opengl.apply(XDevicePointSize{10.0});
    m_opengl.draw(DrawType::POINTS,
                  std::vector<Vec3d>{
                      Vec3d{x1p, y1p, 0.005},
                      Vec3d{x2p, y2p, 0.005},
                  });
    m_opengl.apply(XDevicePointSize{1.0});

    m_opengl.draw(DrawType::LINES,
                  std::vector<Vec3d>{
                      Vec3d{x1p, y1p, 0.005},
                      Vec3d{x2p, y2p, 0.005},
                  });
    m_opengl.apply(XDisable{XOption::BLEND});
}

void MapCanvas::paintSelection(const GLdouble len)
{
    if (m_roomSelection == nullptr)
        return;

    QList<const Room *> rooms = m_roomSelection->values();
    for (const auto room : rooms) {
        paintSelectedRoom(len, room);
    }
}

void MapCanvas::paintSelectedRoom(const GLdouble len, const Room *const room)
{
    qint32 x = room->getPosition().x;
    qint32 y = room->getPosition().y;
    qint32 z = room->getPosition().z;
    qint32 layer = z - m_currentLayer;

    m_opengl.glPushMatrix();
    m_opengl.glTranslated(x - 0.5, y - 0.5, ROOM_Z_DISTANCE * layer);

    m_opengl.apply(XColor4d{Qt::black, 0.4f});

    m_opengl.apply(XEnable{XOption::BLEND});
    m_opengl.apply(XDisable{XOption::DEPTH_TEST});
    m_opengl.callList(m_gllist.room);

    m_opengl.apply(XColor4d{(Qt::red)});
    m_opengl.draw(DrawType::LINE_STRIP,
                  std::vector<Vec3d>{Vec3d{0 + len, 0, 0.005},
                                     Vec3d{0, 0, 0.005},
                                     Vec3d{0, 0 + len, 0.005}});
    m_opengl.draw(DrawType::LINE_STRIP,
                  std::vector<Vec3d>{Vec3d{0 + len, 1, 0.005},
                                     Vec3d{0, 1, 0.005},
                                     Vec3d{0, 1 - len, 0.005}});
    m_opengl.draw(DrawType::LINE_STRIP,
                  std::vector<Vec3d>{Vec3d{1 - len, 1, 0.005},
                                     Vec3d{1, 1, 0.005},
                                     Vec3d{1, 1 - len, 0.005}});
    m_opengl.draw(DrawType::LINE_STRIP,
                  std::vector<Vec3d>{Vec3d{1 - len, 0, 0.005},
                                     Vec3d{1, 0, 0.005},
                                     Vec3d{1, 0 + len, 0.005}});

    if (m_roomSelectionMove.inUse) {
        if (m_roomSelectionMove.wrongPlace) {
            m_opengl.apply(XColor4d{Qt::red, 0.4f});
        } else {
            m_opengl.apply(XColor4d{Qt::white, 0.4f});
        }

        m_opengl.glTranslated(m_roomSelectionMove.x, m_roomSelectionMove.y, ROOM_Z_DISTANCE * layer);
        m_opengl.callList(m_gllist.room);
    }

    m_opengl.apply(XDisable{XOption::BLEND});
    m_opengl.apply(XEnable{XOption::DEPTH_TEST});

    m_opengl.glPopMatrix();
}

void MapCanvas::drawPreSpammedPath()
{
    if (m_prespammedPath->isEmpty())
        return;

    std::vector<Vec3d> verts{};
    QList<Coordinate> path = m_data->getPath(m_prespammedPath->getQueue());
    Coordinate c1, c2;
    double dx = 0.0, dy = 0.0, dz = 0.0;
    bool anypath = false;

    c1 = m_data->getPosition();
    auto it = path.begin();
    while (it != path.end()) {
        if (!anypath) {
            drawPathStart(c1, verts);
            anypath = true;
        }
        c2 = *it;
        if (!drawPath(c1, c2, dx, dy, dz, verts)) {
            break;
        }
        ++it;
    }
    if (anypath) {
        drawPathEnd(dx, dy, dz, verts);
    }
}

void MapCanvas::drawPathStart(const Coordinate &sc, std::vector<Vec3d> &verts)
{
    const qint32 x1 = sc.x;
    const qint32 y1 = sc.y;
    const qint32 z1 = sc.z;
    const qint32 layer1 = z1 - m_currentLayer;

    m_opengl.glPushMatrix();
    m_opengl.glTranslated(x1, y1, 0);

    // Use the player's color
    QColor color = Config().groupManager.color;
    m_opengl.apply(XColor4d{color});
    m_opengl.apply(XEnable{XOption::BLEND});
    m_opengl.apply(XDevicePointSize{4.0});
    m_opengl.apply(XDeviceLineWidth{4.0});

    const double srcZ = static_cast<double>(ROOM_Z_DISTANCE) * static_cast<double>(layer1) + 0.3;

    verts.emplace_back(0.0, 0.0, srcZ);
}

bool MapCanvas::drawPath(const Coordinate &sc,
                         const Coordinate &dc,
                         double &dx,
                         double &dy,
                         double &dz,
                         std::vector<Vec3d> &verts)
{
    qint32 x1 = sc.x;
    qint32 y1 = sc.y;
    //qint32 z1 = sc.z;

    qint32 x2 = dc.x;
    qint32 y2 = dc.y;
    qint32 z2 = dc.z;
    qint32 layer2 = z2 - m_currentLayer;

    dx = static_cast<double>(x2 - x1);
    dy = static_cast<double>(y2 - y1);
    dz = ROOM_Z_DISTANCE * static_cast<double>(layer2) + 0.3;

    verts.emplace_back(dx, dy, dz);

    return true;
}

void MapCanvas::drawPathEnd(const double dx,
                            const double dy,
                            const double dz,
                            std::vector<Vec3d> &verts)
{
    m_opengl.draw(DrawType::LINE_STRIP, verts);

    m_opengl.apply(XDevicePointSize{8.0});
    m_opengl.draw(DrawType::POINTS,
                  std::vector<Vec3d>{
                      Vec3d{dx, dy, dz},
                  });

    m_opengl.apply(XDeviceLineWidth{2.0});
    m_opengl.apply(XDevicePointSize{2.0});
    m_opengl.apply(XDisable{XOption::BLEND});
    m_opengl.glPopMatrix();
}

void MapCanvas::initTextures()
{
    const auto wantTrilinear = Config().canvas.trilinearFiltering;
#define LOAD_PIXMAP_ARRAY(x) \
    do { \
        loadPixmapArray(this->m_textures.x, #x); \
    } while (false)

    LOAD_PIXMAP_ARRAY(terrain);
    LOAD_PIXMAP_ARRAY(road);
    LOAD_PIXMAP_ARRAY(trail);
    LOAD_PIXMAP_ARRAY(load);
    LOAD_PIXMAP_ARRAY(mob);
    this->m_textures.update = loadPixmap("update", 0u);

    if (wantTrilinear) {
        for (auto x : m_textures.terrain)
            setTrilinear(x);
        for (auto x : m_textures.road)
            setTrilinear(x);
        for (auto x : m_textures.trail)
            setTrilinear(x);
        for (auto x : m_textures.load)
            setTrilinear(x);
        for (auto x : m_textures.mob)
            setTrilinear(x);
        setTrilinear(m_textures.update);
    }
#undef LOAD_PIXMAP_ARRAY
}

// I suspect most of these are just rotated versions of one another.
// If that's the case, then we should be able to remove 3/4 of the
// NESW cases and just write a loop that rotates 90 degrees.
//
// In the long run, if we ever go for a 3d POV, then we may want to
// convert these from display lists to meshes (VBO + texture),
// and we'll want to use instanced rendering.
void MapCanvas::makeGlLists()
{
    const auto getDevicePixelRatio = [this]() {
    #if QT_VERSION >= 0x050600
        return static_cast<float>(devicePixelRatioF());
    #else
        return static_cast<float>(devicePixelRatio());
    #endif
    };
    m_opengl.setDevicePixelRatio(getDevicePixelRatio());

    m_gllist.wall[ExitDirection::NORTH] = m_opengl.compile(
        XDraw{DrawType::LINES,
              std::vector<Vec3d>{Vec3d{0.0, 0.0 + ROOM_WALL_ALIGN, 0.0},
                                 Vec3d{1.0, 0.0 + ROOM_WALL_ALIGN, 0.0}}});
    m_gllist.wall[ExitDirection::SOUTH] = m_opengl.compile(
        XDraw{DrawType::LINES,
              std::vector<Vec3d>{Vec3d{0.0, 1.0 - ROOM_WALL_ALIGN, 0.0},
                                 Vec3d{1.0, 1.0 - ROOM_WALL_ALIGN, 0.0}}});
    m_gllist.wall[ExitDirection::EAST] = m_opengl.compile(
        XDraw{DrawType::LINES,
              std::vector<Vec3d>{Vec3d{1.0 - ROOM_WALL_ALIGN, 0.0, 0.0},
                                 Vec3d{1.0 - ROOM_WALL_ALIGN, 1.0, 0.0}}});
    m_gllist.wall[ExitDirection::WEST] = m_opengl.compile(
        XDraw{DrawType::LINES,
              std::vector<Vec3d>{Vec3d{0.0 + ROOM_WALL_ALIGN, 0.0, 0.0},
                                 Vec3d{0.0 + ROOM_WALL_ALIGN, 1.0, 0.0}}});

    m_gllist.exit.up.opaque = m_opengl.compile(XColor4d{Qt::white} /* was 255, 255, 255, 255 */,
                                               XDraw{DrawType::POLYGON,
                                                     std::vector<Vec3d>{Vec3d{0.75, 0.13, 0.0},
                                                                        Vec3d{0.83, 0.17, 0.0},
                                                                        Vec3d{0.87, 0.25, 0.0},
                                                                        Vec3d{0.83, 0.33, 0.0},
                                                                        Vec3d{0.75, 0.37, 0.0},
                                                                        Vec3d{0.67, 0.33, 0.0},
                                                                        Vec3d{0.63, 0.25, 0.0},
                                                                        Vec3d{0.67, 0.17, 0.0}}},
                                               XColor4d{Qt::black} /* was 0, 0, 0, 255 */,
                                               XDraw{DrawType::LINE_LOOP,
                                                     std::vector<Vec3d>{Vec3d{0.75, 0.13, 0.01},
                                                                        Vec3d{0.83, 0.17, 0.01},
                                                                        Vec3d{0.87, 0.25, 0.01},
                                                                        Vec3d{0.83, 0.33, 0.01},
                                                                        Vec3d{0.75, 0.37, 0.01},
                                                                        Vec3d{0.67, 0.33, 0.01},
                                                                        Vec3d{0.63, 0.25, 0.01},
                                                                        Vec3d{0.67, 0.17, 0.01}}},
                                               XDraw{DrawType::POINTS,
                                                     std::vector<Vec3d>{Vec3d{0.75, 0.25, 0.01}}});
    m_gllist.exit.up.transparent
        = m_opengl.compile(XDraw{DrawType::LINE_LOOP,
                                 std::vector<Vec3d>{Vec3d{0.75, 0.13, 0.01},
                                                    Vec3d{0.83, 0.17, 0.01},
                                                    Vec3d{0.87, 0.25, 0.01},
                                                    Vec3d{0.83, 0.33, 0.01},
                                                    Vec3d{0.75, 0.37, 0.01},
                                                    Vec3d{0.67, 0.33, 0.01},
                                                    Vec3d{0.63, 0.25, 0.01},
                                                    Vec3d{0.67, 0.17, 0.01}}},
                           XDraw{DrawType::POINTS, std::vector<Vec3d>{Vec3d{0.75, 0.25, 0.01}}});
    m_gllist.exit.down.opaque = m_opengl.compile(
        /* was 255, 255, 255, 255 */
        XColor4d{Qt::white, 1.0},
        XDraw{DrawType::POLYGON,
              std::vector<Vec3d>{Vec3d{0.25, 0.63, 0.0},
                                 Vec3d{0.33, 0.67, 0.0},
                                 Vec3d{0.37, 0.75, 0.0},
                                 Vec3d{0.33, 0.83, 0.0},
                                 Vec3d{0.25, 0.87, 0.0},
                                 Vec3d{0.17, 0.83, 0.0},
                                 Vec3d{0.13, 0.75, 0.0},
                                 Vec3d{0.17, 0.67, 0.0}}},
        XColor4d{Qt::black, 1.0} /* was 0, 0, 0, 255 */,
        XDraw{DrawType::LINE_LOOP,
              std::vector<Vec3d>{Vec3d{0.25, 0.63, 0.01},
                                 Vec3d{0.33, 0.67, 0.01},
                                 Vec3d{0.37, 0.75, 0.01},
                                 Vec3d{0.33, 0.83, 0.01},
                                 Vec3d{0.25, 0.87, 0.01},
                                 Vec3d{0.17, 0.83, 0.01},
                                 Vec3d{0.13, 0.75, 0.01},
                                 Vec3d{0.17, 0.67, 0.01}}},
        XDraw{DrawType::LINES,
              std::vector<Vec3d>{Vec3d{0.33, 0.67, 0.01},
                                 Vec3d{0.17, 0.83, 0.01},
                                 Vec3d{0.33, 0.83, 0.01},
                                 Vec3d{0.17, 0.67, 0.01}}});
    m_gllist.exit.down.transparent
        = m_opengl.compile(XDraw{DrawType::LINE_LOOP,
                                 std::vector<Vec3d>{Vec3d{0.25, 0.63, 0.01},
                                                    Vec3d{0.33, 0.67, 0.01},
                                                    Vec3d{0.37, 0.75, 0.01},
                                                    Vec3d{0.33, 0.83, 0.01},
                                                    Vec3d{0.25, 0.87, 0.01},
                                                    Vec3d{0.17, 0.83, 0.01},
                                                    Vec3d{0.13, 0.75, 0.01},
                                                    Vec3d{0.17, 0.67, 0.01}}},
                           XDraw{DrawType::LINES,
                                 std::vector<Vec3d>{Vec3d{0.33, 0.67, 0.01},
                                                    Vec3d{0.17, 0.83, 0.01},
                                                    Vec3d{0.33, 0.83, 0.01},
                                                    Vec3d{0.17, 0.67, 0.01}}});

    m_gllist.door[ExitDirection::NORTH] = m_opengl.compile(
        XDraw{DrawType::LINES,
              std::vector<Vec3d>{Vec3d{0.5, 0.0, 0.0},
                                 Vec3d{0.5, 0.11, 0.0},
                                 Vec3d{0.35, 0.11, 0.0},
                                 Vec3d{0.65, 0.11, 0.0}}});
    m_gllist.door[ExitDirection::SOUTH] = m_opengl.compile(
        XDraw{DrawType::LINES,
              std::vector<Vec3d>{Vec3d{0.5, 1.0, 0.0},
                                 Vec3d{0.5, 0.89, 0.0},
                                 Vec3d{0.35, 0.89, 0.0},
                                 Vec3d{0.65, 0.89, 0.0}}});
    m_gllist.door[ExitDirection::EAST] = m_opengl.compile(
        XDraw{DrawType::LINES,
              std::vector<Vec3d>{Vec3d{0.89, 0.5, 0.0},
                                 Vec3d{1.0, 0.5, 0.0},
                                 Vec3d{0.89, 0.35, 0.0},
                                 Vec3d{0.89, 0.65, 0.0}}});
    m_gllist.door[ExitDirection::WEST] = m_opengl.compile(
        XDraw{DrawType::LINES,
              std::vector<Vec3d>{Vec3d{0.11, 0.5, 0.0},
                                 Vec3d{0.0, 0.5, 0.0},
                                 Vec3d{0.11, 0.35, 0.0},
                                 Vec3d{0.11, 0.65, 0.0}}});
    m_gllist.door[ExitDirection::UP]
        = m_opengl.compile(XDeviceLineWidth{3.0},
                           XDraw{DrawType::LINES,
                                 std::vector<Vec3d>{Vec3d{0.69, 0.31, 0.0},
                                                    Vec3d{0.63, 0.37, 0.0},
                                                    Vec3d{0.57, 0.31, 0.0},
                                                    Vec3d{0.69, 0.43, 0.0}}});
    // XDeviceLineWidth{2.0}
    m_gllist.door[ExitDirection::DOWN]
        = m_opengl.compile(XDeviceLineWidth{3.0},
                           XDraw{DrawType::LINES,
                                 std::vector<Vec3d>{Vec3d{0.31, 0.69, 0.0},
                                                    Vec3d{0.37, 0.63, 0.0},
                                                    Vec3d{0.31, 0.57, 0.0},
                                                    Vec3d{0.43, 0.69, 0.0}}});
    // XDeviceLineWidth{2.0}

    m_gllist.room = m_opengl.compile(
        XDrawTextured{DrawType::TRIANGLE_STRIP,
                      std::vector<TexVert>{TexVert{Vec2d{0, 0}, Vec3d{0.0, 1.0, 0.0}},
                                           TexVert{Vec2d{0, 1}, Vec3d{0.0, 0.0, 0.0}},
                                           TexVert{Vec2d{1, 0}, Vec3d{1.0, 1.0, 0.0}},
                                           TexVert{Vec2d{1, 1}, Vec3d{1.0, 0.0, 0.0}}}});

    m_gllist.room_selection.outer = m_opengl.compile(
        XDraw{DrawType::LINE_LOOP,
              std::vector<Vec3d>{Vec3d{-0.2, -0.2, 0.0},
                                 Vec3d{-0.2, 1.2, 0.0},
                                 Vec3d{1.2, 1.2, 0.0},
                                 Vec3d{1.2, -0.2, 0.0}}});
    m_gllist.room_selection.inner = m_opengl.compile(
        XDraw{DrawType::TRIANGLE_STRIP,
              std::vector<Vec3d>{Vec3d{-0.2, 1.2, 0.0},
                                 Vec3d{-0.2, -0.2, 0.0},
                                 Vec3d{1.2, 1.2, 0.0},
                                 Vec3d{1.2, -0.2, 0.0}}});
    m_gllist.character_hint.outer = m_opengl.compile(
        XDraw{DrawType::LINE_LOOP,
              std::vector<Vec3d>{Vec3d{-0.5, 0.0, 0.0},
                                 Vec3d{0.75, 0.5, 0.0},
                                 Vec3d{0.25, 0.0, 0.0},
                                 Vec3d{0.75, -0.5, 0.0}}});
    m_gllist.character_hint.inner = m_opengl.compile(XDraw{DrawType::TRIANGLE_STRIP,
                                                           std::vector<Vec3d>{
                                                               Vec3d{0.75, 0.5, 0.0},
                                                               Vec3d{-0.5, 0.0, 0.0},
                                                               Vec3d{0.25, 0.0, 0.0},
                                                               Vec3d{0.75, -0.5, 0.0},
                                                           }});

    m_gllist.flow.begin[ExitDirection::NORTH]
        = m_opengl.compile(XDraw{DrawType::LINE_STRIP,
                                 std::vector<Vec3d>{Vec3d{0.5, 0.5, 0.1}, Vec3d{0.5, 0.0, 0.1}}},
                           XDraw{DrawType::TRIANGLES,
                                 std::vector<Vec3d>{Vec3d{0.44, +0.20, 0.1},
                                                    Vec3d{0.50, +0.00, 0.1},
                                                    Vec3d{0.56, +0.20, 0.1}}});
    m_gllist.flow.begin[ExitDirection::SOUTH]
        = m_opengl.compile(XDraw{DrawType::LINE_STRIP,
                                 std::vector<Vec3d>{Vec3d{0.5, 0.5, 0.1}, Vec3d{0.5, 1.0, 0.1}}},
                           XDraw{DrawType::TRIANGLES,
                                 std::vector<Vec3d>{Vec3d{0.44, +0.80, 0.1},
                                                    Vec3d{0.50, +1.00, 0.1},
                                                    Vec3d{0.56, +0.80, 0.1}}});
    m_gllist.flow.begin[ExitDirection::EAST]
        = m_opengl.compile(XDraw{DrawType::LINE_STRIP,
                                 std::vector<Vec3d>{Vec3d{0.5, 0.5, 0.1}, Vec3d{1.0, 0.5, 0.1}}},
                           XDraw{DrawType::TRIANGLES,
                                 std::vector<Vec3d>{Vec3d{0.80, +0.44, 0.1},
                                                    Vec3d{1.00, +0.50, 0.1},
                                                    Vec3d{0.80, +0.56, 0.1}}});
    m_gllist.flow.begin[ExitDirection::WEST]
        = m_opengl.compile(XDraw{DrawType::LINE_STRIP,
                                 std::vector<Vec3d>{Vec3d{0.5, 0.5, 0.1}, Vec3d{0.0, 0.5, 0.1}}},
                           XDraw{DrawType::TRIANGLES,
                                 std::vector<Vec3d>{Vec3d{0.20, +0.44, 0.1},
                                                    Vec3d{0.00, +0.50, 0.1},
                                                    Vec3d{0.20, +0.56, 0.1}}});
    m_gllist.flow.begin[ExitDirection::UP]
        = m_opengl.compile(XDraw{DrawType::LINE_STRIP,
                                 std::vector<Vec3d>{Vec3d{0.5, 0.5, 0.1}, Vec3d{0.75, 0.25, 0.1}}},
                           XDraw{DrawType::TRIANGLES,
                                 std::vector<Vec3d>{Vec3d{0.51, 0.42, 0.1},
                                                    Vec3d{0.64, 0.37, 0.1},
                                                    Vec3d{0.60, 0.48, 0.1}}});
    m_gllist.flow.begin[ExitDirection::DOWN]
        = m_opengl.compile(XDraw{DrawType::LINE_STRIP,
                                 std::vector<Vec3d>{Vec3d{0.5, 0.5, 0.1}, Vec3d{0.25, 0.75, 0.1}}},
                           XDraw{DrawType::TRIANGLES,
                                 std::vector<Vec3d>{Vec3d{0.36, 0.57, 0.1},
                                                    Vec3d{0.33, 0.67, 0.1},
                                                    Vec3d{0.44, 0.63, 0.1}}});

    m_gllist.flow.end[ExitDirection::SOUTH] = m_opengl.compile(
        XDraw{DrawType::LINE_STRIP, std::vector<Vec3d>{Vec3d{0.0, 0.0, 0.1}, Vec3d{0.0, 0.5, 0.1}}});
    m_gllist.flow.end[ExitDirection::NORTH] = m_opengl.compile(
        XDraw{DrawType::LINE_STRIP,
              std::vector<Vec3d>{Vec3d{0.0, -0.5, 0.1}, Vec3d{0.0, 0.0, 0.1}}});
    m_gllist.flow.end[ExitDirection::WEST] = m_opengl.compile(
        XDraw{DrawType::LINE_STRIP,
              std::vector<Vec3d>{Vec3d{-0.5, 0.0, 0.1}, Vec3d{0.0, 0.0, 0.1}}});
    m_gllist.flow.end[ExitDirection::EAST] = m_opengl.compile(
        XDraw{DrawType::LINE_STRIP, std::vector<Vec3d>{Vec3d{0.5, 0.0, 0.1}, Vec3d{0.0, 0.0, 0.1}}});
    m_gllist.flow.end[ExitDirection::DOWN] = m_opengl.compile(
        XDraw{DrawType::LINE_STRIP,
              std::vector<Vec3d>{Vec3d{-0.25, 0.25, 0.1}, Vec3d{0.0, 0.0, 0.1}}});
    m_gllist.flow.end[ExitDirection::UP] = m_opengl.compile(
        XDraw{DrawType::LINE_STRIP,
              std::vector<Vec3d>{Vec3d{0.25, -0.25, 0.1}, Vec3d{0.0, 0.0, 0.1}}});
}

float MapCanvas::getDW() const
{
    return (static_cast<float>(width()) / static_cast<float>(BASESIZEX) / 12.0f / m_scaleFactor);
}

float MapCanvas::getDH() const
{
    return (static_cast<float>(height()) / static_cast<float>(BASESIZEY) / 12.0f / m_scaleFactor);
}

/* Direct means it is always called from the emitter's thread */
void MapCanvas::slot_onMessageLoggedDirect(const QOpenGLDebugMessage &message)
{
    using Type = QOpenGLDebugMessage::Type;
    switch (message.type()) {
    case Type::InvalidType:
    case Type::ErrorType:
    case Type::UndefinedBehaviorType:
        break;
    case Type::DeprecatedBehaviorType:
    case Type::PortabilityType:
    case Type::PerformanceType:
    case Type::OtherType:
    case Type::MarkerType:
    case Type::GroupPushType:
    case Type::GroupPopType:
    case Type::AnyType:
        qWarning() << message;
        return;
    }

    qCritical() << message; // TODO: consider crashing
}
