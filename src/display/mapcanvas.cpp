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
#include <QGestureEvent>
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
#include "Filenames.h"
#include "InfoMarkSelection.h"
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
        qWarning() << "failed to create: " << name;
        texture->setSize(1);
        texture->create();

        if (!texture->isCreated())
            throw std::runtime_error(("failed to create: " + name).toStdString());
    }

    return texture;
}

static auto loadPixmap(const char *const name, const uint i)
{
    if (name == nullptr)
        throw NullPointerException();
    return loadTexture(QString::asprintf(":/pixmaps/%s%u.png", name, i));
}

template<typename E, size_t N>
static void loadPixmapArray(EnumIndexedArray<QOpenGLTexture *, E, N> &textures)
{
    for (uint i = 0u; i < N; ++i) {
        const auto x = static_cast<E>(i);
        textures[x] = loadTexture(getPixmapFilename(x));
    }
}

template<RoadIndexType Type>
static void loadPixmapArray(road_texture_array<Type> &textures)
{
    const auto N = textures.size();
    for (uint i = 0u; i < N; ++i) {
        const auto x = TaggedRoadIndex<Type>{static_cast<RoadIndex>(i)};
        textures[x] = loadTexture(getPixmapFilename(x));
    }
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
    grabGesture(Qt::PinchGesture);

    m_opengl.initFont(static_cast<QPaintDevice *>(this));

    setContextMenuPolicy(Qt::CustomContextMenu);

    const auto getAaSamples = []() {
        int samples = getConfig().canvas.antialiasingSamples;
        if (samples <= 0) {
            samples = 2; // Default to 2 samples to prevent restart
        }
        return samples;
    };
    const int samples = getAaSamples();
    QSurfaceFormat format;
    format.setVersion(1, 0);
    format.setSamples(samples);
    setFormat(format);
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

    m_roomSelection = SigRoomSelection{};

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

int inline MapCanvas::GLtoMap(float arg)
{
    if (arg >= 0) {
        return static_cast<int>(arg + 0.5f);
    }
    return static_cast<int>(arg - 0.5f);
}

void MapCanvas::setCanvasMouseMode(CanvasMouseMode mode)
{
    m_canvasMouseMode = mode;

    clearRoomSelection();
    clearConnectionSelection();
    clearInfoMarkSelection();

    switch (m_canvasMouseMode) {
    case CanvasMouseMode::MOVE:
        setCursor(Qt::OpenHandCursor);
        break;

    default:
    case CanvasMouseMode::NONE:
    case CanvasMouseMode::SELECT_CONNECTIONS:
    case CanvasMouseMode::CREATE_INFOMARKS:
        setCursor(Qt::CrossCursor);
        break;

    case CanvasMouseMode::SELECT_ROOMS:
    case CanvasMouseMode::CREATE_ROOMS:
    case CanvasMouseMode::CREATE_CONNECTIONS:
    case CanvasMouseMode::CREATE_ONEWAY_CONNECTIONS:
    case CanvasMouseMode::SELECT_INFOMARKS:
        setCursor(Qt::ArrowCursor);
        break;
    }

    m_selectedArea = false;
    update();
}

void MapCanvas::setRoomSelection(const SigRoomSelection &selection)
{
    m_roomSelection = selection;

    if (m_roomSelection.isValid())
        qDebug() << "Updated selection with" << m_roomSelection.getShared()->size() << "rooms";
    else
        qDebug() << "Cleared room selection";

    // Let the MainWindow know
    emit newRoomSelection(m_roomSelection);
    update();
}

void MapCanvas::setConnectionSelection(ConnectionSelection *selection)
{
    delete m_connectionSelection;
    m_connectionSelection = selection;
    emit newConnectionSelection(m_connectionSelection);
    update();
}

void MapCanvas::setInfoMarkSelection(InfoMarkSelection *selection)
{
    delete m_infoMarkSelection;
    m_infoMarkSelection = nullptr;

    if (selection && m_canvasMouseMode == CanvasMouseMode::CREATE_INFOMARKS) {
        qDebug() << "Creating new infomark";
    } else if (selection && !selection->isEmpty()) {
        qDebug() << "Updated selection with" << selection->size() << "infomarks";
    } else {
        qDebug() << "Cleared infomark selection";
        delete selection;
        selection = nullptr;
    }

    m_infoMarkSelection = selection;
    emit newInfoMarkSelection(m_infoMarkSelection);
    update();
}

static uint32_t operator&(const Qt::KeyboardModifiers left, const Qt::Modifier right)
{
    return static_cast<uint32_t>(left) & static_cast<uint32_t>(right);
}

void MapCanvas::wheelEvent(QWheelEvent *const event)
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
        case CanvasMouseMode::SELECT_INFOMARKS:
        case CanvasMouseMode::CREATE_INFOMARKS:
        case CanvasMouseMode::SELECT_ROOMS:
        case CanvasMouseMode::SELECT_CONNECTIONS:
        case CanvasMouseMode::CREATE_ROOMS:
        case CanvasMouseMode::CREATE_CONNECTIONS:
        case CanvasMouseMode::CREATE_ONEWAY_CONNECTIONS:
            layerDown();
            break;

        case CanvasMouseMode::NONE:
        default:
            break;
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
        case CanvasMouseMode::SELECT_INFOMARKS:
        case CanvasMouseMode::CREATE_INFOMARKS:
        case CanvasMouseMode::SELECT_ROOMS:
        case CanvasMouseMode::SELECT_CONNECTIONS:
        case CanvasMouseMode::CREATE_ROOMS:
        case CanvasMouseMode::CREATE_CONNECTIONS:
        case CanvasMouseMode::CREATE_ONEWAY_CONNECTIONS:
            layerUp();
            break;

        case CanvasMouseMode::NONE:
        default:
            break;
        }
    }
}

void MapCanvas::forceMapperToRoom()
{
    if (m_roomSelection.isValid()) {
        emit newRoomSelection(m_roomSelection);
    } else {
        m_roomSelection = m_data->select(
            Coordinate(GLtoMap(m_sel1.x), GLtoMap(m_sel1.y), m_sel1.layer));
    }
    if (m_roomSelection.getShared()->size() == 1) {
        const RoomId id = m_roomSelection.getShared()->values().front()->getId();
        // Force update rooms only if we're in play or mapping mode
        const bool update = getConfig().general.mapMode != MapMode::OFFLINE;
        emit setCurrentRoom(id, update);
    }
    update();
}

bool MapCanvas::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture) {
        QGestureEvent *gestureEvent = static_cast<QGestureEvent *>(event);
        // Zoom in / out
        if (QGesture *gesture = gestureEvent->gesture(Qt::PinchGesture)) {
            QPinchGesture *pinch = static_cast<QPinchGesture *>(gesture);
            QPinchGesture::ChangeFlags changeFlags = pinch->changeFlags();
            if (changeFlags & QPinchGesture::ScaleFactorChanged) {
                const auto candidateStep = static_cast<float>(pinch->totalScaleFactor());
                const auto candidateScaleFactor = m_scaleFactor * candidateStep;
                if (candidateScaleFactor <= 2.0f && candidateScaleFactor >= 0.04f) {
                    m_currentStepScaleFactor = candidateStep;
                }
            }
            if (pinch->state() == Qt::GestureFinished) {
                m_scaleFactor *= m_currentStepScaleFactor;
                m_currentStepScaleFactor = 1.0f;
            }
            resizeGL(width(), height());
            return true;
        }
    }
    return QWidget::event(event);
}

void MapCanvas::createRoom()
{
    m_roomSelection = m_data->select(Coordinate(GLtoMap(m_sel1.x), GLtoMap(m_sel1.y), m_sel1.layer));
    if (m_roomSelection.getShared()->isEmpty()) {
        Coordinate c = Coordinate(GLtoMap(m_sel1.x), GLtoMap(m_sel1.y), m_currentLayer);
        m_data->createEmptyRoom(c);
    }
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

    m_mouseLeftPressed = (event->buttons() & Qt::LeftButton) != 0u;
    m_mouseRightPressed = (event->buttons() & Qt::RightButton) != 0u;

    if (!m_mouseLeftPressed && m_mouseRightPressed) {
        if (m_canvasMouseMode == CanvasMouseMode::MOVE) {
            // Select infomarks under the cursor
            Coordinate infoCoord = Coordinate(static_cast<int>(m_sel1.x * INFOMARK_SCALE),
                                              static_cast<int>(m_sel1.y * INFOMARK_SCALE),
                                              m_sel1.layer);
            InfoMarkSelection *tmpSel = new InfoMarkSelection(m_data, infoCoord);
            setInfoMarkSelection(tmpSel);

            if (m_infoMarkSelection == nullptr) {
                // Select the room under the cursor
                Coordinate c = Coordinate(GLtoMap(m_sel1.x), GLtoMap(m_sel1.y), m_sel1.layer);
                m_roomSelection = m_data->select(c);
                emit newRoomSelection(m_roomSelection);
            }

            update();
        }
        m_mouseRightPressed = false;
        event->accept();
        return;
    }

    switch (m_canvasMouseMode) {
    case CanvasMouseMode::CREATE_INFOMARKS:
        update();
        break;
    case CanvasMouseMode::SELECT_INFOMARKS:
        // Select infomarks
        if ((event->buttons() & Qt::LeftButton) != 0u) {
            const auto c1 = Coordinate(static_cast<int>(m_sel1.x * INFOMARK_SCALE),
                                       static_cast<int>(m_sel1.y * INFOMARK_SCALE),
                                       m_sel1.layer);
            InfoMarkSelection tmpSel(m_data, c1);
            if (m_infoMarkSelection != nullptr && !tmpSel.isEmpty()
                && m_infoMarkSelection->contains(tmpSel.front())) {
                m_infoMarkSelectionMove.inUse = true;
                m_infoMarkSelectionMove.x = 0;
                m_infoMarkSelectionMove.y = 0;

            } else {
                m_selectedArea = false;
                m_infoMarkSelectionMove.inUse = false;
            }
        }
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
            clearRoomSelection();
            m_ctrlPressed = true;
            m_altPressed = true;
            forceMapperToRoom();
            break;
        }
        // Cancel
        if ((event->buttons() & Qt::RightButton) != 0u) {
            m_selectedArea = false;
            clearRoomSelection();
        }
        // Select rooms
        if ((event->buttons() & Qt::LeftButton) != 0u) {
            if (event->modifiers() != Qt::CTRL) {
                SigRoomSelection tmpSel = m_data->select(
                    Coordinate(GLtoMap(m_sel1.x), GLtoMap(m_sel1.y), m_sel1.layer));
                if ((m_roomSelection.isValid()) && !tmpSel.getShared()->empty()
                    && m_roomSelection.getShared()->contains(tmpSel.getShared()->keys().front())) {
                    m_roomSelectionMove.inUse = true;
                    m_roomSelectionMove.x = 0;
                    m_roomSelectionMove.y = 0;
                    m_roomSelectionMove.wrongPlace = false;
                } else {
                    m_roomSelectionMove.inUse = false;
                    m_selectedArea = false;
                    clearRoomSelection();
                }
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

    case CanvasMouseMode::NONE:
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
    case CanvasMouseMode::SELECT_INFOMARKS:
        if ((event->buttons() & Qt::LeftButton) != 0u) {
            if (m_infoMarkSelectionMove.inUse) {
                m_infoMarkSelectionMove.x = m_sel2.x - m_sel1.x;
                m_infoMarkSelectionMove.y = m_sel2.y - m_sel1.y;
                setCursor(Qt::ClosedHandCursor);

            } else {
                m_selectedArea = true;
            }
        }
        update();
        break;
    case CanvasMouseMode::CREATE_INFOMARKS:
        if ((event->buttons() & Qt::LeftButton) != 0u) {
            m_selectedArea = true;
        }
        update();
        break;
    case CanvasMouseMode::MOVE:
        if (((event->buttons() & Qt::LeftButton) != 0u) && m_mouseLeftPressed) {
            const float scrollfactor = SCROLLFACTOR();
            auto idx = static_cast<int>((m_sel2.x - m_moveBackup.x) / scrollfactor);
            auto idy = static_cast<int>((m_sel2.y - m_moveBackup.y) / scrollfactor);

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

                if (r1 != nullptr && r2 != nullptr) {
                    if (!(r1->exit(dir1).containsOut(r2->getId()))
                        || !(r2->exit(dir2).containsOut(r1->getId()))) { // not two ways
                        if (dir2 != ExitDirection::UNKNOWN) {
                            m_connectionSelection->removeSecond();
                        } else if (dir2 == ExitDirection::UNKNOWN
                                   && (!(r1->exit(dir1).containsOut(r2->getId()))
                                       || (r1->exit(dir1).containsIn(r2->getId())))) { // not oneway
                            m_connectionSelection->removeSecond();
                        }
                    }
                }
                update();
            }
        }
        break;

    case CanvasMouseMode::CREATE_ROOMS:
    case CanvasMouseMode::NONE:
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
    case CanvasMouseMode::SELECT_INFOMARKS:
        setCursor(Qt::ArrowCursor);
        if (m_mouseLeftPressed) {
            m_mouseLeftPressed = false;
            if (m_infoMarkSelectionMove.inUse) {
                m_infoMarkSelectionMove.inUse = false;
                if (m_infoMarkSelection != nullptr) {
                    // Update infomark location
                    for (auto mark : *m_infoMarkSelection) {
                        Coordinate c1(mark->getPosition1().x
                                          + static_cast<int>(m_infoMarkSelectionMove.x
                                                             * INFOMARK_SCALE),
                                      mark->getPosition1().y
                                          + static_cast<int>(m_infoMarkSelectionMove.y
                                                             * INFOMARK_SCALE),
                                      mark->getPosition1().z);
                        mark->setPosition1(c1);
                        Coordinate c2(mark->getPosition2().x
                                          + static_cast<int>(m_infoMarkSelectionMove.x
                                                             * INFOMARK_SCALE),
                                      mark->getPosition2().y
                                          + static_cast<int>(m_infoMarkSelectionMove.y
                                                             * INFOMARK_SCALE),
                                      mark->getPosition2().z);
                        mark->setPosition2(c2);
                    }
                }
            } else {
                // Add infomarks to selection
                const auto c1 = Coordinate(static_cast<int>(m_sel1.x * INFOMARK_SCALE),
                                           static_cast<int>(m_sel1.y * INFOMARK_SCALE),
                                           m_sel1.layer);
                const auto c2 = Coordinate(static_cast<int>(m_sel2.x * INFOMARK_SCALE),
                                           static_cast<int>(m_sel2.y * INFOMARK_SCALE),
                                           m_sel2.layer);
                InfoMarkSelection *tmpSel = new InfoMarkSelection(m_data, c1, c2);
                if (tmpSel && tmpSel->size() == 1) {
                    QString ctemp = QString("Selected Info Mark: %1 %2")
                                        .arg(tmpSel->front()->getName())
                                        .arg(tmpSel->front()->getText());
                    emit log("MapCanvas", ctemp);
                }
                setInfoMarkSelection(tmpSel);
            }
            m_selectedArea = false;
        }
        update();
        break;
    case CanvasMouseMode::CREATE_INFOMARKS:
        if (m_mouseLeftPressed) {
            m_mouseLeftPressed = false;
            // Add infomarks to selection
            const auto c1 = Coordinate(static_cast<int>(m_sel1.x * INFOMARK_SCALE),
                                       static_cast<int>(m_sel1.y * INFOMARK_SCALE),
                                       m_sel1.layer);
            const auto c2 = Coordinate(static_cast<int>(m_sel2.x * INFOMARK_SCALE),
                                       static_cast<int>(m_sel2.y * INFOMARK_SCALE),
                                       m_sel2.layer);
            InfoMarkSelection *tmpSel = new InfoMarkSelection(m_data, c1, c2, 0);
            tmpSel->clear(); // REVISIT: Should creation workflow require the selection to be empty?
            setInfoMarkSelection(tmpSel);
        }
        update();
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
                    m_data->execute(new GroupMapAction(new MoveRelative(moverel), m_roomSelection),
                                    m_roomSelection);
                }
            } else {
                if (!m_roomSelection.isValid()) {
                    // add rooms to default selections
                    m_roomSelection = m_data->select(Coordinate(GLtoMap(m_sel1.x),
                                                                GLtoMap(m_sel1.y),
                                                                m_sel1.layer),
                                                     Coordinate(GLtoMap(m_sel2.x),
                                                                GLtoMap(m_sel2.y),
                                                                m_sel2.layer));
                } else {
                    // add or remove rooms to/from default selection
                    SigRoomSelection tmpSel = m_data->select(Coordinate(GLtoMap(m_sel1.x),
                                                                        GLtoMap(m_sel1.y),
                                                                        m_sel1.layer),
                                                             Coordinate(GLtoMap(m_sel2.x),
                                                                        GLtoMap(m_sel2.y),
                                                                        m_sel2.layer));
                    QList<RoomId> keys = tmpSel.getShared()->keys();
                    for (RoomId key : keys) {
                        if (m_roomSelection.getShared()->contains(key)) {
                            m_data->unselectRoom(key, m_roomSelection);
                        } else {
                            m_data->getRoom(key, m_roomSelection);
                        }
                    }
                }

                if (m_roomSelection.getShared()->empty()) {
                    clearRoomSelection();
                } else {
                    if (m_roomSelection.getShared()->size() == 1) {
                        const Room *r = m_roomSelection.getShared()->values().front();
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

                if (r1 != nullptr && r2 != nullptr) {
                    SigRoomSelection tmpSel = m_data->select();
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

                if (r1 != nullptr && r2 != nullptr) {
                    if (!(r1->exit(dir1).containsOut(r2->getId()))
                        || !(r2->exit(dir2).containsOut(r1->getId()))) {
                        if (dir2 != ExitDirection::UNKNOWN) {
                            delete m_connectionSelection;
                            m_connectionSelection = nullptr;
                        } else if (dir2 == ExitDirection::UNKNOWN
                                   && (!(r1->exit(dir1).containsOut(r2->getId()))
                                       || (r1->exit(dir1).containsIn(r2->getId())))) { // not oneway
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

    case CanvasMouseMode::NONE:
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

void MapCanvas::zoomReset()
{
    m_scaleFactor = 1.0f;
    resizeGL(width(), height());
}

void MapCanvas::initializeGL()
{
    m_opengl.initializeOpenGLFunctions();
    const auto getString = [this](const GLint id) -> QByteArray {
        const unsigned char *s = m_opengl.glGetString(static_cast<GLenum>(id));
        return QByteArray{reinterpret_cast<const char *>(s)};
    };

    const auto version = getString(GL_VERSION);
    const auto renderer = getString(GL_RENDERER);
    const auto vendor = getString(GL_VENDOR);
    const auto glSlVersion = getString(GL_SHADING_LANGUAGE_VERSION);
    qInfo() << "OpenGL Version: " << version;
    qInfo() << "OpenGL Renderer: " << renderer;
    qInfo() << "OpenGL Vendor: " << vendor;
    qInfo() << "OpenGL GLSL: " << glSlVersion;
    emit log("MapCanvas", "OpenGL Version: " + version);
    emit log("MapCanvas", "OpenGL Renderer: " + renderer);
    emit log("MapCanvas", "OpenGL Vendor: " + vendor);
    emit log("MapCanvas", "OpenGL GLSL: " + glSlVersion);

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

    if (getConfig().canvas.antialiasingSamples > 0) {
        m_opengl.apply(XEnable{XOption::MULTISAMPLE});
    }

    initTextures();

    // <= OpenGL 3.0
    makeGlLists(); // TODO(nschimme): Convert these GlLists into shaders
    m_opengl.glShadeModel(static_cast<GLenum>(GL_FLAT));
    m_opengl.glPolygonStipple(getStipple(StippleType::HalfTone));

    // >= OpenGL 3.0
    m_opengl.apply(XEnable{XOption::DEPTH_TEST});
    m_opengl.apply(XEnable{XOption::NORMALIZE});
    m_opengl.glBlendFunc(static_cast<GLenum>(GL_SRC_ALPHA),
                         static_cast<GLenum>(GL_ONE_MINUS_SRC_ALPHA));
}

void MapCanvas::resizeGL(int width, int height)
{
    if (m_textures.update == nullptr) {
        // resizeGL called but initializeGL was not called yet
        return;
    }

    const float swp = m_scaleFactor * m_currentStepScaleFactor
                      * (1.0f - (static_cast<float>(width - BASESIZEX) / static_cast<float>(width)));
    const float shp = m_scaleFactor * m_currentStepScaleFactor
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
    // emit onEnsureVisible(c.x, c.y);
}

void MapCanvas::drawGroupCharacters()
{
    CGroup *const group = m_groupManager->getGroup();
    if ((group == nullptr) || getConfig().groupManager.state == GroupManagerState::Off
        || m_data->isEmpty()) {
        return;
    }

    GroupSelection *const selection = group->selectAll();
    for (auto &character : *selection) {
        const RoomId id = character->getPosition();
        // Do not draw the character if they're in an "Unknown" room
        if (id == DEFAULT_ROOMID || id == INVALID_ROOMID)
            continue;
        if (character->getName() != getConfig().groupManager.charName) {
            SigRoomSelection roomSelection = m_data->select();
            if (const Room *const r = m_data->getRoom(id, roomSelection)) {
                drawCharacter(r->getPosition(), character->getColor());
            }
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
    m_opengl.apply(XColor4d{Qt::black, 0.4});
    m_opengl.apply(XEnable{XOption::BLEND});
    m_opengl.apply(XDisable{XOption::DEPTH_TEST});

    if (((static_cast<float>(x) < m_visible1.x - 1.0f)
         || (static_cast<float>(x) > m_visible2.x + 1.0f))
        || ((static_cast<float>(y) < m_visible1.y - 1.0f)
            || (static_cast<float>(y) > m_visible2.y + 1.0f))) {
        // Player is distant
        const double cameraCenterX = static_cast<double>(m_visible1.x + m_visible2.x) / 2.0;
        const double cameraCenterY = static_cast<double>(m_visible1.y + m_visible2.y) / 2.0;

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
    const auto backgroundColor = getConfig().canvas.backgroundColor;
    m_opengl.glClearColor(static_cast<float>(backgroundColor.redF()),
                          static_cast<float>(backgroundColor.greenF()),
                          static_cast<float>(backgroundColor.blueF()),
                          static_cast<float>(backgroundColor.alphaF()));

    m_opengl.glClear(static_cast<GLbitfield>(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    MapCanvasRoomDrawer drawer{*static_cast<MapCanvasData *>(this), this->m_opengl};

    if (!m_data->isEmpty()) {
        drawRooms(drawer);

    } else {
        drawer.renderText(static_cast<double>(m_visible1.x + m_visible2.x) / 2,
                          static_cast<double>(m_visible1.y + m_visible2.y) / 2,
                          "No map loaded");
    }

    GLdouble len = 0.2;
    if (m_selectedArea) {
        m_opengl.apply(XEnable{XOption::BLEND});
        m_opengl.apply(XDisable{XOption::DEPTH_TEST});
        m_opengl.apply(XColor4d{Qt::black, 0.5});
        m_opengl
            .draw(DrawType::POLYGON,
                  std::vector<Vec3d>{
                      Vec3d{static_cast<double>(m_sel1.x), static_cast<double>(m_sel1.y), 0.005},
                      Vec3d{static_cast<double>(m_sel2.x), static_cast<double>(m_sel1.y), 0.005},
                      Vec3d{static_cast<double>(m_sel2.x), static_cast<double>(m_sel2.y), 0.005},
                      Vec3d{static_cast<double>(m_sel1.x), static_cast<double>(m_sel2.y), 0.005}});

        m_opengl.apply(XColor4d{(Qt::white)});
        m_opengl.apply(LineStippleType::FOUR);
        m_opengl.apply(XEnable{XOption::LINE_STIPPLE});
        m_opengl
            .draw(DrawType::LINE_LOOP,
                  std::vector<Vec3d>{
                      Vec3d{static_cast<double>(m_sel1.x), static_cast<double>(m_sel1.y), 0.005},
                      Vec3d{static_cast<double>(m_sel2.x), static_cast<double>(m_sel1.y), 0.005},
                      Vec3d{static_cast<double>(m_sel2.x), static_cast<double>(m_sel2.y), 0.005},
                      Vec3d{static_cast<double>(m_sel1.x), static_cast<double>(m_sel2.y), 0.005}});
        m_opengl.apply(XDisable{XOption::LINE_STIPPLE});
        m_opengl.apply(XDisable{XOption::BLEND});
        m_opengl.apply(XEnable{XOption::DEPTH_TEST});
    }

    // Draw yellow guide when creating an infomark line/arrow
    if (m_canvasMouseMode == CanvasMouseMode::CREATE_INFOMARKS && m_selectedArea) {
        m_opengl.apply(XColor4d{Qt::yellow, 1.0});
        m_opengl.apply(XDevicePointSize{3.0});
        m_opengl.apply(XDeviceLineWidth{3.0});

        m_opengl.draw(DrawType::LINES,
                      std::vector<Vec3d>{
                          Vec3d{static_cast<double>(m_sel1.x), static_cast<double>(m_sel1.y), 0.005},
                          Vec3d{static_cast<double>(m_sel2.x), static_cast<double>(m_sel2.y), 0.005},
                      });
    }

    // paint selected connection
    paintSelectedConnection();

    // paint selection
    paintSelection(len);

    // paint selected infomarks
    paintSelectedInfoMarks();

    // draw the characters before the current position
    drawGroupCharacters();

    // paint char current position
    if (!m_data->isEmpty()) {
        // Use the player's selected color
        const QColor color = getConfig().groupManager.color;
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

    const auto wantInfoMarks = (m_scaleFactor * m_currentStepScaleFactor >= 0.25f);
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
    GLdouble x2p = static_cast<double>(m_sel2.x);
    GLdouble y2p = static_cast<double>(m_sel2.y);

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
    case ExitDirection::NONE:
    default:
        break;
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
        case ExitDirection::NONE:
        default:
            break;
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
    if (!m_roomSelection.isValid())
        return;

    QList<const Room *> rooms = m_roomSelection.getShared()->values();
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

    m_opengl.apply(XColor4d{Qt::black, 0.4});

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
            m_opengl.apply(XColor4d{Qt::red, 0.4});
        } else {
            m_opengl.apply(XColor4d{Qt::white, 0.4});
        }

        m_opengl.glTranslated(m_roomSelectionMove.x, m_roomSelectionMove.y, ROOM_Z_DISTANCE * layer);
        m_opengl.callList(m_gllist.room);
    }

    m_opengl.apply(XDisable{XOption::BLEND});
    m_opengl.apply(XEnable{XOption::DEPTH_TEST});

    m_opengl.glPopMatrix();
}

void MapCanvas::paintSelectedInfoMarks()
{
    if (m_infoMarkSelection == nullptr)
        return;

    for (const auto marker : *m_infoMarkSelection) {
        paintSelectedInfoMark(marker);
    }
}

void MapCanvas::paintSelectedInfoMark(const InfoMark *const marker)
{
    const qreal x1 = static_cast<double>(marker->getPosition1().x) / INFOMARK_SCALE;
    const qreal y1 = static_cast<double>(marker->getPosition1().y) / INFOMARK_SCALE;
    const qreal x2 = static_cast<double>(marker->getPosition2().x) / INFOMARK_SCALE;
    const qreal y2 = static_cast<double>(marker->getPosition2().y) / INFOMARK_SCALE;
    const qreal dx = x2 - x1;
    const qreal dy = y2 - y1;

    m_opengl.glPushMatrix();
    m_opengl.glTranslated(x1, y1, 0.0);
    m_opengl.apply(XColor4d{(Qt::red)});
    m_opengl.apply(XEnable{XOption::BLEND});
    m_opengl.apply(XDisable{XOption::DEPTH_TEST});

    const auto draw_info_mark = [this](const auto marker, const auto &dx, const auto &dy) {
        switch (marker->getType()) {
        case InfoMarkType::TEXT:
            m_opengl.draw(DrawType::LINE_LOOP,
                          std::vector<Vec3d>{
                              Vec3d{0.0, 0.0, 1.0},
                              Vec3d{0.0, 0.25 + dy, 1.0},
                              Vec3d{0.2 + dx, 0.25 + dy, 1.0},
                              Vec3d{0.2 + dx, 0.0, 1.0},
                          });
            break;
        case InfoMarkType::LINE:
            m_opengl.apply(XDevicePointSize{2.0});
            m_opengl.apply(XDeviceLineWidth{2.0});
            m_opengl.draw(DrawType::LINES,
                          std::vector<Vec3d>{
                              Vec3d{0.0, 0.0, 0.1},
                              Vec3d{dx, dy, 0.1},
                          });
            break;
        case InfoMarkType::ARROW:
            m_opengl.apply(XDevicePointSize{2.0});
            m_opengl.apply(XDeviceLineWidth{2.0});
            m_opengl.draw(DrawType::LINE_STRIP,
                          std::vector<Vec3d>{Vec3d{0.0, 0.05, 1.0},
                                             Vec3d{dx - 0.2, dy + 0.1, 1.0},
                                             Vec3d{dx - 0.1, dy + 0.1, 1.0}});
            m_opengl.draw(DrawType::LINE_STRIP,
                          std::vector<Vec3d>{Vec3d{dx - 0.1, dy + 0.1 - 0.07, 1.0},
                                             Vec3d{dx - 0.1, dy + 0.1 + 0.07, 1.0},
                                             Vec3d{dx + 0.1, dy + 0.1, 1.0}});
            break;
        }
    };

    draw_info_mark(marker, dx, dy);

    if (m_infoMarkSelectionMove.inUse) {
        m_opengl.glTranslated(static_cast<double>(m_infoMarkSelectionMove.x),
                              static_cast<double>(m_infoMarkSelectionMove.y),
                              0);
        draw_info_mark(marker, dx, dy);
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
    const QColor color = getConfig().groupManager.color;
    m_opengl.apply(XColor4d{color});
    m_opengl.apply(XEnable{XOption::BLEND});
    m_opengl.apply(XDevicePointSize{4.0});
    m_opengl.apply(XDeviceLineWidth{4.0});

    const double srcZ = ROOM_Z_DISTANCE * static_cast<double>(layer1) + 0.3;

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
    // qint32 z1 = sc.z;

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
    const auto wantTrilinear = getConfig().canvas.trilinearFiltering;

    loadPixmapArray(this->m_textures.terrain);
    loadPixmapArray(this->m_textures.road);
    loadPixmapArray(this->m_textures.trail);
    loadPixmapArray(this->m_textures.load);
    loadPixmapArray(this->m_textures.mob);
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
