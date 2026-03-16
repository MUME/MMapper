#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/EnumIndexedArray.h"
#include "../global/logging.h"
#include "../map/ExitDirection.h"
#include "../map/mmapper2room.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/roomselection.h"
#include "../opengl/OpenGL.h"
#include "CanvasMouseModeEnum.h"
#include "RoadIndex.h"
#include "connectionselection.h"
#include "prespammedpath.h"

#include <cassert>
#include <map>
#include <memory>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <variant>

#include <QOpenGLTexture>
#include <QWindow>
#include <QtGui/QMatrix4x4>
#include <QtGui/QMouseEvent>
#include <QtGui/qopengl.h>
#include <QtGui>

class ConnectionSelection;
class MapData;
class PrespammedPath;
class InfomarkSelection;

enum class NODISCARD RoomTintEnum { DARK, NO_SUNDEATH };
static const size_t NUM_ROOM_TINTS = 2;
NODISCARD extern const MMapper::Array<RoomTintEnum, NUM_ROOM_TINTS> &getAllRoomTints();
#define ALL_ROOM_TINTS getAllRoomTints()

struct NODISCARD ScaleFactor final
{
public:
    // value chosen so the inverse hits 1/25th after 20 steps, just as before.
    static constexpr const float ZOOM_STEP = 1.175f;
    static constexpr int MIN_VALUE_HUNDREDTHS = 4; // 1/25th
    static constexpr int MAX_VALUE_INT = 5;
    static constexpr float MIN_VALUE = static_cast<float>(MIN_VALUE_HUNDREDTHS) * 0.01f;
    static constexpr float MAX_VALUE = static_cast<float>(MAX_VALUE_INT);

private:
    float m_scaleFactor = 1.f;

private:
    NODISCARD static float clamp(float x)
    {
        assert(std::isfinite(x)); // note: also checks for NaN
        return std::clamp(x, MIN_VALUE, MAX_VALUE);
    }

public:
    NODISCARD static bool isClamped(float x) { return ::isClamped(x, MIN_VALUE, MAX_VALUE); }

public:
    NODISCARD float getRaw() const { return clamp(m_scaleFactor); }
    NODISCARD float getTotal() const { return clamp(m_scaleFactor); }

public:
    void set(const float scale) { m_scaleFactor = clamp(scale); }
    void reset() { *this = ScaleFactor(); }

public:
    ScaleFactor &operator*=(const float ratio)
    {
        assert(std::isfinite(ratio) && ratio > 0.f);
        set(m_scaleFactor * ratio);
        return *this;
    }
    void logStep(const int numSteps)
    {
        if (numSteps == 0) {
            return;
        }
        auto &self = *this;
        self *= std::pow(ZOOM_STEP, static_cast<float>(numSteps));
    }
};

struct NODISCARD MapCanvasViewport
{
private:
    QWindow &m_window;

private:
    mutable glm::mat4 m_viewProj{1.f};
    glm::vec2 m_scroll{0.f};
    ScaleFactor m_scaleFactor;
    int m_currentLayer = 0;
    mutable bool m_viewProjDirty = true;

public:
    explicit MapCanvasViewport(QWindow &window)
        : m_window{window}
    {}

public:
    NODISCARD auto width() const { return m_window.width(); }
    NODISCARD auto height() const { return m_window.height(); }

public:
    NODISCARD const glm::vec2 &getScroll() const { return m_scroll; }
    void setScroll(const glm::vec2 &scroll)
    {
        if (m_scroll != scroll) {
            m_scroll = scroll;
            m_viewProjDirty = true;
        }
    }

    NODISCARD const ScaleFactor &getScaleFactor() const { return m_scaleFactor; }
    void setScaleFactor(const ScaleFactor &scaleFactor)
    {
        m_scaleFactor = scaleFactor;
        m_viewProjDirty = true;
    }

    NODISCARD int getCurrentLayer() const { return m_currentLayer; }
    void setCurrentLayer(const int layer)
    {
        if (m_currentLayer != layer) {
            m_currentLayer = layer;
            m_viewProjDirty = true;
        }
    }

    void markViewProjDirty() { m_viewProjDirty = true; }
    void setMvpExtern(const glm::mat4 &mvp) const
    {
        m_viewProj = mvp;
        m_viewProjDirty = false;
    }

    NODISCARD const glm::mat4 &getViewProj() const;
    NODISCARD Viewport getViewport() const
    {
        return Viewport{glm::ivec2{0, 0}, glm::ivec2{m_window.width(), m_window.height()}};
    }
    NODISCARD float getTotalScaleFactor() const { return m_scaleFactor.getTotal(); }

public:
    NODISCARD std::optional<glm::vec3> project(const glm::vec3 &) const;
    NODISCARD glm::vec3 unproject_raw(const glm::vec3 &) const;
    NODISCARD glm::vec3 unproject_raw(const glm::vec3 &, const glm::mat4 &) const;
    NODISCARD glm::vec3 unproject_clamped(const glm::vec2 &) const;
    NODISCARD glm::vec3 unproject_clamped(const glm::vec2 &, const glm::mat4 &) const;
    NODISCARD std::optional<glm::vec3> unproject(const QInputEvent *event) const;
    NODISCARD std::optional<glm::vec3> unproject(const glm::vec2 &xy) const;
    NODISCARD std::optional<MouseSel> getUnprojectedMouseSel(const QInputEvent *event) const;
    NODISCARD std::optional<MouseSel> getUnprojectedMouseSel(const glm::vec2 &xy) const;
    NODISCARD std::optional<glm::vec2> getMouseCoords(const QInputEvent *event) const;
};

class NODISCARD MapScreen final
{
public:
    static constexpr const float DEFAULT_MARGIN_PIXELS = 24.f;

private:
    const MapCanvasViewport &m_viewport;
    enum class NODISCARD VisiblityResultEnum {
        INSIDE_MARGIN,
        ON_MARGIN,
        OUTSIDE_MARGIN,
        OFF_SCREEN
    };

public:
    explicit MapScreen(const MapCanvasViewport &);
    ~MapScreen();
    DELETE_CTORS_AND_ASSIGN_OPS(MapScreen);

public:
    NODISCARD glm::vec3 getCenter() const;
    NODISCARD bool isRoomVisible(const Coordinate &c, float margin) const;
    NODISCARD glm::vec3 getProxyLocation(const glm::vec3 &pos, float margin) const;
    NODISCARD const MapCanvasViewport &getViewport() const { return m_viewport; }

private:
    NODISCARD VisiblityResultEnum testVisibility(const glm::vec3 &input_pos, float margin) const;
};

struct NODISCARD AltDragState
{
    QPoint lastPos;
    QCursor originalCursor;
};

struct NODISCARD DragState
{
    glm::vec3 startWorldPos;
    glm::vec2 startScroll;
    glm::mat4 startViewProj;
};

struct NODISCARD PinchState
{
    float initialDistance = 0.f;
    float lastFactor = 1.f;
};

struct NODISCARD MagnificationState
{
    float lastValue = 1.f;
};

struct NODISCARD RoomSelMove
{
    Coordinate2i pos;
    bool wrongPlace = false;
};

struct NODISCARD InfomarkSelectionMove
{
    Coordinate2f pos;
};

struct NODISCARD AreaSelectionState
{};

struct NODISCARD MapCanvasInputState
{
    CanvasMouseModeEnum m_canvasMouseMode = CanvasMouseModeEnum::MOVE;

    bool m_mouseRightPressed = false;
    bool m_mouseLeftPressed = false;
    bool m_altPressed = false;
    bool m_ctrlPressed = false;

    // mouse selection
    std::optional<MouseSel> m_sel1;
    std::optional<MouseSel> m_sel2;

    // Mutually exclusive mouse-based interactions.
    std::optional<
        std::variant<AltDragState, DragState, RoomSelMove, InfomarkSelectionMove, AreaSelectionState>>
        m_activeInteraction;

    // Gesture states (pinch, magnification) can occur concurrently with mouse interactions
    // and each other, so they are managed independently.
    std::optional<PinchState> m_pinchState;
    std::optional<MagnificationState> m_magnificationState;

    SharedRoomSelection m_roomSelection;

    std::shared_ptr<InfomarkSelection> m_infoMarkSelection;

    std::shared_ptr<ConnectionSelection> m_connectionSelection;

    PrespammedPath &m_prespammedPath;

public:
    explicit MapCanvasInputState(PrespammedPath &prespammedPath);
    ~MapCanvasInputState();

public:
    NODISCARD static MouseSel getMouseSel(const std::optional<MouseSel> &x)
    {
        if (x.has_value()) {
            return x.value();
        }

        assert(false);
        return {};
    }

public:
    NODISCARD bool hasSel1() const { return m_sel1.has_value(); }
    NODISCARD bool hasSel2() const { return m_sel2.has_value(); }

public:
    NODISCARD MouseSel getSel1() const { return getMouseSel(m_sel1); }
    NODISCARD MouseSel getSel2() const { return getMouseSel(m_sel2); }

public:
    template<typename T>
    NODISCARD const T *getInteraction() const
    {
        return m_activeInteraction ? std::get_if<T>(&*m_activeInteraction) : nullptr;
    }

    template<typename T>
    NODISCARD T *getInteraction()
    {
        return m_activeInteraction ? std::get_if<T>(&*m_activeInteraction) : nullptr;
    }

private:
    template<typename T>
    void beginInteraction(T &&state)
    {
        if (m_activeInteraction && !getInteraction<std::decay_t<T>>()) {
            MMLOG_WARNING() << "Starting new interaction while another is active. Overwriting.";
            endInteraction();
        }

        m_activeInteraction.emplace(std::forward<T>(state));
    }

public:
    void beginAltDrag(const QPoint &pos, const QCursor &cursor)
    {
        beginInteraction(AltDragState{pos, cursor});
    }
    void beginDrag(const glm::vec3 &worldPos, const glm::vec2 &scroll, const glm::mat4 &viewProj)
    {
        beginInteraction(DragState{worldPos, scroll, viewProj});
    }
    void beginRoomMove() { beginInteraction(RoomSelMove{}); }
    void beginInfomarkMove() { beginInteraction(InfomarkSelectionMove{}); }
    void beginAreaSelection() { beginInteraction(AreaSelectionState{}); }
    void endInteraction() { m_activeInteraction.reset(); }

public:
    void beginPinch(const float initialDistance)
    {
        m_pinchState.emplace(PinchState{initialDistance});
    }
    void updatePinch(const float lastFactor)
    {
        if (!m_pinchState) {
            return;
        }
        m_pinchState->lastFactor = lastFactor;
    }
    void endPinch() { m_pinchState.reset(); }

    void beginMagnification() { m_magnificationState.emplace(MagnificationState{1.f}); }
    void updateMagnification(const float lastValue)
    {
        if (!m_magnificationState) {
            return;
        }
        m_magnificationState->lastValue = lastValue;
    }
    void endMagnification() { m_magnificationState.reset(); }

public:
    NODISCARD bool hasRoomSelectionMove() const { return getInteraction<RoomSelMove>() != nullptr; }
    NODISCARD bool hasInfomarkSelectionMove() const
    {
        return getInteraction<InfomarkSelectionMove>() != nullptr;
    }
    NODISCARD bool hasAreaSelection() const
    {
        return getInteraction<AreaSelectionState>() != nullptr;
    }
};
