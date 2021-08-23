// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mapcanvas.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include <QGestureEvent>
#include <QMessageLogContext>
#include <QOpenGLDebugMessage>
#include <QSize>
#include <QString>
#include <QToolTip>
#include <QtGui>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../global/utils.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/customaction.h"
#include "../mapdata/infomark.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/roomselection.h"
#include "InfoMarkSelection.h"
#include "MapCanvasData.h"
#include "MapCanvasRoomDrawer.h"
#include "connectionselection.h"

#if defined(_MSC_VER) || defined(__MINGW32__)
#undef near // Bad dog, Microsoft; bad dog!!!
#undef far  // Bad dog, Microsoft; bad dog!!!
#endif

using NonOwningPointer = MapCanvas *;
NODISCARD static NonOwningPointer &primaryMapCanvas()
{
    static NonOwningPointer primary = nullptr;
    return primary;
}

MapCanvas::MapCanvas(MapData &mapData,
                     PrespammedPath &prespammedPath,
                     Mmapper2Group &groupManager,
                     QWidget *const parent)
    : QOpenGLWidget{parent}
    , MapCanvasViewport{static_cast<QWidget &>(*this)}
    , MapCanvasInputState{prespammedPath}
    , m_mapScreen{static_cast<MapCanvasViewport &>(*this)}
    , m_opengl{}
    , m_glFont{m_opengl}
    , m_data{mapData}
    , m_groupManager{groupManager}
{
    NonOwningPointer &pmc = primaryMapCanvas();
    if (pmc == nullptr)
        pmc = this;

    setCursor(Qt::OpenHandCursor);
    grabGesture(Qt::PinchGesture);
    setContextMenuPolicy(Qt::CustomContextMenu);

    initSurface();
}

MapCanvas::~MapCanvas()
{
    NonOwningPointer &pmc = primaryMapCanvas();
    if (pmc == this)
        pmc = nullptr;

    cleanupOpenGL();
}

MapCanvas *MapCanvas::getPrimary()
{
    return primaryMapCanvas();
}

void MapCanvas::slot_layerUp()
{
    m_currentLayer++;
    layerChanged();
}

void MapCanvas::slot_layerDown()
{
    m_currentLayer--;
    layerChanged();
}

void MapCanvas::slot_layerReset()
{
    m_currentLayer = 0;
    layerChanged();
}

void MapCanvas::slot_setCanvasMouseMode(const CanvasMouseModeEnum mode)
{
    // FIXME: This probably isn't what you actually want here,
    // since it clears selections when re-choosing the same mode,
    // or when changing to a compatible mode.
    //
    // e.g. RAYPICK_ROOMS vs SELECT_CONNECTIONS,
    // or if you're trying to use MOVE to pan to reach more rooms
    // (granted, the latter isn't "necessary" because you can use
    // scrollbars or use the new zoom-recenter feature).
    slot_clearAllSelections();

    switch (mode) {
    case CanvasMouseModeEnum::MOVE:
        setCursor(Qt::OpenHandCursor);
        break;

    default:
    case CanvasMouseModeEnum::NONE:
    case CanvasMouseModeEnum::RAYPICK_ROOMS:
    case CanvasMouseModeEnum::SELECT_CONNECTIONS:
    case CanvasMouseModeEnum::CREATE_INFOMARKS:
        setCursor(Qt::CrossCursor);
        break;

    case CanvasMouseModeEnum::SELECT_ROOMS:
    case CanvasMouseModeEnum::CREATE_ROOMS:
    case CanvasMouseModeEnum::CREATE_CONNECTIONS:
    case CanvasMouseModeEnum::CREATE_ONEWAY_CONNECTIONS:
    case CanvasMouseModeEnum::SELECT_INFOMARKS:
        setCursor(Qt::ArrowCursor);
        break;
    }

    m_canvasMouseMode = mode;
    m_selectedArea = false;
    selectionChanged();
}

void MapCanvas::slot_setRoomSelection(const SigRoomSelection &selection)
{
    if (selection.isValid()) {
        m_roomSelection = selection.getShared();
        qDebug() << "Updated selection with" << m_roomSelection->size() << "rooms";
        if (m_roomSelection->size() == 1) {
            const Room *const r = m_roomSelection->getFirstRoom();
            const Coordinate &roomPos = r->getPosition();
            const auto x = roomPos.x;
            const auto y = roomPos.y;
            log(QString("Selected Room Coordinates: %1 %2\n%3").arg(x).arg(y).arg(r->toQString()));
        }
    } else {
        m_roomSelection.reset();
    }

    // Let the MainWindow know
    emit sig_newRoomSelection(selection);
    selectionChanged();
}

void MapCanvas::slot_setConnectionSelection(const std::shared_ptr<ConnectionSelection> &selection)
{
    m_connectionSelection = selection;
    emit sig_newConnectionSelection(selection.get());
    selectionChanged();
}

void MapCanvas::slot_setInfoMarkSelection(const std::shared_ptr<InfoMarkSelection> &selection)
{
    if (m_canvasMouseMode == CanvasMouseModeEnum::CREATE_INFOMARKS) {
        m_infoMarkSelection = selection;

    } else if (selection == nullptr || selection->empty()) {
        m_infoMarkSelection = nullptr;

    } else {
        qDebug() << "Updated selection with" << selection->size() << "infomarks";
        m_infoMarkSelection = selection;
    }

    emit sig_newInfoMarkSelection(m_infoMarkSelection.get());
    selectionChanged();
}

NODISCARD static uint32_t operator&(const Qt::KeyboardModifiers left, const Qt::Modifier right)
{
    return static_cast<uint32_t>(left) & static_cast<uint32_t>(right);
}

void MapCanvas::wheelEvent(QWheelEvent *const event)
{
    const bool hasCtrl = (event->modifiers() & Qt::CTRL) != 0u;

    switch (m_canvasMouseMode) {
    case CanvasMouseModeEnum::MOVE:
    case CanvasMouseModeEnum::SELECT_INFOMARKS:
    case CanvasMouseModeEnum::CREATE_INFOMARKS:
    case CanvasMouseModeEnum::RAYPICK_ROOMS:
    case CanvasMouseModeEnum::SELECT_ROOMS:
    case CanvasMouseModeEnum::SELECT_CONNECTIONS:
    case CanvasMouseModeEnum::CREATE_ROOMS:
    case CanvasMouseModeEnum::CREATE_CONNECTIONS:
    case CanvasMouseModeEnum::CREATE_ONEWAY_CONNECTIONS:
        if (hasCtrl) {
            if (event->angleDelta().y() > 100)
                slot_layerDown();
            else if (event->angleDelta().y() < -100)
                slot_layerUp();
        } else {
            const auto zoomAndMaybeRecenter = [this, event](const int numSteps) -> bool {
                assert(numSteps != 0);
                const auto optCenter = getUnprojectedMouseSel(event);

                // NOTE: Do a regular zoom if the projection failed.
                if (!optCenter) {
                    m_scaleFactor.logStep(numSteps);
                    return false; // failed to recenter
                }

                // Our goal is to keep the point under the mouse constant,
                // so we'll do the usual trick: translate to the origin,
                // scale/rotate, then translate back. However, the amount
                // we translate will differ because zoom changes our height.
                const auto newCenter = optCenter->to_vec3();
                const auto oldCenter = m_mapScreen.getCenter();

                // 1. recenter to mouse location

                const auto delta1 = glm::ivec2(glm::vec2(newCenter - oldCenter)
                                               * static_cast<float>(SCROLL_SCALE));

                emit sig_mapMove(delta1.x, delta1.y);

                // 2. zoom in
                m_scaleFactor.logStep(numSteps);

                // 3. adjust viewport for new projection
                setViewportAndMvp(width(), height());

                // 4. subtract the offset to same mouse coordinate;
                // This probably shouldn't ever fail, but let's make it conditional
                // (worst case: we're left centered on what we clicked on,
                // which will create infuriating overshoots if it ever happens)
                if (const auto optCenter2 = getUnprojectedMouseSel(event)) {
                    const auto delta2 = glm::ivec2(glm::vec2(optCenter2->to_vec3() - newCenter)
                                                   * static_cast<float>(SCROLL_SCALE));

                    emit sig_mapMove(-delta2.x, -delta2.y);

                    // NOTE: caller is expected to call resizeGL() after this function,
                    // so we don't have to update the viewport.
                }

                return true;
            };

            // Change the zoom level
            const int numSteps = event->angleDelta().y() / 120;
            if (numSteps != 0) {
                zoomAndMaybeRecenter(numSteps);
                zoomChanged();
                resizeGL();
            }
        }
        break;

    case CanvasMouseModeEnum::NONE:
    default:
        break;
    }

    event->accept();
}

void MapCanvas::slot_forceMapperToRoom()
{
    if (!m_roomSelection && hasSel1()) {
        m_roomSelection = RoomSelection::createSelection(m_data, getSel1().getCoordinate());
        emit sig_newRoomSelection(SigRoomSelection{m_roomSelection});
    }
    if (m_roomSelection->size() == 1) {
        const RoomId id = m_roomSelection->getFirstRoomId();
        // Force update rooms only if we're in play or mapping mode
        const bool update = getConfig().general.mapMode != MapModeEnum::OFFLINE;
        emit sig_setCurrentRoom(id, update);
    }
    slot_requestUpdate();
}

bool MapCanvas::event(QEvent *const event)
{
    auto tryHandlePinchZoom = [this, event]() -> bool {
        if (event->type() != QEvent::Gesture)
            return false;

        const auto *const gestureEvent = dynamic_cast<QGestureEvent *>(event);
        if (gestureEvent == nullptr)
            return false;

        // Zoom in / out
        QGesture *const gesture = gestureEvent->gesture(Qt::PinchGesture);
        const auto *const pinch = dynamic_cast<QPinchGesture *>(gesture);
        if (pinch == nullptr)
            return false;

        const QPinchGesture::ChangeFlags changeFlags = pinch->changeFlags();
        if (changeFlags & QPinchGesture::ScaleFactorChanged) {
            const auto pinchFactor = static_cast<float>(pinch->totalScaleFactor());
            m_scaleFactor.setPinch(pinchFactor);
            if ((false)) {
                zoomChanged(); // Don't call this here, because it's not true yet.
            }
        }
        if (pinch->state() == Qt::GestureFinished) {
            m_scaleFactor.endPinch();
            zoomChanged(); // might not have actualy changed
        }
        resizeGL();
        return true;
    };

    if (tryHandlePinchZoom())
        return true;

    return QOpenGLWidget::event(event);
}

void MapCanvas::slot_createRoom()
{
    if (!hasSel1())
        return;
    const Coordinate c = getSel1().getCoordinate();
    RoomSelection tmpSel = RoomSelection(m_data, c);
    if (tmpSel.empty()) {
        MAYBE_UNUSED const auto ignored = //
            m_data.createEmptyRoom(Coordinate{c.x, c.y, m_currentLayer});
    }
    mapChanged();
}

// REVISIT: This function doesn't need to return a shared ptr. Consider refactoring InfoMarkSelection?
std::shared_ptr<InfoMarkSelection> MapCanvas::getInfoMarkSelection(const MouseSel &sel)
{
    static constexpr float CLICK_RADIUS = 10.f;

    const auto center = sel.to_vec3();
    const auto optClickPoint = project(center);
    if (!optClickPoint) {
        // This should never happen, so we'll crash in debug, but let's do something
        // "sane" in release build since the assert will not trigger.
        assert(false);

        // This distance is in world space, but the mouse click is in screen space.
        static_assert(INFOMARK_SCALE % 5 == 0);
        static constexpr auto INFOMARK_CLICK_RADIUS = INFOMARK_SCALE / 5;
        const auto pos = sel.getScaledCoordinate(INFOMARK_SCALE);
        const auto lo = pos + Coordinate{-INFOMARK_CLICK_RADIUS, -INFOMARK_CLICK_RADIUS, 0};
        const auto hi = pos + Coordinate{+INFOMARK_CLICK_RADIUS, +INFOMARK_CLICK_RADIUS, 0};
        return InfoMarkSelection::alloc(m_data, lo, hi);
    }

    const glm::vec2 clickPoint = glm::vec2{optClickPoint.value()};
    glm::vec3 maxCoord = center;
    glm::vec3 minCoord = center;

    const auto probe = [this, &clickPoint, &maxCoord, &minCoord](const glm::vec2 &offset) -> void {
        const auto coord = unproject_clamped(clickPoint + offset);
        maxCoord = glm::max(maxCoord, coord);
        minCoord = glm::min(minCoord, coord);
    };

    // screen space can rotate relative to world space, and the projected world
    // space positions can be highly ansitropic (e.g. steep vertical angle),
    // so we'll expand the search area by probing in all 45 and 90 degree angles
    // from the mouse. This isn't perfect, but it should be good enough.
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            probe(glm::vec2{static_cast<float>(dx) * CLICK_RADIUS,
                            static_cast<float>(dy) * CLICK_RADIUS});

    const auto getScaled = [this](const glm::vec3 &c) -> Coordinate {
        const auto pos = glm::ivec3(glm::vec2(c) * static_cast<float>(INFOMARK_SCALE),
                                    m_currentLayer);
        return Coordinate{pos.x, pos.y, pos.z};
    };

    const auto hi = getScaled(maxCoord);
    const auto lo = getScaled(minCoord);

    return InfoMarkSelection::alloc(m_data, lo, hi);
}

void MapCanvas::mousePressEvent(QMouseEvent *const event)
{
    const bool hasLeftButton = (event->buttons() & Qt::LeftButton) != 0u;
    const bool hasRightButton = (event->buttons() & Qt::RightButton) != 0u;
    const bool hasCtrl = (event->modifiers() & Qt::CTRL) != 0u;
    const bool hasAlt = (event->modifiers() & Qt::ALT) != 0u;

    m_sel1 = m_sel2 = getUnprojectedMouseSel(event);
    m_mouseLeftPressed = hasLeftButton;
    m_mouseRightPressed = hasRightButton;

    if (event->button() == Qt::ForwardButton) {
        slot_layerUp();
        return event->accept();
    } else if (event->button() == Qt::BackButton) {
        slot_layerDown();
        return event->accept();
    } else if (!m_mouseLeftPressed && m_mouseRightPressed) {
        if (m_canvasMouseMode == CanvasMouseModeEnum::MOVE && hasSel1()) {
            // Select the room under the cursor
            m_roomSelection = RoomSelection::createSelection(m_data, getSel1().getCoordinate());
            slot_setRoomSelection(SigRoomSelection{m_roomSelection});

            // Select infomarks under the cursor.
            slot_setInfoMarkSelection(getInfoMarkSelection(getSel1()));

            selectionChanged();
        }
        m_mouseRightPressed = false;
        event->accept();
        return;
    }

    switch (m_canvasMouseMode) {
    case CanvasMouseModeEnum::CREATE_INFOMARKS:
        infomarksChanged();
        break;
    case CanvasMouseModeEnum::SELECT_INFOMARKS:
        // Select infomarks
        if (hasLeftButton && hasSel1()) {
            auto tmpSel = getInfoMarkSelection(getSel1());
            if (m_infoMarkSelection != nullptr && !tmpSel->empty()
                && m_infoMarkSelection->contains(tmpSel->front().get())) {
                m_infoMarkSelectionMove.reset();   // dtor, if necessary
                m_infoMarkSelectionMove.emplace(); // ctor
            } else {
                m_selectedArea = false;
                m_infoMarkSelectionMove.reset();
            }
        }
        selectionChanged();
        break;
    case CanvasMouseModeEnum::MOVE:
        if (hasLeftButton && hasSel1()) {
            setCursor(Qt::ClosedHandCursor);
            startMoving(m_sel1.value());
        }
        break;

    case CanvasMouseModeEnum::RAYPICK_ROOMS:
        if (hasLeftButton) {
            const auto xy = getMouseCoords(event);
            const auto near = unproject_raw(glm::vec3{xy, 0.f});
            const auto far = unproject_raw(glm::vec3{xy, 1.f});

            // Note: this rounding assumes we're looking down.
            // We're looking for integer z-planes that cross between
            // the hi and lo points.
            const auto hiz = static_cast<int>(std::floor(near.z));
            const auto loz = static_cast<int>(std::ceil(far.z));
            if (hiz <= loz)
                break;

            bool empty = true;
            const auto tmpSel = RoomSelection::createSelection(m_data);
            const auto inv_denom = 1.f / (far.z - near.z);

            // REVISIT: This loop might feel laggy.
            // Consider clamping the bounds of what we know we're currently displaying (including XY).
            // (If you're looking almost parallel to the world, then the ray could pass through the
            // visible region but still be within the visible Z-range.)
            for (int z = hiz; z >= loz; --z) {
                const float t = (static_cast<float>(z) - near.z) * inv_denom;
                assert(isClamped(t, 0.f, 1.f));
                const auto pos = glm::mix(near, far, t);
                assert(static_cast<int>(std::lround(pos.z)) == z);
                const Coordinate c2 = MouseSel{Coordinate2f{pos.x, pos.y}, z}.getCoordinate();
                if (const Room *const r = m_data.getRoom(c2)) {
                    tmpSel->receiveRoom(&m_data, r);
                    empty = false;
                }
            }

            if (!empty) {
                slot_setRoomSelection(SigRoomSelection(tmpSel));
            }

            selectionChanged();
        }
        break;

    case CanvasMouseModeEnum::SELECT_ROOMS:
        // Force mapper to room shortcut
        if (hasLeftButton && hasCtrl && hasAlt) {
            slot_clearRoomSelection();
            m_ctrlPressed = true;
            m_altPressed = true;
            slot_forceMapperToRoom();
            break;
        }
        // Cancel
        if (hasRightButton) {
            m_selectedArea = false;
            slot_clearRoomSelection();
        }
        // Select rooms
        if (hasLeftButton && hasSel1()) {
            if (!hasCtrl) {
                const auto tmpSel = RoomSelection::createSelection(m_data,
                                                                   getSel1().getCoordinate());
                if ((m_roomSelection != nullptr) && !tmpSel->empty()
                    && m_roomSelection->contains(tmpSel->getFirstRoomId())) {
                    m_roomSelectionMove.emplace(RoomSelMove{});
                } else {
                    m_roomSelectionMove.reset();
                    m_selectedArea = false;
                    slot_clearRoomSelection();
                }
            } else {
                m_ctrlPressed = true;
            }
        }
        selectionChanged();
        break;

    case CanvasMouseModeEnum::CREATE_ONEWAY_CONNECTIONS:
    case CanvasMouseModeEnum::CREATE_CONNECTIONS:
        // Select connection
        if (hasLeftButton && hasSel1()) {
            m_connectionSelection = ConnectionSelection::alloc(&m_data, getSel1());
            if (!m_connectionSelection->isFirstValid()) {
                m_connectionSelection = nullptr;
            }
            emit sig_newConnectionSelection(nullptr);
        }
        // Cancel
        if (hasRightButton) {
            slot_clearConnectionSelection();
        }
        selectionChanged();
        break;

    case CanvasMouseModeEnum::SELECT_CONNECTIONS:
        if (hasLeftButton && hasSel1()) {
            m_connectionSelection = ConnectionSelection::alloc(&m_data, getSel1());
            if (!m_connectionSelection->isFirstValid()) {
                m_connectionSelection = nullptr;
            } else {
                const Room *r1 = m_connectionSelection->getFirst().room;
                ExitDirEnum dir1 = m_connectionSelection->getFirst().direction;

                if (r1->exit(dir1).outIsEmpty()) {
                    m_connectionSelection = nullptr;
                }
            }
            emit sig_newConnectionSelection(nullptr);
        }
        // Cancel
        if (hasRightButton) {
            slot_clearConnectionSelection();
        }
        selectionChanged();
        break;

    case CanvasMouseModeEnum::CREATE_ROOMS:
        slot_createRoom();
        break;

    case CanvasMouseModeEnum::NONE:
    default:
        break;
    }
}

void MapCanvas::mouseMoveEvent(QMouseEvent *const event)
{
    const bool hasLeftButton = (event->buttons() & Qt::LeftButton) != 0u;

    if (m_canvasMouseMode != CanvasMouseModeEnum::MOVE) {
        // NOTE: Y is opposite of what you might expect here.
        const int vScroll = [this, event]() -> int {
            const int h = height();
            const int MARGIN = std::min(100, h / 4);
            const int y = event->pos().y();
            if (y < MARGIN) {
                return SCROLL_SCALE;
            } else if (y > h - MARGIN) {
                return -SCROLL_SCALE;
            } else {
                return 0;
            }
        }();
        const int hScroll = [this, event]() -> int {
            const int w = width();
            const int MARGIN = std::min(100, w / 4);
            const int x = event->pos().x();
            if (x < MARGIN) {
                return -SCROLL_SCALE;
            } else if (x > w - MARGIN) {
                return SCROLL_SCALE;
            } else {
                return 0;
            }
        }();

        emit sig_continuousScroll(hScroll, vScroll);
    }

    m_sel2 = getUnprojectedMouseSel(event);

    switch (m_canvasMouseMode) {
    case CanvasMouseModeEnum::SELECT_INFOMARKS:
        if (hasLeftButton && hasSel1() && hasSel2()) {
            if (hasInfoMarkSelectionMove()) {
                m_infoMarkSelectionMove->pos = getSel2().pos - getSel1().pos;
                setCursor(Qt::ClosedHandCursor);

            } else {
                m_selectedArea = true;
            }
        }
        selectionChanged();
        break;
    case CanvasMouseModeEnum::CREATE_INFOMARKS:
        if (hasLeftButton) {
            m_selectedArea = true;
        }
        // REVISIT: It doesn't look like anything actually changed here?
        infomarksChanged();
        break;
    case CanvasMouseModeEnum::MOVE:
        if (hasLeftButton && m_mouseLeftPressed && hasSel2() && hasBackup()) {
            const Coordinate2i delta
                = ((getSel2().pos - getBackup().pos) * static_cast<float>(SCROLL_SCALE)).truncate();
            if (delta.x != 0 || delta.y != 0) {
                // negated because dragging to right is scrolling to the left.
                emit sig_mapMove(-delta.x, -delta.y);
            }
        }
        break;

    case CanvasMouseModeEnum::RAYPICK_ROOMS:
        break;

    case CanvasMouseModeEnum::SELECT_ROOMS:
        if (hasLeftButton) {
            if (hasRoomSelectionMove() && hasSel1() && hasSel2()) {
                const auto diff = getSel2().pos.truncate() - getSel1().pos.truncate();
                const auto wrongPlace = !m_roomSelection->isMovable(Coordinate{diff, 0});

                m_roomSelectionMove->pos = diff;
                m_roomSelectionMove->wrongPlace = wrongPlace;

                setCursor(wrongPlace ? Qt::ForbiddenCursor : Qt::ClosedHandCursor);
            } else {
                m_selectedArea = true;
            }
        }
        selectionChanged();
        break;

    case CanvasMouseModeEnum::CREATE_ONEWAY_CONNECTIONS:
    case CanvasMouseModeEnum::CREATE_CONNECTIONS:
        if (hasLeftButton && hasSel2()) {
            if (m_connectionSelection != nullptr) {
                m_connectionSelection->setSecond(&m_data, getSel2());

                if (m_connectionSelection->isValid() && !m_connectionSelection->isCompleteNew()) {
                    m_connectionSelection->removeSecond();
                }

                selectionChanged();
            }
        } else {
            slot_requestUpdate();
        }
        break;

    case CanvasMouseModeEnum::SELECT_CONNECTIONS:
        if (hasLeftButton && hasSel2()) {
            if (m_connectionSelection != nullptr) {
                m_connectionSelection->setSecond(&m_data, getSel2());

                if (!m_connectionSelection->isCompleteExisting()) {
                    m_connectionSelection->removeSecond();
                }

                selectionChanged();
            }
        }
        break;

    case CanvasMouseModeEnum::CREATE_ROOMS:
    case CanvasMouseModeEnum::NONE:
    default:
        break;
    }
}

void MapCanvas::mouseReleaseEvent(QMouseEvent *const event)
{
    emit sig_continuousScroll(0, 0);
    m_sel2 = getUnprojectedMouseSel(event);

    if (m_mouseRightPressed) {
        m_mouseRightPressed = false;
    }

    switch (m_canvasMouseMode) {
    case CanvasMouseModeEnum::SELECT_INFOMARKS:
        setCursor(Qt::ArrowCursor);
        if (m_mouseLeftPressed) {
            m_mouseLeftPressed = false;
            if (hasInfoMarkSelectionMove()) {
                const auto pos_copy = m_infoMarkSelectionMove->pos;
                m_infoMarkSelectionMove.reset();
                if (m_infoMarkSelection != nullptr) {
                    const auto offset = Coordinate{(pos_copy * INFOMARK_SCALE).truncate(), 0};

                    // Update infomark location
                    for (const auto &mark : *m_infoMarkSelection) {
                        mark->setPosition1(mark->getPosition1() + offset);
                        mark->setPosition2(mark->getPosition2() + offset);
                    }
                    infomarksChanged();
                }
            } else if (hasSel1() && hasSel2()) {
                // Add infomarks to selection
                const auto c1 = getSel1().getScaledCoordinate(INFOMARK_SCALE);
                const auto c2 = getSel2().getScaledCoordinate(INFOMARK_SCALE);
                auto tmpSel = InfoMarkSelection::alloc(m_data, c1, c2);
                if (tmpSel && tmpSel->size() == 1) {
                    const std::shared_ptr<InfoMark> &firstMark = tmpSel->front();
                    const Coordinate &pos = firstMark->getPosition1();
                    QString ctemp = QString("Selected Info Mark: [%1] (at %2,%3,%4)")
                                        .arg(firstMark->getText().toQString())
                                        .arg(pos.x)
                                        .arg(pos.y)
                                        .arg(pos.z);
                    log(ctemp);
                }
                slot_setInfoMarkSelection(tmpSel);
            }
            m_selectedArea = false;
        }
        selectionChanged();
        break;

    case CanvasMouseModeEnum::CREATE_INFOMARKS:
        if (m_mouseLeftPressed && hasSel1() && hasSel2()) {
            m_mouseLeftPressed = false;
            // Add infomarks to selection
            const auto c1 = getSel1().getScaledCoordinate(INFOMARK_SCALE);
            const auto c2 = getSel2().getScaledCoordinate(INFOMARK_SCALE);
            auto tmpSel = InfoMarkSelection::alloc(m_data, c1, c2);
            tmpSel->clear(); // REVISIT: Should creation workflow require the selection to be empty?
            slot_setInfoMarkSelection(tmpSel);
        }
        infomarksChanged();
        break;

    case CanvasMouseModeEnum::MOVE:
        stopMoving();
        setCursor(Qt::OpenHandCursor);
        if (m_mouseLeftPressed) {
            m_mouseLeftPressed = false;
        }
        // Display a room info tooltip if there was no mouse movement
        if (hasSel1() && hasSel2() && getSel1().to_vec3() == getSel2().to_vec3()) {
            RoomSelection tmpSel = RoomSelection(m_data, getSel1().getCoordinate());
            if (!tmpSel.empty()) {
                QString message = tmpSel.getFirstRoom()->toQString();
                QToolTip::showText(mapToGlobal(event->pos()), message, this, rect(), 5000);
            }
        }
        break;

    case CanvasMouseModeEnum::RAYPICK_ROOMS:
        break;

    case CanvasMouseModeEnum::SELECT_ROOMS:
        setCursor(Qt::ArrowCursor);

        // This seems very unusual.
        if (m_ctrlPressed && m_altPressed) {
            break;
        }

        if (m_mouseLeftPressed) {
            m_mouseLeftPressed = false;

            if (m_roomSelectionMove.has_value()) {
                const auto pos = m_roomSelectionMove->pos;
                const bool wrongPlace = m_roomSelectionMove->wrongPlace;
                m_roomSelectionMove.reset();
                if (!wrongPlace && (m_roomSelection != nullptr)) {
                    const Coordinate moverel{pos, 0};
                    m_data.execute(std::make_unique<GroupMapAction>(std::make_unique<MoveRelative>(
                                                                        moverel),
                                                                    m_roomSelection),
                                   m_roomSelection);
                    mapChanged();
                }

            } else {
                if (hasSel1() && hasSel2()) {
                    const Coordinate &c1 = getSel1().getCoordinate();
                    const Coordinate &c2 = getSel2().getCoordinate();
                    if (m_roomSelection == nullptr) {
                        // add rooms to default selections
                        m_roomSelection = RoomSelection::createSelection(m_data, c1, c2);
                    } else {
                        // add or remove rooms to/from default selection
                        const auto tmpSel = RoomSelection(m_data, c1, c2);
                        for (const auto &[key, value] : tmpSel) {
                            if (m_roomSelection->contains(key)) {
                                m_roomSelection->unselect(key);
                            } else {
                                m_roomSelection->getRoom(key);
                            }
                        }
                    }
                }

                if (!m_roomSelection->empty()) {
                    slot_setRoomSelection(SigRoomSelection{m_roomSelection});
                }
            }
            m_selectedArea = false;
        }
        selectionChanged();
        break;

    case CanvasMouseModeEnum::CREATE_ONEWAY_CONNECTIONS:
    case CanvasMouseModeEnum::CREATE_CONNECTIONS:
        if (m_mouseLeftPressed && hasSel1() && hasSel2()) {
            m_mouseLeftPressed = false;

            if (m_connectionSelection == nullptr) {
                m_connectionSelection = ConnectionSelection::alloc(&m_data, getSel1());
            }
            m_connectionSelection->setSecond(&m_data, getSel2());

            if (!m_connectionSelection->isValid()) {
                m_connectionSelection = nullptr;
            } else {
                const auto first = m_connectionSelection->getFirst();
                const auto second = m_connectionSelection->getSecond();
                const Room *const r1 = first.room;
                const Room *const r2 = second.room;

                if (r1 != nullptr && r2 != nullptr) {
                    const ExitDirEnum dir1 = first.direction;
                    const ExitDirEnum dir2 = second.direction;

                    const RoomId &id1 = r1->getId();
                    const RoomId &id2 = r2->getId();

                    const auto tmpSel = RoomSelection::createSelection(m_data);
                    tmpSel->getRoom(id1);
                    tmpSel->getRoom(id2);

                    const bool isCompleteNew = m_connectionSelection->isCompleteNew();
                    m_connectionSelection = nullptr;

                    if (isCompleteNew) {
                        if (m_canvasMouseMode != CanvasMouseModeEnum::CREATE_ONEWAY_CONNECTIONS) {
                            m_data.execute(std::make_unique<AddTwoWayExit>(id1, id2, dir1, dir2),
                                           tmpSel);
                        } else {
                            m_data.execute(std::make_unique<AddOneWayExit>(id1, id2, dir1), tmpSel);
                        }
                        m_connectionSelection = ConnectionSelection::alloc();
                        m_connectionSelection->setFirst(&m_data, id1, dir1);
                        m_connectionSelection->setSecond(&m_data, id2, dir2);
                        mapChanged();
                    }
                }
            }
            slot_setConnectionSelection(m_connectionSelection);
        }
        selectionChanged();
        break;

    case CanvasMouseModeEnum::SELECT_CONNECTIONS:
        if (m_mouseLeftPressed && (m_connectionSelection != nullptr || hasSel1()) && hasSel2()) {
            m_mouseLeftPressed = false;

            if (m_connectionSelection == nullptr && hasSel1()) {
                m_connectionSelection = ConnectionSelection::alloc(&m_data, getSel1());
            }
            m_connectionSelection->setSecond(&m_data, getSel2());

            if (!m_connectionSelection->isValid() || !m_connectionSelection->isCompleteExisting()) {
                m_connectionSelection = nullptr;
            }
            slot_setConnectionSelection(m_connectionSelection);
        }
        selectionChanged();
        break;

    case CanvasMouseModeEnum::CREATE_ROOMS:
        if (m_mouseLeftPressed) {
            m_mouseLeftPressed = false;
        }
        break;

    case CanvasMouseModeEnum::NONE:
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
    return {BASESIZE, BASESIZE};
}

void MapCanvas::slot_setScroll(const glm::vec2 &worldPos)
{
    m_scroll = worldPos;
    resizeGL();
}

void MapCanvas::slot_setHorizontalScroll(const float worldX)
{
    m_scroll.x = worldX;
    resizeGL();
}

void MapCanvas::slot_setVerticalScroll(const float worldY)
{
    m_scroll.y = worldY;
    resizeGL();
}

void MapCanvas::slot_zoomIn()
{
    m_scaleFactor.logStep(1);
    zoomChanged();
    resizeGL();
}

void MapCanvas::slot_zoomOut()
{
    m_scaleFactor.logStep(-1);
    zoomChanged();
    resizeGL();
}

void MapCanvas::slot_zoomReset()
{
    m_scaleFactor.set(1.f);
    zoomChanged();
    resizeGL();
}

void MapCanvas::slot_dataLoaded()
{
    const Coordinate &pos = m_data.getPosition();
    m_currentLayer = pos.z;
    emit sig_onCenter(pos.to_vec2() + glm::vec2{0.5f, 0.5f});
    {
        // REVISIT: is the makeCurrent necessary for calling update()?
        // MakeCurrentRaii makeCurrentRaii{*this};
        mapAndInfomarksChanged();
    }
}

void MapCanvas::slot_moveMarker(const Coordinate &c)
{
    m_data.setPosition(c);
    m_currentLayer = c.z;
    {
        // REVISIT: is the makeCurrent necessary for calling update()?
        // MakeCurrentRaii makeCurrentRaii{*this};
        update();
    }

    emit sig_onCenter(c.to_vec2() + glm::vec2{0.5f, 0.5f});
}

void MapCanvas::infomarksChanged()
{
    m_batches.infomarksMeshes.reset();
    update();
}

void MapCanvas::layerChanged()
{
    update();
}

void MapCanvas::mapAndInfomarksChanged()
{
    m_batches.resetAll();
    update();
}

void MapCanvas::mapChanged()
{
    // REVISIT: Ideally we'd want to only update the layers/chunks
    // that actually changed.
    m_batches.mapBatches.reset();
    update();
}

void MapCanvas::slot_requestUpdate()
{
    update();
}

void MapCanvas::screenChanged()
{
    auto &gl = getOpenGL();
    const auto newDpi = static_cast<float>(QPaintDevice::devicePixelRatioF());
    const auto oldDpi = gl.getDevicePixelRatio();
    if (!utils::equals(newDpi, oldDpi)) {
        log(QString("Display: %1 DPI").arg(static_cast<double>(newDpi)));
        m_batches.resetAll();
        gl.setDevicePixelRatio(newDpi);
        auto &font = getGLFont();
        font.cleanup();
        font.init();
        update();
    }
}

void MapCanvas::selectionChanged()
{
    update();
}

void MapCanvas::graphicsSettingsChanged()
{
    update();
}

void MapCanvas::userPressedEscape(bool /*pressed*/)
{
    // TODO: encapsulate the states so we can easily cancel anything that's in use.

    switch (m_canvasMouseMode) {
    case CanvasMouseModeEnum::NONE:
    case CanvasMouseModeEnum::CREATE_ROOMS:
        break;

    case CanvasMouseModeEnum::CREATE_CONNECTIONS:
    case CanvasMouseModeEnum::SELECT_CONNECTIONS:
    case CanvasMouseModeEnum::CREATE_ONEWAY_CONNECTIONS:
        slot_clearConnectionSelection(); // calls selectionChanged();
        break;

    case CanvasMouseModeEnum::RAYPICK_ROOMS:
    case CanvasMouseModeEnum::SELECT_ROOMS:
        m_selectedArea = false;
        m_roomSelectionMove.reset();
        slot_clearRoomSelection(); // calls selectionChanged();
        break;

    case CanvasMouseModeEnum::MOVE:
        // special case for move: right click selects infomarks
        FALLTHRU;
    case CanvasMouseModeEnum::SELECT_INFOMARKS:
    case CanvasMouseModeEnum::CREATE_INFOMARKS:
        m_infoMarkSelectionMove.reset();
        slot_clearInfoMarkSelection(); // calls selectionChanged();
        break;
    }
}
