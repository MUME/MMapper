#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../clock/mumemoment.h"
#include "../global/ChangeMonitor.h"
#include "../global/Signal2.h"
#include "../map/PromptFlags.h"
#include "../mapdata/roomselection.h"
#include "../opengl/Font.h"
#include "../opengl/FontFormatFlags.h"
#include "../opengl/OpenGL.h"
#include "../opengl/Weather.h"
#include "FrameManager.h"
#include "Infomarks.h"
#include "MapCanvasConfig.h"
#include "MapCanvasData.h"
#include "MapCanvasRoomDrawer.h"
#include "Textures.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

#include <QColor>
#include <QMatrix4x4>
#include <QOpenGLDebugMessage>
#include <QSize>
#include <QtCore>

class CharacterBatch;
class ConnectionSelection;
class Coordinate;
class GameObserver;
class InfomarkSelection;
class MapData;
class Mmapper2Group;
class PrespammedPath;
class QMouseEvent;
class QNativeGestureEvent;
class QOpenGLDebugLogger;
class QOpenGLDebugMessage;
class QTouchEvent;
class QWheelEvent;
class RoomSelFakeGL;

// Minimal interface a MapCanvas uses to talk back to whatever widget/item
// is actually hosting it on screen. MapCanvas talks only to this
// interface, so it doesn't know or care which concrete host implements it.
class NODISCARD MapCanvasHost
{
public:
    virtual ~MapCanvasHost();

public:
    // Schedule a repaint (e.g. QOpenGLWindow::update()).
    void requestCanvasUpdate() { virt_requestCanvasUpdate(); }

    // The host's current device pixel ratio.
    NODISCARD qreal hostDevicePixelRatio() const { return virt_hostDevicePixelRatio(); }

    // The logical DPI of the host's current screen.
    NODISCARD qreal hostLogicalDpi() const { return virt_hostLogicalDpi(); }

    // The host's current size in logical pixels.
    NODISCARD QSize hostSize() const { return virt_hostSize(); }

    // Set the shape of the cursor shown while hovering the host.
    void setHostCursor(const Qt::CursorShape shape) { virt_setHostCursor(shape); }

    // Called with the GL context current, immediately before the core
    // composites its internal FBO into the currently bound framebuffer
    // (blitFboToDefault()). By this point the paint has left the viewport
    // at (0, 0, w, h) -- the right presentation rect for a host that owns
    // its whole framebuffer (e.g. QOpenGLWindow), hence the no-op default.
    // A host that presents into a SUB-RECT of a framebuffer it shares with
    // other content must re-apply its own viewport here, or the blit lands
    // at the framebuffer's bottom-left corner instead of the host's rect.
    void applyPresentViewport() { virt_applyPresentViewport(); }

private:
    virtual void virt_requestCanvasUpdate() = 0;
    NODISCARD virtual qreal virt_hostDevicePixelRatio() const = 0;
    NODISCARD virtual qreal virt_hostLogicalDpi() const = 0;
    NODISCARD virtual QSize virt_hostSize() const = 0;
    virtual void virt_setHostCursor(Qt::CursorShape shape) = 0;
    virtual void virt_applyPresentViewport() {}
};

class NODISCARD_QOBJECT MapCanvas final : public QObject,
                                          private MapCanvasViewport,
                                          private MapCanvasInputState
{
    Q_OBJECT

public:
    static constexpr const int SCROLL_SCALE = MapCanvasConfig::SCROLL_SCALE;

private:
    struct NODISCARD Diff final
    {
        using DiffQuadVector = std::vector<RoomQuadTexVert>;

        struct NODISCARD MaybeDataOrMesh final
            : public std::variant<std::monostate, DiffQuadVector, UniqueMesh>
        {
        public:
            using base = std::variant<std::monostate, DiffQuadVector, UniqueMesh>;
            using base::base;

        public:
            NODISCARD bool empty() const { return std::holds_alternative<std::monostate>(*this); }
            NODISCARD bool hasData() const { return std::holds_alternative<DiffQuadVector>(*this); }
            NODISCARD bool hasMesh() const { return std::holds_alternative<UniqueMesh>(*this); }

        public:
            NODISCARD const DiffQuadVector &getData() const
            {
                return std::get<DiffQuadVector>(*this);
            }
            NODISCARD const UniqueMesh &getMesh() const { return std::get<UniqueMesh>(*this); }

            void render(OpenGL &gl, const MMTextureId texId)
            {
                if (empty()) {
                    assert(false);
                    return;
                }

                if (hasData()) {
                    *this = gl.createRoomQuadTexBatch(getData(), texId);
                    assert(hasMesh());
                    // REVISIT: rendering immediately after uploading the mesh may lag,
                    // so consider delaying until the data is already on the GPU.
                }

                if (!hasMesh()) {
                    assert(false);
                    return;
                }
                auto &mesh = getMesh();
                mesh.render(gl.getDefaultRenderState().withBlend(BlendModeEnum::TRANSPARENCY));
            }
        };

        struct NODISCARD HighlightDiff final
        {
            Map saved;
            Map current;
            MaybeDataOrMesh highlights;
        };

        std::optional<std::future<HighlightDiff>> futureHighlight;
        std::optional<HighlightDiff> highlight;

        NODISCARD bool isUpToDate(const Map &saved, const Map &current) const;
        NODISCARD bool hasRelatedDiff(const Map &save) const;
        void cancelUpdates(const Map &saved);
        void maybeAsyncUpdate(const Map &saved, const Map &current);

        void resetExistingMeshesAndIgnorePendingRemesh()
        {
            futureHighlight.reset();
            highlight.reset();
        }
    };

private:
    MapScreen m_mapScreen;
    GameObserver &m_observer;
    mutable OpenGL m_opengl;
    GLFont m_glFont;
    Batches m_batches;
    MapCanvasTextures m_textures;
    MapData &m_data;
    Mmapper2Group &m_groupManager;
    Diff m_diff;

    // Declared before m_frameManager, whose constructor already invokes the
    // repaint callback that uses it.
    MapCanvasHost &m_host;
    Qt::CursorShape m_currentCursor = Qt::ArrowCursor;

    FrameManager m_frameManager;
    std::unique_ptr<QOpenGLDebugLogger> m_logger;
    Signal2Lifetime m_lifetime;
    GLWeather m_weather;
    bool m_cleanedUp = false;

    // Deferred (defer-to-next-render) state for GL work that historically ran
    // outside of paintGL(), relying on the implicit assumption that the host's
    // GL context happened to still be current. See hostPaintGL()/actuallyPaintGL().
    bool m_pendingForceUpdateMeshes = false;
    bool m_pendingUpdateTextures = false;
    std::optional<float> m_pendingDpr;
    std::optional<float> m_pendingRenderScale;
    // font preference or logical DPI changed; applied like m_pendingRenderScale
    bool m_pendingFontReload = false;

    // Touch long-press -> context menu. A left press synthesized from touch
    // arms the timer; moving past the drag threshold or releasing disarms it
    // (see handleMousePress()/handleMouseMove()/handleMouseRelease()).
    QTimer m_longPressTimer;
    std::optional<QPointF> m_longPressPos;
    // Set when the timer fired: the finger's eventual release is then
    // swallowed, since the context menu was the response to that gesture.
    bool m_longPressFired = false;

    // Touch flick: recent scroll samples of a touch drag in MOVE mode, and
    // the velocity (world units per second) the view keeps after release
    // until it decays (see handleMouseRelease()/onFlickTick()).
    struct NODISCARD ScrollSample final
    {
        qint64 msecs = 0;
        glm::vec2 scroll{0.f};
    };
    std::deque<ScrollSample> m_scrollSamples;
    QTimer m_flickTimer;
    glm::vec2 m_flickVelocity{0.f};

public:
    explicit MapCanvas(MapData &mapData,
                       GameObserver &observer,
                       PrespammedPath &prespammedPath,
                       Mmapper2Group &groupManager,
                       MapCanvasHost &host);
    ~MapCanvas() override;

private:
    NODISCARD qreal currentDpr() const { return m_host.hostDevicePixelRatio(); }
    NODISCARD float currentLogicalDpi() const
    {
        return static_cast<float>(m_host.hostLogicalDpi());
    }
    // Configuration::canvas.renderScale (a percentage) as the factor the
    // offscreen FBO is rendered at; see Legacy::Functions::setRenderScale().
    NODISCARD static float configuredRenderScale();
    NODISCARD QSize currentHostSize() const { return m_host.hostSize(); }
    void applyCursor(Qt::CursorShape shape)
    {
        m_currentCursor = shape;
        m_host.setHostCursor(shape);
    }
    NODISCARD QCursor currentCursor() const { return QCursor(m_currentCursor); }

private:
    NODISCARD inline auto &getOpenGL() { return m_opengl; }
    NODISCARD inline auto &getGLFont() { return m_glFont; }

public:
    // Called by the host with a current GL context. Idempotent: cleanup state
    // is reset the next time a host attaches and re-initializes.
    NODISCARD bool hostInitializeGL();
    void hostPaintGL();
    void hostResize(int width, int height, qreal dpr);
    // Must be called with a current GL context (the host is responsible for that).
    void hostCleanupGL();
    NODISCARD bool isCleanedUp() const { return m_cleanedUp; }

public:
    using MapCanvasViewport::getTotalScaleFactor;
    using MapCanvasViewport::height;
    using MapCanvasViewport::width;
    void setZoom(float zoom)
    {
        ScaleFactor sf = getScaleFactor();
        sf.set(zoom);
        setScaleFactor(sf);
        zoomChanged();
    }
    NODISCARD float getRawZoom() const { return getScaleFactor().getRaw(); }

private:
    void onMovement();
    void startMoving(const MouseSel &startPos);
    void stopMoving();

private:
    void reportGLVersion();
    NODISCARD bool isBlacklistedDriver();

protected:
    void onViewProjDirty() const override;

public:
    // Bodies of the QOpenGLWindow event overrides on the facade, exposed as
    // plain QEvent-taking entry points so any host can forward into the same
    // logic. handleMouseMove() further delegates the position+modifiers core
    // to handlePointerMove(), which a hover-event path with no button/press
    // semantics can call directly.
    void handleMousePress(QMouseEvent *event);
    void handleMouseRelease(QMouseEvent *event);
    void handleMouseMove(QMouseEvent *event);
    void handlePointerMove(glm::vec2 pos, Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons);
    void handleWheel(QWheelEvent *event);
    void handleTouch(QTouchEvent *event);
    void handleNativeGesture(QNativeGestureEvent *event);
    // Returns true if the event was handled (i.e. it was a recognized native
    // zoom gesture); mirrors the facade's bool event(QEvent*) override.
    NODISCARD bool handleGenericEvent(QEvent *event);

private:
    void drawGroupCharacters(CharacterBatch &characterBatch, ServerRoomId yourServerId);

    void initLogger();

    void initTextures();
    void updateTextures();
    void updateMultisampling();

    NODISCARD std::shared_ptr<InfomarkSelection> getInfomarkSelection(const MouseSel &sel);

public:
    void setMvp(const glm::mat4 &viewProj);
    void setViewportAndMvp(int width, int height, qreal dpr);

    void zoomAt(float factor, glm::vec2 mousePos);
    void handleZoomAtEvent(const QInputEvent *event, float deltaFactor);

    NODISCARD BatchedInfomarksMeshes getInfomarksMeshes();
    void drawInfomark(InfomarksBatch &batch,
                      const InfomarkHandle &marker,
                      int currentLayer,
                      glm::vec2 offset = {},
                      const std::optional<Color> &overrideColor = std::nullopt);
    void updateBatches();
    void finishPendingMapBatches();
    void updateMapBatches();
    void updateInfomarkBatches();

    void applyPendingGLWork();
    void actuallyPaintGL();
    void paintMap();
    void renderMapBatches();
    void paintBatchedInfomarks();
    void paintSelections();
    void paintSelectionArea();
    void paintNewInfomarkSelection();
    void paintSelectedRooms();
    void paintSelectedRoom(RoomSelFakeGL &, const RawRoom &room);
    void paintSelectedConnection();
    void paintNearbyConnectionPoints();
    void paintSelectedInfomarks();
    void paintCharacters();
    void paintDifferences();

    // Immediately drops all cached meshes (must run with a current GL context).
    void forceUpdateMeshes();
    // Defers forceUpdateMeshes() to the start of the next hostPaintGL().
    void requestForceUpdateMeshes();
    // Defers updateTextures() to the start of the next hostPaintGL().
    void requestUpdateTextures();

public:
    void slot_rebuildMeshes() { requestForceUpdateMeshes(); }
    void infomarksChanged();
    void layerChanged();
    void slot_mapChanged();
    void slot_requestUpdate();
    void screenChanged();
    void selectionChanged();
    void graphicsSettingsChanged();
    void zoomChanged() { emit sig_zoomChanged(getRawZoom()); }
    void syncViewportConfig();

public:
    // Forwarded by MapWindow::keyPressEvent()/keyReleaseEvent(); the bool
    // parameter is otherwise unused (see the .cpp).
    void userPressedEscape(bool);

private:
    // Selects the room and infomarks under a canvas-local, top-left-origin
    // point and emits sig_customContextMenuRequested; shared by the
    // right-mouse-button press and the touch long-press (see
    // handleMousePress()).
    void requestContextMenuAt(const QPointF &pos);
    void cancelLongPress();
    void onLongPress();
    void recordScrollSample(qint64 msecs);
    void startFlick();
    void stopFlick();
    void onFlickTick();

public:
private:
    void log(const QString &msg) { emit sig_log("MapCanvas", msg); }

signals:
    void sig_onCenter(glm::vec2 worldCoord);
    void sig_mapMove(int dx, int dy);
    void sig_setScrollBars(Coordinate min, Coordinate max);
    void sig_continuousScroll(int, int);

    void sig_log(const QString &, const QString &);

    void sig_selectionChanged();
    void sig_newRoomSelection(const SigRoomSelection &);
    void sig_newConnectionSelection(ConnectionSelection *);
    void sig_newInfomarkSelection(InfomarkSelection *);

    void sig_setCurrentRoom(RoomId id, bool update);
    void sig_zoomChanged(float);

    void sig_showTooltip(const QString &text, const QPoint &pos);
    void sig_customContextMenuRequested(const QPoint &pos);
    void sig_dismissContextMenu();

    // Emitted from hostInitializeGL() when OpenGL initialization fails (e.g.
    // an unsupported/blacklisted driver). The core stays QtWidgets-free, so
    // it's up to whoever connects to this signal (the widget facade) to show
    // a QMessageBox and/or hide the host window.
    void sig_glInitFailed(const QString &reason);

    // Emitted from the (direct) GL debug message handler for fatal errors.
    // The core stays QtWidgets-free, so the widget facade is responsible for
    // showing a blocking dialog and aborting.
    void sig_glFatalError(const QString &message);

public slots:
    void slot_onForcedPositionChange();
    void slot_createRoom();

    void slot_setCanvasMouseMode(CanvasMouseModeEnum mode);

    void slot_setScroll(const glm::vec2 worldPos);
    // void setScroll(const glm::ivec2 ) = delete; // moc tries to call the wrong one if you define this
    void slot_setHorizontalScroll(float worldX);
    void slot_setVerticalScroll(float worldY);

    void slot_zoomIn();
    void slot_zoomOut();
    void slot_zoomReset();
    void slot_centerOnPlayer() { onMovement(); }

    void slot_layerUp();
    void slot_layerDown();
    void slot_layerReset();

    void slot_setRoomSelection(const SigRoomSelection &);
    void slot_setConnectionSelection(const std::shared_ptr<ConnectionSelection> &);
    void slot_setInfomarkSelection(const std::shared_ptr<InfomarkSelection> &);

    void slot_clearRoomSelection() { slot_setRoomSelection(SigRoomSelection{}); }
    void slot_clearConnectionSelection() { slot_setConnectionSelection(nullptr); }
    void slot_clearInfomarkSelection() { slot_setInfomarkSelection(nullptr); }

    void slot_clearAllSelections()
    {
        slot_clearRoomSelection();
        slot_clearConnectionSelection();
        slot_clearInfomarkSelection();
    }

    void slot_dataLoaded();
    void slot_moveMarker(RoomId id);

private slots:
    void slot_onMessageLoggedDirect(const QOpenGLDebugMessage &message);
};
