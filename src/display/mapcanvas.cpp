// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mapcanvas.h"

#include "../configuration/configuration.h"
#include "../global/parserutils.h"
#include "../global/progresscounter.h"
#include "../global/utils.h"
#include "../map/ExitDirection.h"
#include "../map/RoomHandle.h"
#include "../map/coordinate.h"
#include "../map/exit.h"
#include "../map/infomark.h"
#include "../map/room.h"
#include "../map/roomid.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/roomselection.h"
#include "InfomarkSelection.h"
#include "MapCanvasData.h"
#include "MapCanvasRoomDrawer.h"
#include "connectionselection.h"

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
    if (pmc == nullptr) {
        pmc = this;
    }

    setCursor(Qt::OpenHandCursor);
    grabGesture(Qt::PinchGesture);
    setContextMenuPolicy(Qt::CustomContextMenu);
}

MapCanvas::~MapCanvas()
{
    NonOwningPointer &pmc = primaryMapCanvas();
    if (pmc == this) {
        pmc = nullptr;
    }

    cleanupOpenGL();
}

MapCanvas *MapCanvas::getPrimary()
{
    return primaryMapCanvas();
}

void MapCanvas::slot_layerUp()
{
    ++m_currentLayer;
    layerChanged();
}

void MapCanvas::slot_layerDown()
{
    --m_currentLayer;
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
    if (!selection.isValid()) {
        m_roomSelection.reset();
    } else {
        m_roomSelection = selection.getShared();
        auto &sel = deref(m_roomSelection);
        auto &map = m_data;
        sel.removeMissing(map);

        qDebug() << "Updated selection with" << sel.size() << "rooms";
        if (sel.size() == 1) {
            if (const auto r = map.findRoomHandle(sel.getFirstRoomId())) {
                auto message = mmqt::previewRoom(r,
                                                 mmqt::StripAnsiEnum::Yes,
                                                 mmqt::PreviewStyleEnum::ForLog);
                log(message);
            }
        }
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

void MapCanvas::slot_setInfomarkSelection(const std::shared_ptr<InfomarkSelection> &selection)
{
    if (m_canvasMouseMode == CanvasMouseModeEnum::CREATE_INFOMARKS) {
        m_infoMarkSelection = selection;

    } else if (selection == nullptr || selection->empty()) {
        m_infoMarkSelection = nullptr;

    } else {
        qDebug() << "Updated selection with" << selection->size() << "infomarks";
        m_infoMarkSelection = selection;
    }

    emit sig_newInfomarkSelection(m_infoMarkSelection.get());
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
            if (event->angleDelta().y() > 100) {
                slot_layerDown();
            } else if (event->angleDelta().y() < -100) {
                slot_layerUp();
            }
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
                if (const auto &optCenter2 = getUnprojectedMouseSel(event)) {
                    const auto delta2 = glm::ivec2(glm::vec2(optCenter2->to_vec3() - newCenter)
                                                   * static_cast<float>(SCROLL_SCALE));

                    emit sig_mapMove(-delta2.x, -delta2.y);

                    // NOTE: caller is expected to call update() after this function,
                    // so we don't have to update the viewport.
                }

                return true;
            };

            // Change the zoom level
            const int numSteps = event->angleDelta().y() / 120;
            if (numSteps != 0) {
                zoomAndMaybeRecenter(numSteps);
                zoomChanged();
                update();
            }
        }
        break;

    case CanvasMouseModeEnum::NONE:
    default:
        break;
    }

    event->accept();
}

void MapCanvas::slot_onForcedPositionChange()
{
    slot_requestUpdate();
}

bool MapCanvas::event(QEvent *const event)
{
    auto tryHandlePinchZoom = [this, event]() -> bool {
        if (event->type() != QEvent::Gesture) {
            return false;
        }

        const auto *const gestureEvent = dynamic_cast<QGestureEvent *>(event);
        if (gestureEvent == nullptr) {
            return false;
        }

        // Zoom in / out
        QGesture *const gesture = gestureEvent->gesture(Qt::PinchGesture);
        const auto *const pinch = dynamic_cast<QPinchGesture *>(gesture);
        if (pinch == nullptr) {
            return false;
        }

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
            zoomChanged(); // might not have actually changed
        }
        update();
        return true;
    };

    if (tryHandlePinchZoom()) {
        return true;
    }

    return QOpenGLWidget::event(event);
}

void MapCanvas::slot_createRoom()
{
    if (!hasSel1()) {
        return;
    }

    const Coordinate c = getSel1().getCoordinate();
    if (const auto &existing = m_data.findRoomHandle(c)) {
        return;
    }

    if (m_data.createEmptyRoom(Coordinate{c.x, c.y, m_currentLayer})) {
        // success
    } else {
        // failed!
    }
}

// REVISIT: This function doesn't need to return a shared ptr. Consider refactoring InfomarkSelection?
std::shared_ptr<InfomarkSelection> MapCanvas::getInfomarkSelection(const MouseSel &sel)
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
        return InfomarkSelection::alloc(m_data, lo, hi);
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
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            probe(glm::vec2{static_cast<float>(dx) * CLICK_RADIUS,
                            static_cast<float>(dy) * CLICK_RADIUS});
        }
    }

    const auto getScaled = [this](const glm::vec3 &c) -> Coordinate {
        const auto pos = glm::ivec3(glm::vec2(c) * static_cast<float>(INFOMARK_SCALE),
                                    m_currentLayer);
        return Coordinate{pos.x, pos.y, pos.z};
    };

    const auto hi = getScaled(maxCoord);
    const auto lo = getScaled(minCoord);

    return InfomarkSelection::alloc(m_data, lo, hi);
}

RoomHandle MapCanvas::pickRoomAt(const Coordinate &coord)
{
    const auto rooms = m_data.findRoomHandles(coord);

    if (rooms.empty()) {
        return RoomHandle{};
    }

    if (rooms.size() == 1) {
        return rooms.front();
    }

    // Multiple rooms at this coordinate - show picker menu
    QMenu menu(tr("Select Room"), this);

    for (const auto &room : rooms) {
        const auto extId = room.getIdExternal();
        const auto name = room.getName().toQString();
        const QString label = QString("#%1: %2").arg(extId.asUint32()).arg(name);

        QAction *action = menu.addAction(label);
        action->setData(QVariant::fromValue(room.getId().asUint32()));
    }

    menu.addSeparator();
    menu.addAction(tr("Cancel"));

    QAction *selected = menu.exec(QCursor::pos());
    if (selected == nullptr || !selected->data().isValid()) {
        return RoomHandle{};
    }

    const uint32_t selectedId = selected->data().toUInt();
    for (const auto &room : rooms) {
        if (room.getId().asUint32() == selectedId) {
            return room;
        }
    }

    return RoomHandle{};
}

void MapCanvas::mousePressEvent(QMouseEvent *const event)
{
    const bool hasLeftButton = (event->buttons() & Qt::LeftButton) != 0u;
    const bool hasRightButton = (event->buttons() & Qt::RightButton) != 0u;
    const bool hasCtrl = (event->modifiers() & Qt::CTRL) != 0u;
    MAYBE_UNUSED const bool hasAlt = (event->modifiers() & Qt::ALT) != 0u;

    if (hasLeftButton && hasAlt) {
        m_altDragState.emplace(AltDragState{event->pos(), cursor()});
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

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
            m_roomSelection = RoomSelection::createSelection(
                m_data.findAllRooms(getSel1().getCoordinate()));
            slot_setRoomSelection(SigRoomSelection{m_roomSelection});

            // Select infomarks under the cursor.
            slot_setInfomarkSelection(getInfomarkSelection(getSel1()));

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
            auto tmpSel = getInfomarkSelection(getSel1());
            if (m_infoMarkSelection != nullptr && !tmpSel->empty()
                && m_infoMarkSelection->contains(tmpSel->front().getId())) {
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
            if (hiz <= loz) {
                break;
            }

            const auto inv_denom = 1.f / (far.z - near.z);

            // REVISIT: This loop might feel laggy.
            // Consider clamping the bounds of what we know we're currently displaying (including XY).
            // (If you're looking almost parallel to the world, then the ray could pass through the
            // visible region but still be within the visible Z-range.)
            RoomIdSet tmpSel;
            for (int z = hiz; z >= loz; --z) {
                const float t = (static_cast<float>(z) - near.z) * inv_denom;
                assert(isClamped(t, 0.f, 1.f));
                const auto pos = glm::mix(near, far, t);
                assert(static_cast<int>(std::lround(pos.z)) == z);
                const Coordinate c2 = MouseSel{Coordinate2f{pos.x, pos.y}, z}.getCoordinate();
                for (const auto &r : m_data.findRoomHandles(c2)) {
                    tmpSel.insert(r.getId());
                }
            }

            if (!tmpSel.empty()) {
                slot_setRoomSelection(
                    SigRoomSelection(RoomSelection::createSelection(std::move(tmpSel))));
            }

            selectionChanged();
        }
        break;

    case CanvasMouseModeEnum::SELECT_ROOMS:
        // Cancel
        if (hasRightButton) {
            m_selectedArea = false;
            slot_clearRoomSelection();
        }
        // Select rooms
        if (hasLeftButton && hasSel1()) {
            if (!hasCtrl) {
                const auto pRoom = pickRoomAt(getSel1().getCoordinate());
                if (pRoom.exists() && m_roomSelection != nullptr
                    && m_roomSelection->contains(pRoom.getId())) {
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
            const auto room = pickRoomAt(getSel1().getCoordinate());
            if (room.exists()) {
                m_connectionSelection = ConnectionSelection::alloc(m_data, room, getSel1());
            } else {
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
            const auto room = pickRoomAt(getSel1().getCoordinate());
            if (room.exists()) {
                m_connectionSelection = ConnectionSelection::alloc(m_data, room, getSel1());
            } else {
                m_connectionSelection = nullptr;
            }
            if (!m_connectionSelection || !m_connectionSelection->isFirstValid()) {
                m_connectionSelection = nullptr;
            } else {
                const auto &r1 = m_connectionSelection->getFirst().room;
                const ExitDirEnum dir1 = m_connectionSelection->getFirst().direction;

                if (r1.getExit(dir1).outIsEmpty()) {
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
    if (m_altDragState.has_value()) {
        // The user released the Alt key mid-drag.
        if (!((event->modifiers() & Qt::ALT) != 0u)) {
            setCursor(m_altDragState->originalCursor);
            m_altDragState.reset();
            // Don't accept the event; let the underlying widgets handle it.
            return;
        }

        auto &conf = setConfig().canvas.advanced;
        auto &dragState = m_altDragState.value();
        bool angleChanged = false;

        const auto pos = event->pos();
        const auto delta = pos - dragState.lastPos;
        dragState.lastPos = pos;

        // Camera rotation sensitivity: 3 degree per pixel
        constexpr float SENSITIVITY = 0.3f;

        // Horizontal movement adjusts yaw (horizontalAngle)
        const int dx = delta.x();
        if (dx != 0) {
            conf.horizontalAngle.set(conf.horizontalAngle.get()
                                     + static_cast<int>(static_cast<float>(dx) * SENSITIVITY));
            angleChanged = true;
        }

        // Vertical movement adjusts pitch (verticalAngle), if auto-tilt is off
        if (!conf.autoTilt.get()) {
            const int dy = delta.y();
            if (dy != 0) {
                // Negated to match intuitive up/down dragging
                conf.verticalAngle.set(conf.verticalAngle.get()
                                       + static_cast<int>(static_cast<float>(-dy) * SENSITIVITY));
                angleChanged = true;
            }
        }

        if (angleChanged) {
            graphicsSettingsChanged();
        }
        event->accept();
        return;
    }

    const bool hasLeftButton = (event->buttons() & Qt::LeftButton) != 0u;

    if (m_canvasMouseMode != CanvasMouseModeEnum::MOVE) {
        // NOTE: Y is opposite of what you might expect here.
        const int vScroll = std::invoke([this, event]() -> int {
            const int h = height();
            const int MARGIN = std::min(100, h / 4);
            const auto y = event->position().y();
            if (y < MARGIN) {
                return SCROLL_SCALE;
            } else if (y > h - MARGIN) {
                return -SCROLL_SCALE;
            } else {
                return 0;
            }
        });
        const int hScroll = std::invoke([this, event]() -> int {
            const int w = width();
            const int MARGIN = std::min(100, w / 4);
            const auto x = event->position().x();
            if (x < MARGIN) {
                return -SCROLL_SCALE;
            } else if (x > w - MARGIN) {
                return SCROLL_SCALE;
            } else {
                return 0;
            }
        });

        emit sig_continuousScroll(hScroll, vScroll);
    }

    m_sel2 = getUnprojectedMouseSel(event);

    switch (m_canvasMouseMode) {
    case CanvasMouseModeEnum::SELECT_INFOMARKS:
        if (hasLeftButton && hasSel1() && hasSel2()) {
            if (hasInfomarkSelectionMove()) {
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
                auto &map = this->m_data;
                auto isMovable = [&map](RoomSelection &sel, const Coordinate &offset) -> bool {
                    sel.removeMissing(map);
                    return map.getCurrentMap().wouldAllowRelativeMove(sel.getRoomIds(), offset);
                };

                const auto diff = getSel2().pos.truncate() - getSel1().pos.truncate();
                const auto wrongPlace = !isMovable(deref(m_roomSelection), Coordinate{diff, 0});
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
                m_connectionSelection->setSecond(getSel2());

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
                m_connectionSelection->setSecond(getSel2());

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
    if (m_altDragState.has_value()) {
        setCursor(m_altDragState->originalCursor);
        m_altDragState.reset();
        event->accept();
        return;
    }

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
            if (hasInfomarkSelectionMove()) {
                const auto pos_copy = m_infoMarkSelectionMove->pos;
                m_infoMarkSelectionMove.reset();
                if (m_infoMarkSelection != nullptr) {
                    const auto offset = Coordinate{(pos_copy * INFOMARK_SCALE).truncate(), 0};

                    // Update infomark location
                    const InfomarkSelection &sel = deref(m_infoMarkSelection);
                    sel.applyOffset(offset);
                    infomarksChanged();
                }
            } else if (hasSel1() && hasSel2()) {
                // Add infomarks to selection
                const auto c1 = getSel1().getScaledCoordinate(INFOMARK_SCALE);
                const auto c2 = getSel2().getScaledCoordinate(INFOMARK_SCALE);
                auto tmpSel = InfomarkSelection::alloc(m_data, c1, c2);
                if (tmpSel && tmpSel->size() == 1) {
                    const InfomarkHandle &firstMark = tmpSel->front();
                    const Coordinate &pos = firstMark.getPosition1();
                    QString ctemp = QString("Selected Info Mark: [%1] (at %2,%3,%4)")
                                        .arg(firstMark.getText().toQString())
                                        .arg(pos.x)
                                        .arg(pos.y)
                                        .arg(pos.z);
                    log(ctemp);
                }
                slot_setInfomarkSelection(tmpSel);
            }
            m_selectedArea = false;
        }
        selectionChanged();
        break;

    case CanvasMouseModeEnum::CREATE_INFOMARKS:
        if (m_mouseLeftPressed && hasSel1() && hasSel2()) {
            m_mouseLeftPressed = false;
            const auto c1 = getSel1().getScaledCoordinate(INFOMARK_SCALE);
            const auto c2 = getSel2().getScaledCoordinate(INFOMARK_SCALE);
            // REVISIT: this previously searched for all the marks in c1, c2,
            // and then immediately cleared the selection.
            // Why??? Now it just allocates an empty selection.
            auto tmpSel = InfomarkSelection::allocEmpty(m_data, c1, c2);
            slot_setInfomarkSelection(tmpSel);
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
            const auto rooms = m_data.findRoomHandles(getSel1().getCoordinate());
            if (!rooms.empty()) {
                // Tooltip doesn't support ANSI, and there's no way to add formatted text.
                QString message;
                for (size_t i = 0; i < rooms.size(); ++i) {
                    if (i > 0) {
                        message += QString("\n\n--- Room %1 of %2 ---\n\n")
                                       .arg(i + 1)
                                       .arg(rooms.size());
                    }
                    message += mmqt::previewRoom(rooms[i],
                                                 mmqt::StripAnsiEnum::Yes,
                                                 mmqt::PreviewStyleEnum::ForDisplay);
                }

                QToolTip::showText(mapToGlobal(event->position().toPoint()),
                                   message,
                                   this,
                                   rect(),
                                   5000);
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
                    m_data.applySingleChange(Change{
                        room_change_types::MoveRelative2{m_roomSelection->getRoomIds(), moverel}});
                }

            } else {
                if (hasSel1() && hasSel2()) {
                    const Coordinate &c1 = getSel1().getCoordinate();
                    const Coordinate &c2 = getSel2().getCoordinate();
                    if (m_roomSelection == nullptr) {
                        // add rooms to default selections
                        m_roomSelection = RoomSelection::createSelection(
                            m_data.findAllRooms(c1, c2));
                    } else {
                        auto &sel = deref(m_roomSelection);
                        sel.removeMissing(m_data);
                        // add or remove rooms to/from default selection
                        const auto tmpSel = m_data.findAllRooms(c1, c2);
                        for (const RoomId key : tmpSel) {
                            if (sel.contains(key)) {
                                sel.erase(key);
                            } else {
                                sel.insert(key);
                            }
                        }
                    }
                }

                if (m_roomSelection != nullptr && !m_roomSelection->empty()) {
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
                const auto room = pickRoomAt(getSel1().getCoordinate());
                if (room.exists()) {
                    m_connectionSelection = ConnectionSelection::alloc(m_data, room, getSel1());
                }
            }
            if (m_connectionSelection) {
                m_connectionSelection->setSecond(getSel2());
            }

            if (!m_connectionSelection || !m_connectionSelection->isValid()) {
                m_connectionSelection = nullptr;
            } else {
                const auto first = m_connectionSelection->getFirst();
                const auto second = m_connectionSelection->getSecond();
                const auto &r1 = first.room;
                const auto &r2 = second.room;

                if (r1.exists() && r2.exists()) {
                    const ExitDirEnum dir1 = first.direction;
                    const ExitDirEnum dir2 = second.direction;

                    const RoomId id1 = r1.getId();
                    const RoomId id2 = r2.getId();

                    const bool isCompleteNew = m_connectionSelection->isCompleteNew();
                    m_connectionSelection = nullptr;

                    if (isCompleteNew) {
                        const WaysEnum ways = (m_canvasMouseMode
                                               != CanvasMouseModeEnum::CREATE_ONEWAY_CONNECTIONS)
                                                  ? WaysEnum::TwoWay
                                                  : WaysEnum::OneWay;
                        const exit_change_types::ModifyExitConnection change{ChangeTypeEnum::Add,
                                                                             id1,
                                                                             dir1,
                                                                             id2,
                                                                             ways};
                        m_data.applySingleChange(Change{change});
                    }
                    m_connectionSelection = ConnectionSelection::alloc(m_data);
                    m_connectionSelection->setFirst(id1, dir1);
                    m_connectionSelection->setSecond(id2, dir2);
                    slot_mapChanged();
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
                const auto room = pickRoomAt(getSel1().getCoordinate());
                if (room.exists()) {
                    m_connectionSelection = ConnectionSelection::alloc(m_data, room, getSel1());
                }
            }
            if (m_connectionSelection) {
                m_connectionSelection->setSecond(getSel2());
            }

            if (!m_connectionSelection || !m_connectionSelection->isValid()
                || !m_connectionSelection->isCompleteExisting()) {
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
    return {sizeHint().width() / 4, sizeHint().height() / 4};
}

QSize MapCanvas::sizeHint() const
{
    return {1280, 720};
}

void MapCanvas::slot_setScroll(const glm::vec2 &worldPos)
{
    m_scroll = worldPos;
    update();
}

void MapCanvas::slot_setHorizontalScroll(const float worldX)
{
    m_scroll.x = worldX;
    update();
}

void MapCanvas::slot_setVerticalScroll(const float worldY)
{
    m_scroll.y = worldY;
    update();
}

void MapCanvas::slot_zoomIn()
{
    m_scaleFactor.logStep(1);
    zoomChanged();
    update();
}

void MapCanvas::slot_zoomOut()
{
    m_scaleFactor.logStep(-1);
    zoomChanged();
    update();
}

void MapCanvas::slot_zoomReset()
{
    m_scaleFactor.set(1.f);
    zoomChanged();
    update();
}

void MapCanvas::onMovement()
{
    const Coordinate &pos = m_data.tryGetPosition().value_or(Coordinate{});
    m_currentLayer = pos.z;
    emit sig_onCenter(pos.to_vec2() + glm::vec2{0.5f, 0.5f});
    update();
}

void MapCanvas::slot_dataLoaded()
{
    onMovement();

    // REVISIT: is the makeCurrent necessary for calling update()?
    // MakeCurrentRaii makeCurrentRaii{*this};
    forceUpdateMeshes();
}

void MapCanvas::slot_moveMarker(const RoomId id)
{
    assert(id != INVALID_ROOMID);
    m_data.setRoom(id);
    onMovement();
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

void MapCanvas::forceUpdateMeshes()
{
    m_batches.resetExistingMeshesAndIgnorePendingRemesh();
    m_diff.resetExistingMeshesAndIgnorePendingRemesh();
    update();
}

void MapCanvas::slot_mapChanged()
{
    // REVISIT: Ideally we'd want to only update the layers/chunks
    // that actually changed.
    if ((false)) {
        m_batches.mapBatches.reset();
    }
    update();
}

void MapCanvas::slot_requestUpdate()
{
    update();
}

void MapCanvas::screenChanged()
{
    auto &gl = getOpenGL();
    if (!gl.isRendererInitialized()) {
        return;
    }

    const auto newDpi = static_cast<float>(QPaintDevice::devicePixelRatioF());
    const auto oldDpi = gl.getDevicePixelRatio();

    if (!utils::equals(newDpi, oldDpi)) {
        log(QString("Display: %1 DPI").arg(static_cast<double>(newDpi)));

        // NOTE: The new in-flight mesh will get UI updates when it "finishes".
        // This means we could save the existing raw mesh data and re-finish it
        // without having to compute it again, right?
        m_batches.resetExistingMeshesButKeepPendingRemesh();

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
    emit sig_selectionChanged();
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
        FALLTHROUGH;
    case CanvasMouseModeEnum::SELECT_INFOMARKS:
    case CanvasMouseModeEnum::CREATE_INFOMARKS:
        m_infoMarkSelectionMove.reset();
        slot_clearInfomarkSelection(); // calls selectionChanged();
        break;
    }
}
