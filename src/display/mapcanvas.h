#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/ChangeMonitor.h"
#include "../global/Signal2.h"
#include "../mapdata/roomselection.h"
#include "../opengl/Font.h"
#include "../opengl/FontFormatFlags.h"
#include "../opengl/OpenGL.h"
#include "Infomarks.h"
#include "MapCanvasData.h"
#include "MapCanvasRoomDrawer.h"
#include "Textures.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

#include <glm/glm.hpp>

#include <QColor>
#include <QMatrix4x4>
#include <QOpenGLDebugMessage>
#include <QOpenGLWidget>
#include <QtCore>

class CharacterBatch;
class ConnectionSelection;
class Coordinate;
class InfomarkSelection;
class MapData;
class Mmapper2Group;
class PrespammedPath;
class QMouseEvent;
class QOpenGLDebugLogger;
class QOpenGLDebugMessage;
class QWheelEvent;
class QWidget;
class RoomSelFakeGL;

class NODISCARD_QOBJECT MapCanvas final : public QOpenGLWidget,
                                          private MapCanvasViewport,
                                          private MapCanvasInputState
{
    Q_OBJECT

public:
    static constexpr const int BASESIZE = 528; // REVISIT: Why this size? 16*33 isn't special.
    static constexpr const int SCROLL_SCALE = 64;

private:
    struct NODISCARD FrameRateController final
    {
        std::chrono::steady_clock::time_point lastFrameTime;
        bool animating = false;
    };

    struct NODISCARD Diff final
    {
        struct NODISCARD MaybeDataOrMesh final
            : public std::variant<std::monostate, ColoredTexVertVector, UniqueMesh>
        {
        public:
            using base = std::variant<std::monostate, ColoredTexVertVector, UniqueMesh>;
            using base::base;

        public:
            NODISCARD bool empty() const { return std::holds_alternative<std::monostate>(*this); }
            NODISCARD bool hasData() const
            {
                return std::holds_alternative<ColoredTexVertVector>(*this);
            }
            NODISCARD bool hasMesh() const { return std::holds_alternative<UniqueMesh>(*this); }

        public:
            NODISCARD const ColoredTexVertVector &getData() const
            {
                return std::get<ColoredTexVertVector>(*this);
            }
            NODISCARD const UniqueMesh &getMesh() const { return std::get<UniqueMesh>(*this); }

            void render(OpenGL &gl, const MMTextureId texId)
            {
                if (empty()) {
                    assert(false);
                    return;
                }

                if (hasData()) {
                    *this = gl.createColoredTexturedQuadBatch(getData(), texId);
                    assert(hasMesh());
                    // REVISIT: rendering immediately after uploading the mesh may lag,
                    // so consider delaying until the data is already on the GPU.
                }

                if (!hasMesh()) {
                    assert(false);
                    return;
                }
                auto &mesh = getMesh();
                mesh.render(GLRenderState().withBlend(BlendModeEnum::TRANSPARENCY));
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
    OpenGL m_opengl;
    GLFont m_glFont;
    Batches m_batches;
    MapCanvasTextures m_textures;
    MapData &m_data;
    Mmapper2Group &m_groupManager;
    Diff m_diff;
    FrameRateController m_frameRateController;
    std::unique_ptr<QOpenGLDebugLogger> m_logger;
    Signal2Lifetime m_lifetime;

    struct AltDragState
    {
        QPoint lastPos;
        QCursor originalCursor;
    };
    std::optional<AltDragState> m_altDragState;

public:
    explicit MapCanvas(MapData &mapData,
                       PrespammedPath &prespammedPath,
                       Mmapper2Group &groupManager,
                       QWidget *parent);
    ~MapCanvas() final;

public:
    NODISCARD static MapCanvas *getPrimary();

private:
    NODISCARD inline auto &getOpenGL() { return m_opengl; }
    NODISCARD inline auto &getGLFont() { return m_glFont; }
    void cleanupOpenGL();

public:
    NODISCARD QSize minimumSizeHint() const override;
    NODISCARD QSize sizeHint() const override;

    using MapCanvasViewport::getTotalScaleFactor;
    void setZoom(float zoom)
    {
        m_scaleFactor.set(zoom);
        zoomChanged();
    }
    NODISCARD float getRawZoom() const { return m_scaleFactor.getRaw(); }

public:
    NODISCARD auto width() const { return QOpenGLWidget::width(); }
    NODISCARD auto height() const { return QOpenGLWidget::height(); }
    NODISCARD auto rect() const { return QOpenGLWidget::rect(); }

private:
    void onMovement();

private:
    void reportGLVersion();
    NODISCARD bool isBlacklistedDriver();

protected:
    void initializeGL() override;
    void paintGL() override;
    void paintEvent(QPaintEvent *event) override;

    void drawGroupCharacters(CharacterBatch &characterBatch);

    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool event(QEvent *e) override;

private:
    void setAnimating(bool value);
    void renderLoop();

private:
    void initLogger();

    void resizeGL() { resizeGL(width(), height()); }
    void initTextures();
    void updateTextures();
    void updateMultisampling();

    NODISCARD std::shared_ptr<InfomarkSelection> getInfomarkSelection(const MouseSel &sel);
    NODISCARD static glm::mat4 getViewProj_old(const glm::vec2 &scrollPos,
                                               const glm::ivec2 &size,
                                               float zoomScale,
                                               int currentLayer);
    NODISCARD static glm::mat4 getViewProj(const glm::vec2 &scrollPos,
                                           const glm::ivec2 &size,
                                           float zoomScale,
                                           int currentLayer);
    void setMvp(const glm::mat4 &viewProj);
    void setViewportAndMvp(int width, int height);

    NODISCARD BatchedInfomarksMeshes getInfomarksMeshes();
    void drawInfomark(InfomarksBatch &batch,
                      const InfomarkHandle &marker,
                      int currentLayer,
                      const glm::vec2 &offset = {},
                      const std::optional<Color> &overrideColor = std::nullopt);
    void updateBatches();
    void finishPendingMapBatches();
    void updateMapBatches();
    void updateInfomarkBatches();

    void actuallyPaintGL(double texturesMs, double batchesMs);
    void renderPerfStats(double texturesMs, double batchesMs, double paintMs);
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
    void forceUpdateMeshes();

public:
    void slot_rebuildMeshes() { forceUpdateMeshes(); }
    void infomarksChanged();
    void layerChanged();
    void slot_mapChanged();
    void slot_requestUpdate();
    void screenChanged();
    void selectionChanged();
    void graphicsSettingsChanged();
    void zoomChanged() { emit sig_zoomChanged(getRawZoom()); }

public:
    void userPressedEscape(bool);

private:
    void log(const QString &msg) { emit sig_log("MapCanvas", msg); }

signals:
    void sig_onCenter(const glm::vec2 &worldCoord);
    void sig_mapMove(int dx, int dy);
    void sig_setScrollBars(const Coordinate &min, const Coordinate &max);
    void sig_continuousScroll(int, int);

    void sig_log(const QString &, const QString &);

    void sig_selectionChanged();
    void sig_newRoomSelection(const SigRoomSelection &);
    void sig_newConnectionSelection(ConnectionSelection *);
    void sig_newInfomarkSelection(InfomarkSelection *);

    void sig_setCurrentRoom(RoomId id, bool update);
    void sig_zoomChanged(float);

public slots:
    void slot_onForcedPositionChange();
    void slot_createRoom();

    void slot_setCanvasMouseMode(CanvasMouseModeEnum mode);

    void slot_setScroll(const glm::vec2 &worldPos);
    // void setScroll(const glm::ivec2 &) = delete; // moc tries to call the wrong one if you define this
    void slot_setHorizontalScroll(float worldX);
    void slot_setVerticalScroll(float worldY);

    void slot_zoomIn();
    void slot_zoomOut();
    void slot_zoomReset();

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

    void slot_onMessageLoggedDirect(const QOpenGLDebugMessage &message);
};
