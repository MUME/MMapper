#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <QWidget>
#include <QtGui/QMatrix4x4>
#include <QtGui/QMouseEvent>
#include <QtGui/QOpenGLTexture>
#include <QtGui/qopengl.h>
#include <QtGui>

#include "../global/EnumIndexedArray.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/mmapper2room.h"
#include "../mapdata/roomselection.h"
#include "../opengl/OpenGL.h"
#include "CanvasMouseModeEnum.h"
#include "RoadIndex.h"
#include "connectionselection.h"
#include "prespammedpath.h"

class ConnectionSelection;
class MapData;
class PrespammedPath;
class InfoMarkSelection;

enum class RoomTintEnum { DARK, NO_SUNDEATH };
static const size_t NUM_ROOM_TINTS = 2;
const MMapper::Array<RoomTintEnum, NUM_ROOM_TINTS> &getAllRoomTints();
#define ALL_ROOM_TINTS getAllRoomTints()

template<typename T>
using RoomTintArray = EnumIndexedArray<T, RoomTintEnum, NUM_ROOM_TINTS>;

struct NODISCARD LayerMeshes final
{
    UniqueMeshVector terrain;
    RoomTintArray<UniqueMesh> tints;
    UniqueMeshVector overlays;
    UniqueMeshVector doors;
    UniqueMeshVector walls;
    UniqueMeshVector dottedWalls;
    UniqueMeshVector upDownExits;
    UniqueMeshVector streamIns;
    UniqueMeshVector streamOuts;
    UniqueMesh layerBoost;
    bool isValid = false;

    LayerMeshes() = default;
    DEFAULT_MOVES_DELETE_COPIES(LayerMeshes);
    ~LayerMeshes() = default;

    void render(int thisLayer, int focusedLayer);
    explicit operator bool() const { return isValid; }
};

// This must be ordered so we can iterate over the layers from lowest to highest.
using BatchedMeshes = std::map<int, LayerMeshes>;

struct ScaleFactor final
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
    // pinch gesture
    float m_pinchFactor = 1.f;

private:
    static float clamp(float x)
    {
        assert(std::isfinite(x)); // note: also checks for NaN
        return std::clamp(x, MIN_VALUE, MAX_VALUE);
    }

public:
    static bool isClamped(float x) { return ::isClamped(x, MIN_VALUE, MAX_VALUE); }

public:
    float getRaw() const { return clamp(m_scaleFactor); }
    float getTotal() const { return clamp(m_scaleFactor * m_pinchFactor); }

public:
    void set(const float scale) { m_scaleFactor = clamp(scale); }
    void setPinch(const float pinch)
    {
        // Don't bother to clamp this, since the total is clamped.
        m_pinchFactor = pinch;
    }
    void endPinch()
    {
        const float total = getTotal();
        m_scaleFactor = total;
        m_pinchFactor = 1.f;
    }
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
        if (numSteps == 0)
            return;
        auto &self = *this;
        self *= std::pow(ZOOM_STEP, static_cast<float>(numSteps));
    }
};

struct MapCanvasViewport
{
private:
    QWidget &m_sizeWidget;

public:
    glm::mat4 m_viewProj{1.f};
    glm::vec2 m_scroll{0.f};
    ScaleFactor m_scaleFactor;
    int m_currentLayer = 0;

public:
    explicit MapCanvasViewport(QWidget &sizeWidget)
        : m_sizeWidget{sizeWidget}
    {}

public:
    auto width() const { return m_sizeWidget.width(); }
    auto height() const { return m_sizeWidget.height(); }
    Viewport getViewport() const
    {
        const auto &r = m_sizeWidget.rect();
        return Viewport{glm::ivec2{r.x(), r.y()}, glm::ivec2{r.width(), r.height()}};
    }
    float getTotalScaleFactor() const { return m_scaleFactor.getTotal(); }

public:
    std::optional<glm::vec3> project(const glm::vec3 &) const;
    glm::vec3 unproject_raw(const glm::vec3 &) const;
    glm::vec3 unproject_clamped(const glm::vec2 &) const;
    std::optional<glm::vec3> unproject(const QInputEvent *event) const;
    std::optional<MouseSel> getUnprojectedMouseSel(const QInputEvent *event) const;
    glm::vec2 getMouseCoords(const QInputEvent *event) const;
};

class MapScreen final
{
public:
    static constexpr const float DEFAULT_MARGIN_PIXELS = 24.f;

private:
    const MapCanvasViewport &m_viewport;
    enum class VisiblityResultEnum { INSIDE_MARGIN, ON_MARGIN, OUTSIDE_MARGIN, OFF_SCREEN };

public:
    explicit MapScreen(const MapCanvasViewport &);
    ~MapScreen();
    DELETE_CTORS_AND_ASSIGN_OPS(MapScreen);

public:
    glm::vec3 getCenter() const;
    bool isRoomVisible(const Coordinate &c, float margin) const;
    glm::vec3 getProxyLocation(const glm::vec3 &pos, float margin) const;

private:
    VisiblityResultEnum testVisibility(const glm::vec3 &input_pos, float margin) const;
};

struct MapCanvasInputState
{
    CanvasMouseModeEnum m_canvasMouseMode = CanvasMouseModeEnum::MOVE;

    bool m_mouseRightPressed = false;
    bool m_mouseLeftPressed = false;
    bool m_altPressed = false;
    bool m_ctrlPressed = false;

    // mouse selection
    std::optional<MouseSel> m_sel1;
    std::optional<MouseSel> m_sel2;
    // scroll origin of the current mouse movement
    std::optional<MouseSel> m_moveBackup;

    bool m_selectedArea = false; // no area selected at start time
    SharedRoomSelection m_roomSelection;

    struct RoomSelMove final
    {
        Coordinate2i pos;
        bool wrongPlace = false;
    };

    std::optional<RoomSelMove> m_roomSelectionMove;
    bool hasRoomSelectionMove() { return m_roomSelectionMove.has_value(); }

    std::shared_ptr<InfoMarkSelection> m_infoMarkSelection;

    struct InfoMarkSelectionMove final
    {
        Coordinate2f pos;
    };
    std::optional<InfoMarkSelectionMove> m_infoMarkSelectionMove;
    bool hasInfoMarkSelectionMove() const { return m_infoMarkSelectionMove.has_value(); }

    std::shared_ptr<ConnectionSelection> m_connectionSelection;

    PrespammedPath *m_prespammedPath = nullptr;

public:
    explicit MapCanvasInputState(PrespammedPath *prespammedPath);
    ~MapCanvasInputState();

public:
    static MouseSel getMouseSel(const std::optional<MouseSel> &x)
    {
        if (x.has_value())
            return x.value();

        assert(false);
        return {};
    }

public:
    bool hasSel1() const { return m_sel1.has_value(); }
    bool hasSel2() const { return m_sel2.has_value(); }
    bool hasBackup() const { return m_moveBackup.has_value(); }

public:
    MouseSel getSel1() const { return getMouseSel(m_sel1); }
    MouseSel getSel2() const { return getMouseSel(m_sel2); }
    MouseSel getBackup() const { return getMouseSel(m_moveBackup); }

public:
    void startMoving(const MouseSel &startPos) { m_moveBackup = startPos; }
    void stopMoving() { m_moveBackup.reset(); }
};
