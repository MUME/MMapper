#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <vector>
#include <QColor>
#include <QMatrix4x4>
#include <QOpenGLDebugMessage>
#include <QOpenGLFunctions_1_0>
#include <QOpenGLWidget>
#include <QSize>
#include <QString>
#include <QtCore>
#include <QtGlobal>
#include <QtGui/qopengl.h>

#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/parseevent.h"
#include "../global/EnumIndexedArray.h"
#include "../global/Flags.h"
#include "../global/roomid.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/mmapper2exit.h"
#include "../mapdata/mmapper2room.h"
#include "FontFormatFlags.h"
#include "MapCanvasData.h"
#include "MapCanvasRoomDrawer.h"
#include "OpenGL.h"
#include "RoadIndex.h"

class ConnectionSelection;
class Coordinate;
class InfoMarkSelection;
class MapCanvasRoomDrawer;
class MapData;
class Mmapper2Group;
class PrespammedPath;
class QFont;
class QFontMetrics;
class QMouseEvent;
class QObject;
class QOpenGLDebugLogger;
class QOpenGLDebugMessage;
class QOpenGLTexture;
class QWheelEvent;
class QWidget;
class Room;
struct RoomId;

class MapCanvas final : public QOpenGLWidget, private MapCanvasData
{
private:
    Q_OBJECT

private:
    OpenGL m_opengl;
    Mmapper2Group *m_groupManager = nullptr;

public:
    explicit MapCanvas(MapData *mapData,
                       PrespammedPath *prespammedPath,
                       Mmapper2Group *groupManager,
                       QWidget *parent);
    ~MapCanvas() override;

private:
    auto &getOpenGL() { return m_opengl; }
    void cleanupOpenGL();
    void makeCurrentAndUpdate();

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    static float SCROLLFACTOR();
    float getDW() const;
    float getDH() const;

public:
    auto width() const { return QOpenGLWidget::width(); }
    auto height() const { return QOpenGLWidget::height(); }
    auto rect() const { return QOpenGLWidget::rect(); }

public slots:
    void forceMapperToRoom();
    void createRoom();

    void setCanvasMouseMode(CanvasMouseModeEnum);

    void setHorizontalScroll(int x);
    void setVerticalScroll(int y);
    void setScroll(int x, int y);
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void layerUp();
    void layerDown();
    void layerReset();

    void setRoomSelection(const SigRoomSelection &);
    void clearRoomSelection() { setRoomSelection(SigRoomSelection{}); }
    void setConnectionSelection(ConnectionSelection *);
    void clearConnectionSelection() { setConnectionSelection(nullptr); }
    void setInfoMarkSelection(InfoMarkSelection *);
    void clearInfoMarkSelection() { setInfoMarkSelection(nullptr); }

    // void worldChanged();

    void dataLoaded();
    void moveMarker(const Coordinate &);

    void slot_onMessageLoggedDirect(const QOpenGLDebugMessage &message);

signals:
    void onEnsureVisible(qint32 x, qint32 y);
    void onCenter(qint32 x, qint32 y);

    void mapMove(int dx, int dy);
    void setScrollBars(const Coordinate &ulf, const Coordinate &lrb);

    void log(const QString &, const QString &);

    void continuousScroll(qint8, qint8);

    void newRoomSelection(const SigRoomSelection &);
    void newConnectionSelection(ConnectionSelection *);
    void newInfoMarkSelection(InfoMarkSelection *);

    void setCurrentRoom(RoomId id, bool update);

protected:
    // void closeEvent(QCloseEvent *event);

    void initializeGL() override;
    void paintGL() override;

    void drawGroupCharacters();
    void drawCharacter(const Coordinate &c, const QColor &color, bool fill = true);

    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool event(QEvent *e) override;

    void drawPathStart(const Coordinate &, std::vector<Vec3f> &verts, const QColor &color);
    bool drawPath(const Coordinate &sc,
                  const Coordinate &dc,
                  float &dx,
                  float &dy,
                  float &dz,
                  std::vector<Vec3f> &verts);
    void drawPathEnd(float dx, float dy, float dz, std::vector<Vec3f> &verts);

private:
    QOpenGLDebugLogger *m_logger = nullptr;

    void initTextures();
    void makeGlLists();

    void setTrilinear(const std::unique_ptr<QOpenGLTexture> &x) const;

    void drawPreSpammedPath(const Coordinate &c1,
                            const QList<Coordinate> &path,
                            const QColor &color);
    void paintSelection();
    void paintSelectedRooms();
    void paintSelectedRoom(const Room *room);
    void paintSelectedConnection();
    void paintSelectedInfoMarks();
    void paintSelectedInfoMark(const InfoMark *marker);

    void drawRooms(MapCanvasRoomDrawer &drawer);
};
