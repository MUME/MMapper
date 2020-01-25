#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>
#include <QColor>
#include <QMatrix4x4>
#include <QOpenGLDebugMessage>
#include <QOpenGLFunctions_1_0>
#include <QOpenGLWidget>
#include <QtCore>

#include "../mapdata/roomselection.h"
#include "../opengl/Font.h"
#include "../opengl/FontFormatFlags.h"
#include "../opengl/OpenGL.h"
#include "Infomarks.h"
#include "MapCanvasData.h"
#include "MapCanvasRoomDrawer.h"
#include "Textures.h"

class CharacterBatch;
class ConnectionSelection;
class Coordinate;
class InfoMark;
class InfoMarkSelection;
class MapData;
class Mmapper2Group;
class PrespammedPath;
class QMouseEvent;
class QOpenGLDebugLogger;
class QOpenGLDebugMessage;
class QWheelEvent;
class QWidget;
class Room;
struct RoomId;
class RoomSelFakeGL;

class MapCanvas final : public QOpenGLWidget, private MapCanvasViewport, private MapCanvasInputState
{
    Q_OBJECT

private:
    MapScreen m_mapScreen;
    OpenGL m_opengl;
    GLFont m_glFont;
    Batches m_batches;
    MapCanvasTextures m_textures;
    MapData &m_data;

    Mmapper2Group *m_groupManager = nullptr;
    struct OptionStatus final
    {
        std::optional<int> multisampling;
        std::optional<bool> trilinear;
    } graphicsOptionsStatus;

public:
    explicit MapCanvas(MapData *mapData,
                       PrespammedPath *prespammedPath,
                       Mmapper2Group *groupManager,
                       QWidget *parent);
    ~MapCanvas() override;

public:
    static MapCanvas *getPrimary();

private:
    inline auto &getOpenGL() { return m_opengl; }
    inline auto &getGLFont() { return m_glFont; }
    void cleanupOpenGL();

    void initSurface();

public:
    static constexpr int BASESIZE = 528; // REVISIT: Why this size? 16*33 isn't special.
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    static constexpr const int SCROLL_SCALE = 64;
    using MapCanvasViewport::getTotalScaleFactor;
    void setZoom(float zoom)
    {
        m_scaleFactor.set(zoom);
        zoomChanged();
    }
    float getRawZoom() const { return m_scaleFactor.getRaw(); }

public:
    auto width() const { return QOpenGLWidget::width(); }
    auto height() const { return QOpenGLWidget::height(); }
    auto rect() const { return QOpenGLWidget::rect(); }

public slots:
    void forceMapperToRoom();
    void createRoom();

    void setCanvasMouseMode(CanvasMouseModeEnum);

    void setScroll(const glm::vec2 &worldPos);
    // void setScroll(const glm::ivec2 &) = delete; // moc tries to call the wrong one if you define this
    void setHorizontalScroll(float worldX);
    void setVerticalScroll(float worldY);

    void zoomIn();
    void zoomOut();
    void zoomReset();

    void layerUp();
    void layerDown();
    void layerReset();

    void setRoomSelection(const SigRoomSelection &);
    void clearRoomSelection() { setRoomSelection(SigRoomSelection{}); }
    void setConnectionSelection(const std::shared_ptr<ConnectionSelection> &);
    void clearConnectionSelection() { setConnectionSelection(nullptr); }
    void setInfoMarkSelection(const std::shared_ptr<InfoMarkSelection> &);
    void clearInfoMarkSelection() { setInfoMarkSelection(nullptr); }

    void clearAllSelections()
    {
        clearRoomSelection();
        clearConnectionSelection();
        clearInfoMarkSelection();
    }

    void dataLoaded();
    void moveMarker(const Coordinate &);

    void slot_onMessageLoggedDirect(const QOpenGLDebugMessage &message);

signals:
    void sig_onCenter(const glm::vec2 &worldCoord);
    void sig_mapMove(int dx, int dy);
    void sig_setScrollBars(const Coordinate &min, const Coordinate &max);
    void sig_continuousScroll(int, int);

    void log(const QString &, const QString &);

    void newRoomSelection(const SigRoomSelection &);
    void newConnectionSelection(ConnectionSelection *);
    void newInfoMarkSelection(InfoMarkSelection *);

    void setCurrentRoom(RoomId id, bool update);
    void sig_zoomChanged(float);

private:
    void reportGLVersion();
    bool isBlacklistedDriver();

protected:
    void initializeGL() override;
    void paintGL() override;

    void drawGroupCharacters(CharacterBatch &characterBatch);

    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool event(QEvent *e) override;

private:
    std::unique_ptr<QOpenGLDebugLogger> m_logger;
    void initLogger();

    void resizeGL() { resizeGL(width(), height()); }
    void initTextures();
    void updateTextures();
    void updateMultisampling();

    std::shared_ptr<InfoMarkSelection> getInfoMarkSelection(const MouseSel &sel);
    static glm::mat4 getViewProj_old(const glm::vec2 &scrollPos,
                                     const glm::ivec2 &size,
                                     float zoomScale,
                                     int currentLayer);
    static glm::mat4 getViewProj(const glm::vec2 &scrollPos,
                                 const glm::ivec2 &size,
                                 float zoomScale,
                                 int currentLayer);
    void setMvp(const glm::mat4 &viewProj);
    void setViewportAndMvp(int width, int height);

    BatchedInfomarksMeshes getInfoMarksMeshes();
    void drawInfoMark(InfomarksBatch &batch,
                      InfoMark *marker,
                      int currentLayer,
                      const glm::vec2 &offset = {},
                      const std::optional<Color> &overrideColor = std::nullopt);
    void updateBatches();
    void updateMapBatches();
    void updateInfomarkBatches();

    void actuallyPaintGL();
    void paintMap();
    void renderMapBatches();
    void paintBatchedInfomarks();
    void paintSelections();
    void paintSelectionArea();
    void paintNewInfomarkSelection();
    void paintSelectedRooms();
    void paintSelectedRoom(RoomSelFakeGL &, const Room &room);
    void paintSelectedConnection();
    void paintNearbyConnectionPoints();
    void paintSelectedInfoMarks();
    void paintCharacters();

public:
    void mapAndInfomarksChanged();
    void infomarksChanged();
    void layerChanged();
    void mapChanged();
    void requestUpdate();
    void screenChanged();
    void selectionChanged();
    void graphicsSettingsChanged();
    void zoomChanged() { emit sig_zoomChanged(getRawZoom()); }

public:
    void userPressedEscape(bool);
};
