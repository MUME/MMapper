// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mapcanvas.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include <QGestureEvent>
#include <QMatrix4x4>
#include <QMessageBox>
#include <QMessageLogContext>
#include <QOpenGLDebugMessage>
#include <QSet>
#include <QSize>
#include <QString>
#include <QtGui>

#include "../configuration/configuration.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/room.h"
#include "../global/EnumIndexedArray.h"
#include "../global/RAII.h"
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
#include "../parser/CommandQueue.h"
#include "../parser/ConnectedRoomFlags.h"
#include "../parser/ExitsFlags.h"
#include "../parser/PromptFlags.h"
#include "Filenames.h"
#include "InfoMarkSelection.h"
#include "MapCanvasData.h"
#include "MapCanvasRoomDrawer.h"
#include "OpenGL.h"
#include "RoadIndex.h"
#include "connectionselection.h"
#include "prespammedpath.h"

static std::unique_ptr<QOpenGLTexture> loadTexture(const QString &name)
{
    auto texture = std::unique_ptr<QOpenGLTexture>{new QOpenGLTexture{QImage{name}.mirrored()}};

    if (!texture->isCreated()) {
        qWarning() << "failed to create: " << name;
        texture->setSize(1);
        texture->create();

        if (!texture->isCreated())
            throw std::runtime_error(("failed to create: " + name).toStdString());
    }

    return texture;
}

template<typename E>
static void loadPixmapArray(texture_array<E> &textures)
{
    const auto N = textures.size();
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

MapCanvas::MapCanvas(MapData *const mapData,
                     PrespammedPath *const prespammedPath,
                     Mmapper2Group *const groupManager,
                     QWidget *const parent)
    : QOpenGLWidget(parent)
    , MapCanvasData(mapData, prespammedPath, static_cast<QWidget &>(*this))
    , m_groupManager(groupManager)
{
    setCursor(Qt::OpenHandCursor);
    grabGesture(Qt::PinchGesture);

    getOpenGL().initFont(static_cast<QPaintDevice *>(this));

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
    cleanupOpenGL();
}

class NODISCARD MakeCurrentRaii final
{
private:
    QOpenGLWidget &m_glWidget;

public:
    explicit MakeCurrentRaii(QOpenGLWidget &widget)
        : m_glWidget{widget}
    {
        m_glWidget.makeCurrent();
    }
    ~MakeCurrentRaii() { m_glWidget.doneCurrent(); }

    DELETE_CTORS_AND_ASSIGN_OPS(MakeCurrentRaii);
};

void MapCanvas::cleanupOpenGL()
{
    // Make sure the context is current and then explicitly
    // destroy all underlying OpenGL resources.
    MakeCurrentRaii makeCurrentRaii{*this};
    MapCanvasData::destroyAllGLObjects();
}

void MapCanvas::makeCurrentAndUpdate()
{
    // Minor semantic difference: previously we didn't call doneCurrent().
    MakeCurrentRaii makeCurrentRaii{*this};
    update();
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

void MapCanvas::layerReset()
{
    m_currentLayer = 0;
    update();
}

void MapCanvas::setCanvasMouseMode(const CanvasMouseMode mode)
{
    clearRoomSelection();
    clearConnectionSelection();
    clearInfoMarkSelection();

    switch (mode) {
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

    m_canvasMouseMode = mode;
    m_selectedArea = false;
    update();
}

void MapCanvas::setRoomSelection(const SigRoomSelection &selection)
{
    if (selection.isValid()) {
        m_roomSelection = selection.getShared();
        qDebug() << "Updated selection with" << m_roomSelection->size() << "rooms";
    } else {
        m_roomSelection.reset();
        qDebug() << "Cleared room selection";
    }

    // Let the MainWindow know
    emit newRoomSelection(selection);
    update();
}

void MapCanvas::setConnectionSelection(ConnectionSelection *const selection)
{
    delete std::exchange(m_connectionSelection, selection);
    emit newConnectionSelection(selection);
    update();
}

void MapCanvas::setInfoMarkSelection(InfoMarkSelection *selection)
{
    if (selection != nullptr && m_canvasMouseMode == CanvasMouseMode::CREATE_INFOMARKS) {
        qDebug() << "Creating new infomark";
    } else if (selection != nullptr && !selection->isEmpty()) {
        qDebug() << "Updated selection with" << selection->size() << "infomarks";
    } else {
        qDebug() << "Cleared infomark selection";
        delete std::exchange(selection, nullptr);
    }

    delete std::exchange(m_infoMarkSelection, selection);
    emit newInfoMarkSelection(selection);
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
        case CanvasMouseMode::SELECT_INFOMARKS:
        case CanvasMouseMode::CREATE_INFOMARKS:
        case CanvasMouseMode::SELECT_ROOMS:
        case CanvasMouseMode::SELECT_CONNECTIONS:
        case CanvasMouseMode::CREATE_ROOMS:
        case CanvasMouseMode::CREATE_CONNECTIONS:
        case CanvasMouseMode::CREATE_ONEWAY_CONNECTIONS:
            if ((event->modifiers() & Qt::CTRL) != 0u) {
                layerDown();
            } else {
                zoomIn();
            }
            break;

        case CanvasMouseMode::NONE:
        default:
            break;
        }
    }

    if (event->delta() < -100) {
        switch (m_canvasMouseMode) {
        case CanvasMouseMode::MOVE:
        case CanvasMouseMode::SELECT_INFOMARKS:
        case CanvasMouseMode::CREATE_INFOMARKS:
        case CanvasMouseMode::SELECT_ROOMS:
        case CanvasMouseMode::SELECT_CONNECTIONS:
        case CanvasMouseMode::CREATE_ROOMS:
        case CanvasMouseMode::CREATE_CONNECTIONS:
        case CanvasMouseMode::CREATE_ONEWAY_CONNECTIONS:
            if ((event->modifiers() & Qt::CTRL) != 0u) {
                layerUp();
            } else {
                zoomOut();
            }
            break;

        case CanvasMouseMode::NONE:
        default:
            break;
        }
    }
}

void MapCanvas::forceMapperToRoom()
{
    if (!m_roomSelection) {
        m_roomSelection = RoomSelection::createSelection(*m_data, m_sel1.getCoordinate());
        emit newRoomSelection(SigRoomSelection{m_roomSelection});
    }
    if (m_roomSelection->size() == 1) {
        const RoomId id = m_roomSelection->getFirstRoomId();
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
    const Coordinate c = m_sel1.getCoordinate();
    RoomSelection tmpSel = RoomSelection(*m_data, c);
    if (tmpSel.isEmpty()) {
        m_data->createEmptyRoom(Coordinate{c.x, c.y, m_currentLayer});
    }
    update();
}

void MapCanvas::mousePressEvent(QMouseEvent *const event)
{
    m_sel1 = m_sel2 = getUnprojectedMouseSel(event);

    m_mouseLeftPressed = (event->buttons() & Qt::LeftButton) != 0u;
    m_mouseRightPressed = (event->buttons() & Qt::RightButton) != 0u;

    if (!m_mouseLeftPressed && m_mouseRightPressed) {
        if (m_canvasMouseMode == CanvasMouseMode::MOVE) {
            m_roomSelection = RoomSelection::createSelection(*m_data, m_sel1.getCoordinate());
            setRoomSelection(SigRoomSelection{m_roomSelection});

            // Select infomarks under the cursor
            const Coordinate infoCoord = m_sel1.getScaledCoordinate(INFOMARK_SCALE);

            // TODO: use RAII to avoid leaking this allocation.
            setInfoMarkSelection(new InfoMarkSelection(m_data, infoCoord));
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
            const Coordinate c1 = m_sel1.getScaledCoordinate(INFOMARK_SCALE);
            InfoMarkSelection tmpSel(m_data, c1);
            if (m_infoMarkSelection != nullptr && !tmpSel.isEmpty()
                && m_infoMarkSelection->contains(tmpSel.front())) {
                m_infoMarkSelectionMove.inUse = true;
                m_infoMarkSelectionMove.pos = Coordinate2f{};

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

            // REVISIT: why doesn't this copy layer?
            m_moveBackup.pos = m_sel1.pos;
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
                const auto tmpSel = RoomSelection::createSelection(*m_data, m_sel1.getCoordinate());
                if ((m_roomSelection != nullptr) && !tmpSel->isEmpty()
                    && m_roomSelection->contains(tmpSel->begin().key())) {
                    m_roomSelectionMove.pos = Coordinate2i{};
                    m_roomSelectionMove.inUse = true;
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
            m_connectionSelection = new ConnectionSelection(m_data, m_sel1);
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
            m_connectionSelection = new ConnectionSelection(m_data, m_sel1);
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

    m_sel2 = getUnprojectedMouseSel(event);

    switch (m_canvasMouseMode) {
    case CanvasMouseMode::SELECT_INFOMARKS:
        if ((event->buttons() & Qt::LeftButton) != 0u) {
            if (m_infoMarkSelectionMove.inUse) {
                m_infoMarkSelectionMove.pos = m_sel2.pos - m_sel1.pos;
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
            const Coordinate2i pos = ((m_sel2.pos - m_moveBackup.pos) / scrollfactor).round();
            const auto idx = pos.x;
            const auto idy = pos.y;

            emit mapMove(-idx, -idy);

            if (idx != 0) {
                m_moveBackup.pos.x = m_sel2.pos.x - static_cast<float>(idx) * scrollfactor;
            }
            if (idy != 0) {
                m_moveBackup.pos.y = m_sel2.pos.y - static_cast<float>(idy) * scrollfactor;
            }
        }
        break;

    case CanvasMouseMode::SELECT_ROOMS:
        if ((event->buttons() & Qt::LeftButton) != 0u) {
            if (m_roomSelectionMove.inUse) {
                const auto diff = m_sel2.pos.round() - m_sel1.pos.round();
                const auto wrongPlace = !m_roomSelection->isMovable(Coordinate{diff, 0});

                m_roomSelectionMove.pos = diff;
                m_roomSelectionMove.wrongPlace = wrongPlace;

                setCursor(wrongPlace ? Qt::ForbiddenCursor : Qt::ClosedHandCursor);
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
                m_connectionSelection->setSecond(m_data, m_sel2);

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
                m_connectionSelection->setSecond(m_data, m_sel2);

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
    m_sel2 = getUnprojectedMouseSel(event);

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
                    const auto offset
                        = Coordinate{(m_infoMarkSelectionMove.pos * INFOMARK_SCALE).round(), 0};

                    // Update infomark location
                    for (auto mark : *m_infoMarkSelection) {
                        mark->setPosition1(mark->getPosition1() + offset);
                        mark->setPosition2(mark->getPosition2() + offset);
                    }
                }
            } else {
                // Add infomarks to selection
                const auto c1 = m_sel1.getScaledCoordinate(INFOMARK_SCALE);
                const auto c2 = m_sel2.getScaledCoordinate(INFOMARK_SCALE);
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
            const auto c1 = m_sel1.getScaledCoordinate(INFOMARK_SCALE);
            const auto c2 = m_sel2.getScaledCoordinate(INFOMARK_SCALE);
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
                    const Coordinate moverel(m_roomSelectionMove.pos, 0);
                    m_data->execute(new GroupMapAction(new MoveRelative(moverel), m_roomSelection),
                                    m_roomSelection);
                }
            } else {
                if (m_roomSelection == nullptr) {
                    // add rooms to default selections
                    m_roomSelection = RoomSelection::createSelection(*m_data,
                                                                     m_sel1.getCoordinate(),
                                                                     m_sel2.getCoordinate());
                } else {
                    // add or remove rooms to/from default selection
                    const auto tmpSel = RoomSelection(*m_data,
                                                      m_sel1.getCoordinate(),
                                                      m_sel2.getCoordinate());
                    for (const RoomId &key : tmpSel.keys()) {
                        if (m_roomSelection->contains(key)) {
                            m_roomSelection->unselect(key);
                        } else {
                            m_roomSelection->getRoom(key);
                        }
                    }
                }

                if (!m_roomSelection->empty()) {
                    emit newRoomSelection(SigRoomSelection{m_roomSelection});
                    if (m_roomSelection->size() == 1) {
                        const Room *const r = m_roomSelection->first();
                        const auto x = r->getPosition().x;
                        const auto y = r->getPosition().y;

                        // REVISIT: use a StringBuilder of some sort?
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
                m_connectionSelection = new ConnectionSelection(m_data, m_sel1);
            }
            m_connectionSelection->setSecond(m_data, m_sel2);

            if (!m_connectionSelection->isValid()) {
                delete m_connectionSelection;
                m_connectionSelection = nullptr;
            } else {
                const auto first = m_connectionSelection->getFirst();
                const auto second = m_connectionSelection->getSecond();
                const Room *const r1 = first.room;
                const Room *const r2 = second.room;

                if (r1 != nullptr && r2 != nullptr) {
                    const ExitDirection dir1 = first.direction;
                    const ExitDirection dir2 = second.direction;

                    const RoomId id1 = r1->getId();
                    const RoomId &id2 = r2->getId();

                    const auto tmpSel = RoomSelection::createSelection(*m_data);
                    tmpSel->getRoom(id1);
                    tmpSel->getRoom(id2);

                    delete std::exchange(m_connectionSelection, nullptr);

                    if (!(r1->exit(dir1).containsOut(id2)) || !(r2->exit(dir2).containsOut(id1))) {
                        if (m_canvasMouseMode != CanvasMouseMode::CREATE_ONEWAY_CONNECTIONS) {
                            m_data->execute(new AddTwoWayExit(id1, id2, dir1, dir2), tmpSel);
                        } else {
                            m_data->execute(new AddOneWayExit(id1, id2, dir1), tmpSel);
                        }
                        m_connectionSelection = new ConnectionSelection();
                        m_connectionSelection->setFirst(m_data, id1, dir1);
                        m_connectionSelection->setSecond(m_data, id2, dir2);
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
                m_connectionSelection = new ConnectionSelection(m_data, m_sel1);
            }
            m_connectionSelection->setSecond(m_data, m_sel2);

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
    auto &gl = getOpenGL();
    if (!gl.initializeOpenGLFunctions()) {
        qWarning() << "Unable to initialize OpenGL functions";
        if (!getConfig().canvas.softwareOpenGL) {
            setConfig().canvas.softwareOpenGL = true;
            setConfig().write();
            QMessageBox::critical(this,
                                  "OpenGL Error",
                                  "Please restart MMapper to enable software rendering");
        } else {
            QMessageBox::critical(this, "OpenGL Error", "Please upgrade your video card drivers");
        }
        return;
    }
    const auto getString = [&gl](const GLint id) -> QByteArray {
        const unsigned char *s = gl.glGetString(static_cast<GLenum>(id));
        return QByteArray{as_cstring(s)};
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

    if (getCurrentPlatform() == Platform::Windows && vendor == "Microsoft Corporation"
        && renderer == "GDI Generic") {
        setConfig().canvas.softwareOpenGL = true;
        setConfig().write();
        hide();
        doneCurrent();
        QMessageBox::critical(this,
                              "OpenGL Driver Blacklisted",
                              "Please restart MMapper to enable software rendering");
        return;
    }

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
        gl.apply(XEnable{XOption::MULTISAMPLE});
    }

    initTextures();

    // NOTE: setDevicePixelRatio() must be before makeGlLists()
    gl.setDevicePixelRatio(static_cast<float>(QPaintDevice::devicePixelRatioF()));

    // <= OpenGL 3.0
    makeGlLists(); // TODO(nschimme): Convert these GlLists into shaders
    gl.glShadeModel(static_cast<GLenum>(GL_FLAT));
    gl.glPolygonStipple(getStipple(StippleType::HalfTone));

    // >= OpenGL 3.0
    gl.apply(XEnable{XOption::DEPTH_TEST});
    gl.apply(XEnable{XOption::NORMALIZE});
    gl.glBlendFunc(static_cast<GLenum>(GL_SRC_ALPHA), static_cast<GLenum>(GL_ONE_MINUS_SRC_ALPHA));
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

    // Minor semantic difference: previously we didn't call doneCurrent().
    MakeCurrentRaii makeCurrentRaii{*this};
    auto &gl = getOpenGL();
    gl.glViewport(0, 0, width, height);

    // >= OpenGL 3.1
    m_projection.setToIdentity();
    m_projection.frustum(-0.5f, +0.5f, +0.5f, -0.5f, 5.0f, 80.0f);
    m_projection.scale(swp, shp, 1.0f);
    m_projection.translate(-SCROLLFACTOR() * static_cast<float>(m_scroll.x),
                           -SCROLLFACTOR() * static_cast<float>(m_scroll.y),
                           -60.0f);
    m_modelview.setToIdentity();

    // <= OpenGL 3.0
    gl.setMatrix(MatrixType::PROJECTION, m_projection);
    gl.setMatrix(MatrixType::MODELVIEW, m_modelview);

    QVector3D v1 = unproject(QVector3D(0.0f, static_cast<float>(height), CAMERA_Z_DISTANCE));
    m_visible1.x = v1.x();
    m_visible1.y = v1.y();
    QVector3D v2 = unproject(QVector3D(static_cast<float>(width), 0.0f, CAMERA_Z_DISTANCE));
    m_visible2.x = v2.x();
    m_visible2.y = v2.y();

    // Render
    update();
}

void MapCanvas::setTrilinear(const std::unique_ptr<QOpenGLTexture> &x) const
{
    if (x != nullptr)
        x->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
}

void MapCanvas::dataLoaded()
{
    m_currentLayer = static_cast<qint16>(m_data->getPosition().z);
    emit onCenter(m_data->getPosition().x, m_data->getPosition().y);
    makeCurrentAndUpdate();
}

void MapCanvas::moveMarker(const Coordinate &c)
{
    m_data->setPosition(c);
    m_currentLayer = static_cast<qint16>(c.z);
    makeCurrentAndUpdate();
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

    // Omit player so that they know group members are below them
    QSet<RoomId> drawnRoomIds;
    auto selection = group->selectAll();
    for (auto &character : *selection) {
        const RoomId id = character->getRoomId();
        // Do not draw the character if they're in an "Unknown" room
        if (id == DEFAULT_ROOMID || id == INVALID_ROOMID || id > m_data->getMaxId())
            continue;
        if (character->getName() != getConfig().groupManager.charName) {
            auto roomSelection = RoomSelection(*m_data);
            if (const Room *const r = roomSelection.getRoom(id)) {
                const auto pos = r->getPosition();
                const auto color = character->getColor();
                const bool fill = !drawnRoomIds.contains(r->getId());
                drawCharacter(pos, color, fill);
                const auto prespam = m_data->getPath(pos, character->prespam);
                drawPreSpammedPath(pos, prespam, color);
                drawnRoomIds.insert(r->getId());
            }
        }
    }
}

void MapCanvas::drawCharacter(const Coordinate &c, const QColor &color, bool fill)
{
    const float x = static_cast<float>(c.x);
    const float y = static_cast<float>(c.y);
    const qint32 layer = c.z - m_currentLayer;

    auto &gl = getOpenGL();
    gl.glPushMatrix();
    gl.apply(XColor4f{Qt::black, 0.4f});
    gl.apply(XEnable{XOption::BLEND});
    gl.apply(XDisable{XOption::DEPTH_TEST});

    if ((x < m_visible1.x) || (x > m_visible2.x) || (y < m_visible1.y) || (y > m_visible2.y)) {
        // Player is distant
        const float cameraCenterX = (m_visible1.x + m_visible2.x) / 2.0f;
        const float cameraCenterY = (m_visible1.y + m_visible2.y) / 2.0f;

        // Calculate degrees from camera center to character
        const float adjacent = cameraCenterY - y;
        const float opposite = cameraCenterX - x;
        const float radians = std::atan2(adjacent, opposite);
        const float degrees = radians * static_cast<float>(180.0 / M_PI);

        // Identify character hint coordinates using an elipse to represent the screen
        const auto radiusX = (m_visible2.x - m_visible1.x) / 2.0f - 0.75f;
        const auto radiusY = (m_visible2.y - m_visible1.y) / 2.0f - 0.75f;
        const float characterHintX = cameraCenterX + (std::cos(radians) * radiusX * -1);
        const float characterHintY = cameraCenterY + (std::sin(radians) * radiusY * -1);

        // Rotate according to angle
        gl.glTranslatef(characterHintX, characterHintY, m_currentLayer + 0.1f);
        gl.glRotatef(degrees, 0.0f, 0.0f, 1.0f);

        // Scale based upon normalized distance
        const float distance = std::sqrt((adjacent * adjacent) + (opposite * opposite));
        const float normalized = 1.0f - (std::min(distance, BASESIZEX * 3.0f) / BASESIZEX * 3.0f);
        const float scaleFactor = std::max(0.3f, normalized);
        gl.glScalef(scaleFactor, scaleFactor, 1.0f);

        if (fill)
            gl.callList(m_gllist.character_hint.filled);
        gl.apply(XDisable{XOption::BLEND});

        gl.apply(XColor4f{color});
        gl.callList(m_gllist.character_hint.outline);
    } else if (layer != 0) {
        // Player is not on the same layer
        gl.glTranslatef(x, y - 0.5f, m_currentLayer + 0.1f);
        gl.glRotatef(270.0f, 0.0f, 0.0f, 1.0f);

        if (fill)
            gl.callList(m_gllist.character_hint.filled);
        gl.apply(XDisable{XOption::BLEND});

        gl.apply(XColor4f{color});
        gl.callList(m_gllist.character_hint.outline);
    } else {
        // Player is on the same layer and visible
        gl.glTranslatef(x - 0.5f, y - 0.5f, ROOM_Z_DISTANCE * layer + 0.1f);

        if (fill)
            gl.callList(m_gllist.room_selection.filled);
        gl.apply(XDisable{XOption::BLEND});

        gl.apply(XColor4f{color});
        gl.callList(m_gllist.room_selection.outline);
    }
    gl.apply(XEnable{XOption::DEPTH_TEST});
    gl.glPopMatrix();
}

void MapCanvas::paintGL()
{
    // Background Color
    const auto backgroundColor = getConfig().canvas.backgroundColor;
    auto &gl = getOpenGL();
    gl.glClearColor(static_cast<float>(backgroundColor.redF()),
                    static_cast<float>(backgroundColor.greenF()),
                    static_cast<float>(backgroundColor.blueF()),
                    static_cast<float>(backgroundColor.alphaF()));

    gl.glClear(static_cast<GLbitfield>(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    MapCanvasRoomDrawer drawer{*static_cast<MapCanvasData *>(this), gl};

    if (m_data->isEmpty()) {
        drawer.renderText((m_visible1.x + m_visible2.x) / 2.0f,
                          (m_visible1.y + m_visible2.y) / 2.0f,
                          "No map loaded");
    } else {
        drawRooms(drawer);
    }

    paintSelectedRooms();

    // paint selected connection
    paintSelectedConnection();

    // paint selection
    paintSelection();

    // paint selected infomarks
    paintSelectedInfoMarks();

    if (!m_data->isEmpty()) {
        // draw the characters before the current position
        drawGroupCharacters();

        // paint char current position
        const QColor color = getConfig().groupManager.color;
        drawCharacter(m_data->getPosition(), color);

        // paint prespam
        const auto prespam = m_data->getPath(m_data->getPosition(), m_prespammedPath->getQueue());
        drawPreSpammedPath(m_data->getPosition(), prespam, color);
    }
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

    GLfloat x1p = r->getPosition().x;
    GLfloat y1p = r->getPosition().y;
    GLfloat x2p = m_sel2.pos.x;
    GLfloat y2p = m_sel2.pos.y;

    /* TODO: factor duplicate code using vec2 return value */
    switch (m_connectionSelection->getFirst().direction) {
    case ExitDirection::NORTH:
        y1p -= 0.4f;
        break;
    case ExitDirection::SOUTH:
        y1p += 0.4f;
        break;
    case ExitDirection::EAST:
        x1p += 0.4f;
        break;
    case ExitDirection::WEST:
        x1p -= 0.4f;
        break;
    case ExitDirection::UP:
        x1p += 0.3f;
        y1p -= 0.3f;
        break;
    case ExitDirection::DOWN:
        x1p -= 0.3f;
        y1p += 0.3f;
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
            y2p -= 0.4f;
            break;
        case ExitDirection::SOUTH:
            y2p += 0.4f;
            break;
        case ExitDirection::EAST:
            x2p += 0.4f;
            break;
        case ExitDirection::WEST:
            x2p -= 0.4f;
            break;
        case ExitDirection::UP:
            x2p += 0.3f;
            y2p -= 0.3f;
            break;
        case ExitDirection::DOWN:
            x2p -= 0.3f;
            y2p += 0.3f;
            break;
        case ExitDirection::UNKNOWN:
        case ExitDirection::NONE:
        default:
            break;
        }
    }

    auto &gl = getOpenGL();
    gl.apply(XColor4f{(Qt::red)});
    gl.apply(XDevicePointSize{10.0});
    gl.draw(DrawType::POINTS,
            std::vector<Vec3f>{
                Vec3f{x1p, y1p, 0.005f},
                Vec3f{x2p, y2p, 0.005f},
            });
    gl.apply(XDevicePointSize{1.0f});

    gl.draw(DrawType::LINES,
            std::vector<Vec3f>{
                Vec3f{x1p, y1p, 0.005f},
                Vec3f{x2p, y2p, 0.005f},
            });
    gl.apply(XDisable{XOption::BLEND});
}

void MapCanvas::paintSelection()
{
    // Mouse selected area
    auto &gl = getOpenGL();
    if (m_selectedArea) {
        gl.apply(XEnable{XOption::BLEND});
        gl.apply(XDisable{XOption::DEPTH_TEST});
        gl.apply(XColor4f{Qt::black, 0.5f});
        const auto x1 = m_sel1.pos.x;
        const auto y1 = m_sel1.pos.y;
        const auto x2 = m_sel2.pos.x;
        const auto y2 = m_sel2.pos.y;
        gl.draw(DrawType::TRIANGLE_STRIP,
                std::vector<Vec3f>{Vec3f{x1, y1, 0.005f},
                                   Vec3f{x2, y1, 0.005f},
                                   Vec3f{x1, y2, 0.005f},
                                   Vec3f{x2, y2, 0.005f}});

        gl.apply(XColor4f{(Qt::white)});
        gl.apply(LineStippleType::FOUR);
        gl.apply(XEnable{XOption::LINE_STIPPLE});
        gl.draw(DrawType::LINE_LOOP,
                std::vector<Vec3f>{Vec3f{x1, y1, 0.005f},
                                   Vec3f{x2, y1, 0.005f},
                                   Vec3f{x2, y2, 0.005f},
                                   Vec3f{x1, y2, 0.005f}});
        gl.apply(XDisable{XOption::LINE_STIPPLE});
        gl.apply(XDisable{XOption::BLEND});
        gl.apply(XEnable{XOption::DEPTH_TEST});
    }

    // Draw yellow guide when creating an infomark line/arrow
    if (m_canvasMouseMode == CanvasMouseMode::CREATE_INFOMARKS && m_selectedArea) {
        gl.apply(XColor4f{Qt::yellow, 1.0f});
        gl.apply(XDevicePointSize{3.0});
        gl.apply(XDeviceLineWidth{3.0});

        gl.draw(DrawType::LINES,
                std::vector<Vec3f>{
                    Vec3f{m_sel1.pos.x, m_sel1.pos.y, 0.005f},
                    Vec3f{m_sel2.pos.x, m_sel2.pos.y, 0.005f},
                });
    }
}

void MapCanvas::paintSelectedRooms()
{
    if (!m_roomSelection || m_roomSelection->isEmpty())
        return;

    for (const Room *const room : *m_roomSelection) {
        paintSelectedRoom(room);
    }
}

void MapCanvas::paintSelectedRoom(const Room *const room)
{
    qint32 x = room->getPosition().x;
    qint32 y = room->getPosition().y;
    qint32 z = room->getPosition().z;
    qint32 layer = z - m_currentLayer;

    auto &gl = getOpenGL();
    gl.glPushMatrix();
    gl.apply(XEnable{XOption::BLEND});
    gl.apply(XDisable{XOption::DEPTH_TEST});

    if (!m_roomSelectionMove.inUse
        && ((x < m_visible1.x) || (x > m_visible2.x) || (y < m_visible1.y) || (y > m_visible2.y))) {
        // Room is distant
        const float cameraCenterX = (m_visible1.x + m_visible2.x) / 2.0f;
        const float cameraCenterY = (m_visible1.y + m_visible2.y) / 2.0f;

        // Calculate degrees from camera center to room
        const float adjacent = cameraCenterY - y;
        const float opposite = cameraCenterX - x;
        const float radians = std::atan2(adjacent, opposite);
        const float degrees = radians * static_cast<float>(180.0 / M_PI);

        // Identify room hint coordinates using an elipse to represent the screen
        const auto radiusX = (m_visible2.x - m_visible1.x) / 2.0f - 0.25f;
        const auto radiusY = (m_visible2.y - m_visible1.y) / 2.0f - 0.25f;
        const float roomHintX = cameraCenterX + (std::cos(radians) * radiusX * -1);
        const float roomHintY = cameraCenterY + (std::sin(radians) * radiusY * -1);

        // Rotate according to angle
        gl.glTranslatef(roomHintX, roomHintY, m_currentLayer + 0.1f);
        gl.glRotatef(degrees, 0.0f, 0.0f, 1.0f);

        // Scale based upon normalized distance
        const float distance = std::sqrt((adjacent * adjacent) + (opposite * opposite));
        const float normalized = 1.0f - (std::min(distance, BASESIZEX * 3.0f) / BASESIZEX * 3.0f);
        const float scaleFactor = std::max(0.3f, normalized);
        gl.glScalef(scaleFactor, scaleFactor, 1.0f);
    } else {
        // Room is close
        gl.glTranslatef(x - 0.5f, y - 0.5f, ROOM_Z_DISTANCE * layer);
    }

    gl.apply(XColor4f{Qt::black, 0.4f});

    gl.callList(m_gllist.room);

    const float len = 0.2f;
    gl.apply(XColor4f{(Qt::red)});
    gl.draw(DrawType::LINE_STRIP,
            std::vector<Vec3f>{Vec3f{0 + len, 0, 0.005f},
                               Vec3f{0, 0, 0.005f},
                               Vec3f{0, 0 + len, 0.005f}});
    gl.draw(DrawType::LINE_STRIP,
            std::vector<Vec3f>{Vec3f{0 + len, 1, 0.005f},
                               Vec3f{0, 1, 0.005f},
                               Vec3f{0, 1 - len, 0.005f}});
    gl.draw(DrawType::LINE_STRIP,
            std::vector<Vec3f>{Vec3f{1 - len, 1, 0.005f},
                               Vec3f{1, 1, 0.005f},
                               Vec3f{1, 1 - len, 0.005f}});
    gl.draw(DrawType::LINE_STRIP,
            std::vector<Vec3f>{Vec3f{1 - len, 0, 0.005f},
                               Vec3f{1, 0, 0.005f},
                               Vec3f{1, 0 + len, 0.005f}});

    if (m_roomSelectionMove.inUse) {
        if (m_roomSelectionMove.wrongPlace) {
            gl.apply(XColor4f{Qt::red, 0.4f});
        } else {
            gl.apply(XColor4f{Qt::white, 0.4f});
        }

        gl.glTranslatef(m_roomSelectionMove.pos.x,
                        m_roomSelectionMove.pos.y,
                        ROOM_Z_DISTANCE * layer);
        gl.callList(m_gllist.room);
    }

    gl.apply(XDisable{XOption::BLEND});
    gl.apply(XEnable{XOption::DEPTH_TEST});
    gl.glPopMatrix();
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
    const float x1 = static_cast<float>(marker->getPosition1().x) / INFOMARK_SCALE;
    const float y1 = static_cast<float>(marker->getPosition1().y) / INFOMARK_SCALE;
    const float x2 = static_cast<float>(marker->getPosition2().x) / INFOMARK_SCALE;
    const float y2 = static_cast<float>(marker->getPosition2().y) / INFOMARK_SCALE;
    const float dx = x2 - x1;
    const float dy = y2 - y1;

    auto &gl = getOpenGL();
    gl.glPushMatrix();
    gl.glTranslatef(x1, y1, 0.0f);
    gl.apply(XColor4f{(Qt::red)});
    gl.apply(XEnable{XOption::BLEND});
    gl.apply(XDisable{XOption::DEPTH_TEST});

    const auto draw_info_mark = [&gl](const InfoMark *const marker, const auto &dx, const auto &dy) {
        switch (marker->getType()) {
        case InfoMarkType::TEXT:
            gl.draw(DrawType::LINE_LOOP,
                    std::vector<Vec3f>{
                        Vec3f{0.0f, 0.0f, 1.0f},
                        Vec3f{0.0f, 0.25f + dy, 1.0f},
                        Vec3f{0.2f + dx, 0.25f + dy, 1.0f},
                        Vec3f{0.2f + dx, 0.0f, 1.0f},
                    });
            break;
        case InfoMarkType::LINE:
            gl.apply(XDevicePointSize{2.0});
            gl.apply(XDeviceLineWidth{2.0});
            gl.draw(DrawType::LINES,
                    std::vector<Vec3f>{
                        Vec3f{0.0f, 0.0f, 0.1f},
                        Vec3f{dx, dy, 0.1f},
                    });
            break;
        case InfoMarkType::ARROW:
            gl.apply(XDevicePointSize{2.0});
            gl.apply(XDeviceLineWidth{2.0});
            gl.draw(DrawType::LINE_STRIP,
                    std::vector<Vec3f>{Vec3f{0.0f, 0.05f, 1.0f},
                                       Vec3f{dx - 0.2f, dy + 0.1f, 1.0f},
                                       Vec3f{dx - 0.1f, dy + 0.1f, 1.0f}});
            gl.draw(DrawType::LINE_STRIP,
                    std::vector<Vec3f>{Vec3f{dx - 0.1f, dy + 0.1f - 0.07f, 1.0f},
                                       Vec3f{dx - 0.1f, dy + 0.1f + 0.07f, 1.0f},
                                       Vec3f{dx + 0.1f, dy + 0.1f, 1.0f}});
            break;
        }
    };

    draw_info_mark(marker, dx, dy);

    if (m_infoMarkSelectionMove.inUse) {
        gl.glTranslatef(m_infoMarkSelectionMove.pos.x, m_infoMarkSelectionMove.pos.y, 0.0f);
        draw_info_mark(marker, dx, dy);
    }

    gl.apply(XDisable{XOption::BLEND});
    gl.apply(XEnable{XOption::DEPTH_TEST});

    gl.glPopMatrix();
}

void MapCanvas::drawPreSpammedPath(const Coordinate &c1,
                                   const QList<Coordinate> &path,
                                   const QColor &color)
{
    if (path.isEmpty())
        return;

    std::vector<Vec3f> verts{};
    float dx = 0.0f, dy = 0.0f, dz = 0.0;
    bool anypath = false;

    auto it = path.begin();
    while (it != path.end()) {
        if (!anypath) {
            drawPathStart(c1, verts, color);
            anypath = true;
        }
        const Coordinate c2 = *it;
        if (!drawPath(c1, c2, dx, dy, dz, verts)) {
            break;
        }
        ++it;
    }
    if (anypath) {
        drawPathEnd(dx, dy, dz, verts);
    }
}

void MapCanvas::drawPathStart(const Coordinate &sc, std::vector<Vec3f> &verts, const QColor &color)
{
    const qint32 x1 = sc.x;
    const qint32 y1 = sc.y;
    const qint32 z1 = sc.z;
    const qint32 layer1 = z1 - m_currentLayer;

    auto &gl = getOpenGL();
    gl.glPushMatrix();
    gl.glTranslatef(x1, y1, 0);

    gl.apply(XColor4f{color});
    gl.apply(XEnable{XOption::BLEND});
    gl.apply(XDisable{XOption::DEPTH_TEST});
    gl.apply(XDevicePointSize{4.0});
    gl.apply(XDeviceLineWidth{4.0});

    const float srcZ = ROOM_Z_DISTANCE * static_cast<float>(layer1) + 0.3f;

    verts.emplace_back(0.0f, 0.0f, srcZ);
}

bool MapCanvas::drawPath(const Coordinate &sc,
                         const Coordinate &dc,
                         float &dx,
                         float &dy,
                         float &dz,
                         std::vector<Vec3f> &verts)
{
    qint32 x1 = sc.x;
    qint32 y1 = sc.y;
    // qint32 z1 = sc.z;

    qint32 x2 = dc.x;
    qint32 y2 = dc.y;
    qint32 z2 = dc.z;
    qint32 layer2 = z2 - m_currentLayer;

    dx = static_cast<float>(x2 - x1);
    dy = static_cast<float>(y2 - y1);
    dz = ROOM_Z_DISTANCE * static_cast<float>(layer2) + 0.3f;

    verts.emplace_back(dx, dy, dz);

    return true;
}

void MapCanvas::drawPathEnd(const float dx,
                            const float dy,
                            const float dz,
                            std::vector<Vec3f> &verts)
{
    auto &gl = getOpenGL();
    gl.draw(DrawType::LINE_STRIP, verts);

    gl.apply(XDevicePointSize{8.0});
    gl.draw(DrawType::POINTS,
            std::vector<Vec3f>{
                Vec3f{dx, dy, dz},
            });

    gl.apply(XDeviceLineWidth{2.0});
    gl.apply(XDevicePointSize{2.0});
    gl.apply(XDisable{XOption::BLEND});
    gl.apply(XEnable{XOption::DEPTH_TEST});
    gl.glPopMatrix();
}

void MapCanvas::initTextures()
{
    const auto wantTrilinear = getConfig().canvas.trilinearFiltering;

    loadPixmapArray(this->m_textures.terrain);
    loadPixmapArray(this->m_textures.road);
    loadPixmapArray(this->m_textures.trail);
    loadPixmapArray(this->m_textures.load);
    loadPixmapArray(this->m_textures.mob);
    this->m_textures.update = loadTexture(getPixmapFilenameRaw("update0.png"));

    if (wantTrilinear) {
        for (const auto &x : m_textures.terrain)
            setTrilinear(x);
        for (const auto &x : m_textures.road)
            setTrilinear(x);
        for (const auto &x : m_textures.trail)
            setTrilinear(x);
        for (const auto &x : m_textures.load)
            setTrilinear(x);
        for (const auto &x : m_textures.mob)
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
    CompileOnly gl{getOpenGL()};

    EnumIndexedArray<int, ExitDirection, NUM_EXITS_NESW> rotationDegrees;
    rotationDegrees[ExitDirection::NORTH] = 0;
    rotationDegrees[ExitDirection::EAST] = 90;
    rotationDegrees[ExitDirection::SOUTH] = 180;
    rotationDegrees[ExitDirection::WEST] = -90;

    EnumIndexedArray<QMatrix4x4, ExitDirection, NUM_EXITS_NESW> rotationMatricesAboutOrigin;
    EnumIndexedArray<QMatrix4x4, ExitDirection, NUM_EXITS_NESW> rotationMatricesAboutRoomMidpoint;
    for (auto dir : ALL_EXITS_NESW) {
        if (auto deg = rotationDegrees[dir]) {
            QMatrix4x4 tmp;
            tmp.translate(0.5f, 0.5f, 0);
            tmp.rotate(deg, 0, 0, 1);
            tmp.translate(-0.5f, -0.5f, 0);
            rotationMatricesAboutRoomMidpoint[dir] = tmp;

            tmp.setToIdentity();
            tmp.rotate(deg, 0, 0, 1);
            rotationMatricesAboutOrigin[dir] = tmp;
        }
    }
    const auto applyRotationMatrix = [](const XDraw &input, const QMatrix4x4 &rot) -> XDraw {
        auto type = input.getType();
        auto args = input.getArgs();
        for (auto &v : args) {
            auto tmpVec = QVector4D{v.x, v.y, v.z, 1.0};
            tmpVec = rot * tmpVec;
            auto w = tmpVec.w();
            assert(w != 0.0f);
            v.x = tmpVec.x() / w;
            v.y = tmpVec.y() / w;
            v.z = tmpVec.z() / w;
        }
        return XDraw{type, args};
    };
    const auto applyRotationDirectionAboutOrigin =
        [&rotationMatricesAboutOrigin, &applyRotationMatrix](const XDraw &input,
                                                             const ExitDirection dir) -> XDraw {
        return applyRotationMatrix(input, rotationMatricesAboutOrigin[dir]);
    };

    static constexpr const float ROOM_WALL_ALIGN = 0.008f;

    const auto northWallLines = XDraw{DrawType::LINES,
                                      std::vector<Vec3f>{Vec3f{0.0f, 0.0f + ROOM_WALL_ALIGN, 0.0f},
                                                         Vec3f{1.0f, 0.0f + ROOM_WALL_ALIGN, 0.0f}}};

    // Lines ABCD = AB, AC, CD.
    // 012345678901234567890
    //           A
    //        C--B--D
    const auto northDoorLines = XDraw{DrawType::LINES,
                                      std::vector<Vec3f>{Vec3f{0.5f, 0.0f, 0.0f},
                                                         Vec3f{0.5f, 0.11f, 0.0f},
                                                         Vec3f{0.35f, 0.11f, 0.0f},
                                                         Vec3f{0.65f, 0.11f, 0.0f}}};
    const auto northFlowBeginLines = XDraw{DrawType::LINE_STRIP,
                                           std::vector<Vec3f>{Vec3f{0.5f, 0.5f, 0.1f},
                                                              Vec3f{0.5f, 0.0f, 0.1f}}};
    const auto northFlowBeginTris = XDraw{DrawType::TRIANGLES,
                                          std::vector<Vec3f>{Vec3f{0.44f, 0.2f, 0.1f},
                                                             Vec3f{0.50f, 0.0f, 0.1f},
                                                             Vec3f{0.56f, 0.2f, 0.1f}}};

    /* NOTE: These point in a direction relative to the origin. */
    const auto northFlowEndLines = XDraw{DrawType::LINE_STRIP,
                                         std::vector<Vec3f>{Vec3f{0.0f, -0.5f, 0.1f},
                                                            Vec3f{0.0f, 0.0f, 0.1f}}};

    for (auto dir : ALL_EXITS_NESW) {
        const auto &rot = rotationMatricesAboutRoomMidpoint[dir];
        m_gllist.wall[dir] = gl.compile(applyRotationMatrix(northWallLines, rot));
        m_gllist.door[dir] = gl.compile(applyRotationMatrix(northDoorLines, rot));
        m_gllist.flow.begin[dir] = gl.compile(applyRotationMatrix(northFlowBeginLines, rot),
                                              applyRotationMatrix(northFlowBeginTris, rot));
        m_gllist.flow.end[dir] = gl.compile(
            applyRotationDirectionAboutOrigin(northFlowEndLines, dir));
    }

    m_gllist.door[ExitDirection::UP]
        = gl.compile(XDeviceLineWidth{3.0},
                     XDraw{DrawType::LINES,
                           std::vector<Vec3f>{Vec3f{0.69f, 0.31f, 0.0f},
                                              Vec3f{0.63f, 0.37f, 0.0f},
                                              Vec3f{0.57f, 0.31f, 0.0f},
                                              Vec3f{0.69f, 0.43f, 0.0f}}});
    m_gllist.door[ExitDirection::DOWN]
        = gl.compile(XDeviceLineWidth{3.0},
                     XDraw{DrawType::LINES,
                           std::vector<Vec3f>{Vec3f{0.31f, 0.69f, 0.0f},
                                              Vec3f{0.37f, 0.63f, 0.0f},
                                              Vec3f{0.31f, 0.57f, 0.0f},
                                              Vec3f{0.43f, 0.69f, 0.0f}}});

    m_gllist.flow.begin[ExitDirection::UP]
        = gl.compile(XDraw{DrawType::LINE_STRIP,
                           std::vector<Vec3f>{Vec3f{0.5f, 0.5f, 0.1f}, Vec3f{0.75f, 0.25f, 0.1f}}},
                     XDraw{DrawType::TRIANGLES,
                           std::vector<Vec3f>{Vec3f{0.51f, 0.42f, 0.1f},
                                              Vec3f{0.64f, 0.37f, 0.1f},
                                              Vec3f{0.60f, 0.48f, 0.1f}}});
    m_gllist.flow.begin[ExitDirection::DOWN]
        = gl.compile(XDraw{DrawType::LINE_STRIP,
                           std::vector<Vec3f>{Vec3f{0.5f, 0.5f, 0.1f}, Vec3f{0.25f, 0.75f, 0.1f}}},
                     XDraw{DrawType::TRIANGLES,
                           std::vector<Vec3f>{Vec3f{0.36f, 0.57f, 0.1f},
                                              Vec3f{0.33f, 0.67f, 0.1f},
                                              Vec3f{0.44f, 0.63f, 0.1f}}});

    m_gllist.flow.end[ExitDirection::DOWN] = gl.compile(
        XDraw{DrawType::LINE_STRIP,
              std::vector<Vec3f>{Vec3f{-0.25f, 0.25f, 0.1f}, Vec3f{0.0f, 0.0f, 0.1f}}});
    m_gllist.flow.end[ExitDirection::UP] = gl.compile(
        XDraw{DrawType::LINE_STRIP,
              std::vector<Vec3f>{Vec3f{0.25f, -0.25f, 0.1f}, Vec3f{0.0f, 0.0f, 0.1f}}});

    const auto offsetz = [](const std::vector<Vec3f> &input,
                            const float zoffset) -> std::vector<Vec3f> {
        std::vector<Vec3f> result;
        result.reserve(input.size());
        for (auto &v : input)
            result.emplace_back(v.x, v.y, v.z + zoffset);
        return result;
    };

    const auto makeRegularPolygon =
        [](const size_t verts, const Vec2f &center, const float radius) -> std::vector<Vec3f> {
        assert(radius > 0.0f);
        std::vector<Vec3f> result;
        result.reserve(verts);

        /*
         *     y
         *     |
         *     4
         *   5   3
         *  6  c  2 --> x
         *   7   1
         *     0
         */
        for (size_t i = 0u; i < verts; i++) {
            /* offset by -pi/2 to start at -Y instead of +X */
            const auto theta = static_cast<float>(2.0 * M_PI * i / verts);
            const auto x = center.x + radius * std::cos(theta);
            const auto y = center.y + radius * std::sin(theta);
            result.emplace_back(x, y, 0.0f);
        }

        return result;
    };

    static constexpr const auto TINY_Z_OFFSET = 0.01f;
    const Vec2f UP_CENTER2D{0.75f, 0.25f};
    const Vec3f UP_CENTER_OFFSET{UP_CENTER2D, TINY_Z_OFFSET};

    /* original was close to but not quite correct; the even verts are radius 0.12,
     * and the odd verts were offset by 0.08, 0.08 */
    const auto upOctagonVerts = makeRegularPolygon(8, UP_CENTER2D, 0.12f);
    const auto upOctagonVertsOffset = offsetz(upOctagonVerts, TINY_Z_OFFSET);

    const Vec2f DOWN_CENTER2D{0.25f, 0.75f};
    const Vec3f DOWN_CENTER_OFFSET{DOWN_CENTER2D, TINY_Z_OFFSET};
    const auto downOctagonVerts = makeRegularPolygon(8, DOWN_CENTER2D, 0.12f);
    const auto downOctagonVertsOffset = offsetz(downOctagonVerts, TINY_Z_OFFSET);
    const auto DOWN_X = XDraw{DrawType::LINES,
                              std::vector<Vec3f>{Vec3f{0.33f, 0.67f, 0.01f},
                                                 Vec3f{0.17f, 0.83f, 0.01f},
                                                 Vec3f{0.33f, 0.83f, 0.01f},
                                                 Vec3f{0.17f, 0.67f, 0.01f}}};

    /* This is using triangle strips instead of triangle fans because strips can be added
     * without using primitive restart.
     *
     * Also, this could just solve for the centroid, but it was easier to pass the known value.
     * However, the function already assumes that it's a CCW-wound octagon and doesn't verify,
     * so it wouldn't help much anyway. Better solution is to read a mesh from a file.
     */
    const auto makeOctagonTriStrip = [](const std::vector<Vec3f> &verts, const Vec2f &m2) {
        assert(verts.size() == 8);

        const auto a = verts[0];
        const auto b = verts[1];
        const auto c = verts[2];
        const auto d = verts[3];
        const auto e = verts[4];
        const auto f = verts[5];
        const auto g = verts[6];
        const auto h = verts[7];
        const auto m = Vec3f{m2, 0.0f};
        /*
         *     c
         *  d /| b
         *  |/ |/ \   abmced
         *  e  m  a
         *
         *  e  m  a
         *   \/| / \   efmgah
         *   f |/  h
         *     g
         */
        /* NOTE: The ee in abmced ee efmgah introduces a degenerate triangle that's always discarded;
         * this is a well-known trick used to stitch triangle strips together. */
        return XDraw{DrawType::TRIANGLE_STRIP,
                     std::vector<Vec3f>{a, b, m, c, e, d, e, e, e, f, m, g, a, h}};
    };

    m_gllist.exit.up.opaque = gl.compile(XColor4f{Qt::white},
                                         makeOctagonTriStrip(upOctagonVerts, UP_CENTER2D),
                                         XColor4f{Qt::black},
                                         XDraw{DrawType::LINE_LOOP, upOctagonVertsOffset},
                                         XDraw{DrawType::POINTS,
                                               std::vector<Vec3f>{UP_CENTER_OFFSET}});
    m_gllist.exit.up.transparent = gl.compile(XDraw{DrawType::LINE_LOOP, upOctagonVertsOffset},
                                              XDraw{DrawType::POINTS,
                                                    std::vector<Vec3f>{UP_CENTER_OFFSET}});
    m_gllist.exit.down.opaque = gl.compile(XColor4f{Qt::white},
                                           makeOctagonTriStrip(downOctagonVerts, DOWN_CENTER2D),
                                           XColor4f{Qt::black},
                                           XDraw{DrawType::LINE_LOOP, downOctagonVertsOffset},
                                           DOWN_X);
    m_gllist.exit.down.transparent = gl.compile(XDraw{DrawType::LINE_LOOP, downOctagonVertsOffset},
                                                DOWN_X);

    m_gllist.room = gl.compile(
        XDrawTextured{DrawType::TRIANGLE_STRIP,
                      std::vector<TexVert>{TexVert{Vec2f{0, 0}, Vec3f{0.0f, 1.0f, 0.0f}},
                                           TexVert{Vec2f{0, 1}, Vec3f{0.0f, 0.0f, 0.0f}},
                                           TexVert{Vec2f{1, 0}, Vec3f{1.0f, 1.0f, 0.0f}},
                                           TexVert{Vec2f{1, 1}, Vec3f{1.0f, 0.0f, 0.0f}}}});

    m_gllist.room_selection.outline = gl.compile(
        XDraw{DrawType::LINE_LOOP,
              std::vector<Vec3f>{Vec3f{-0.2f, -0.2f, 0.0f},
                                 Vec3f{-0.2f, 1.2f, 0.0f},
                                 Vec3f{1.2f, 1.2f, 0.0f},
                                 Vec3f{1.2f, -0.2f, 0.0f}}});
    m_gllist.room_selection.filled = gl.compile(XDraw{DrawType::TRIANGLE_STRIP,
                                                      std::vector<Vec3f>{Vec3f{-0.2f, 1.2f, 0.0f},
                                                                         Vec3f{-0.2f, -0.2f, 0.0f},
                                                                         Vec3f{1.2f, 1.2f, 0.0f},
                                                                         Vec3f{1.2f, -0.2f, 0.0f}}});
    m_gllist.character_hint.outline = gl.compile(
        XDraw{DrawType::LINE_LOOP,
              std::vector<Vec3f>{Vec3f{-0.5f, 0.0f, 0.0f},
                                 Vec3f{0.75f, 0.5f, 0.0f},
                                 Vec3f{0.25f, 0.0f, 0.0f},
                                 Vec3f{0.75f, -0.5f, 0.0f}}});
    m_gllist.character_hint.filled = gl.compile(XDraw{DrawType::TRIANGLE_STRIP,
                                                      std::vector<Vec3f>{
                                                          Vec3f{0.75f, 0.5f, 0.0f},
                                                          Vec3f{-0.5f, 0.0f, 0.0f},
                                                          Vec3f{0.25f, 0.0f, 0.0f},
                                                          Vec3f{0.75f, -0.5f, 0.0f},
                                                      }});
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
