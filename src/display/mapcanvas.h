#pragma once
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

#ifndef MAPCANVAS_H
#define MAPCANVAS_H

#include <array>
#include <cstddef>
#include <cstdint>
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
class InfoMark;
class InfoMarksEditDlg;
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
class RoomSelection;
class SigParseEvent;
struct RoomId;

class MapCanvas final : public QOpenGLWidget, private MapCanvasData
{
private:
    Q_OBJECT

private:
    OpenGL m_opengl;
    Mmapper2Group *m_groupManager = nullptr;
    InfoMarksEditDlg *m_infoMarksEditDlg = nullptr;

public:
    explicit MapCanvas(MapData *mapData,
                       PrespammedPath *prespammedPath,
                       Mmapper2Group *groupManager,
                       QWidget *parent);
    ~MapCanvas() override;

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
    void onInfoMarksEditDlgClose();

    void setCanvasMouseMode(CanvasMouseMode);

    void setHorizontalScroll(int x);
    void setVerticalScroll(int y);
    void setScroll(int x, int y);
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void layerUp();
    void layerDown();

    void setRoomSelection(const RoomSelection *);
    void clearRoomSelection() { setRoomSelection(nullptr); }
    void setConnectionSelection(ConnectionSelection *);
    void clearConnectionSelection() { setConnectionSelection(nullptr); }

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

    void newRoomSelection(const RoomSelection *);
    void newConnectionSelection(ConnectionSelection *);

    void setCurrentRoom(RoomId id, bool update);
    void roomPositionChanged();

protected:
    // void closeEvent(QCloseEvent *event);

    void initializeGL() override;
    void paintGL() override;

    void drawGroupCharacters();
    void drawCharacter(const Coordinate &c, const QColor &color);

    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    void drawPathStart(const Coordinate &, std::vector<Vec3d> &verts);
    bool drawPath(const Coordinate &sc,
                  const Coordinate &dc,
                  double &dx,
                  double &dy,
                  double &dz,
                  std::vector<Vec3d> &verts);
    void drawPathEnd(double dx, double dy, double dz, std::vector<Vec3d> &verts);

private:
    QOpenGLDebugLogger *m_logger = nullptr;

    void initTextures();
    void makeGlLists();

    static int inline GLtoMap(float arg);

    void setTrilinear(QOpenGLTexture *x) const;

    void drawPreSpammedPath();
    void paintSelection(GLdouble len);
    void paintSelectedRoom(GLdouble len, const Room *room);
    void paintSelectedConnection();

    void drawRooms(MapCanvasRoomDrawer &drawer);
};
#endif
