#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
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

#ifndef MMAPPER_MAP_CANVAS_ROOM_DRAWER_H
#define MMAPPER_MAP_CANVAS_ROOM_DRAWER_H

#include <vector>
#include <QColor>
#include <QtCore>

#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/infomark.h"
#include "FontFormatFlags.h"
#include "MapCanvasData.h"
#include "RoadIndex.h"

class InfoMark;
class OpenGL;
class QOpenGLTexture;
class QPaintDevice;
class Room;
class XDisplayList;
struct Vec3d;

/* TODO: move these elsewhere */
static constexpr const double ROOM_Z_DISTANCE = 7.0;
static constexpr const double ROOM_WALL_ALIGN = 0.008;

class MapCanvasRoomDrawer : public MapCanvasData
{
private:
    OpenGL &m_opengl;

public:
    explicit MapCanvasRoomDrawer(const MapCanvasData &data, OpenGL &opengl)
        : MapCanvasData(data)
        , m_opengl(opengl)
    {}

    void drawInfoMark(InfoMark *);

    void drawRoomDoorName(const Room *sourceRoom,
                          ExitDirection sourceDir,
                          const Room *targetRoom,
                          ExitDirection targetDir);
    void drawConnection(const Room *leftRoom,
                        const Room *rightRoom,
                        ExitDirection startDir,
                        ExitDirection endDir,
                        bool oneway,
                        bool inExitFlags = true);
    void drawFlow(const Room *room, const RoomIndex &rooms, ExitDirection exitDirection);
    void drawVertical(const Room *room,
                      const RoomIndex &rooms,
                      qint32 layer,
                      ExitDirection direction,
                      const MapCanvasData::DrawLists::ExitUpDown::OpaqueTransparent &exlists,
                      XDisplayList doorlist);

    void drawExit(const Room *const room, const RoomIndex &rooms, ExitDirection dir);
    void drawRoomConnectionsAndDoors(const Room *room, const RoomIndex &rooms);
    void drawUpperLayers(const Room *room,
                         qint32 layer,
                         const RoadIndex &roadIndex,
                         bool wantExtraDetail);
    void drawInfoMarks();

    void renderText(double x,
                    double y,
                    const QString &text,
                    const QColor &color = Qt::white,
                    FontFormatFlags fontFormatFlag = FontFormatFlags::NONE,
                    double rotationAngle = 0.0);

    void alphaOverlayTexture(QOpenGLTexture *texture);
    void drawLineStrip(const std::vector<Vec3d> &points);
    void drawListWithLineStipple(XDisplayList list, const QColor &color);

    void drawTextBox(const QString &name, qreal x, qreal y, qreal width, qreal height);
    void drawRoom(const Room *room, const RoomIndex &rooms, const RoomLocks &locks);

private:
    void drawConnEndTriUpDownUnknown(qint32 dX, qint32 dY, double dstZ);
    void drawConnEndTriNone(qint32 dX, qint32 dY, double dstZ);

private:
    void drawConnStartTri(ExitDirection startDir, double srcZ);
    void drawConnEndTri(ExitDirection endDir, qint32 dX, qint32 dY, double dstZ);
    void drawConnEndTri1Way(ExitDirection endDir, qint32 dX, qint32 dY, double dstZ);

private:
    void drawConnectionLine(ExitDirection startDir,
                            ExitDirection endDir,
                            bool oneway,
                            bool neighbours,
                            qint32 dX,
                            qint32 dY,
                            double srcZ,
                            double dstZ);

    void drawConnectionTriangles(ExitDirection startDir,
                                 ExitDirection endDir,
                                 bool oneway,
                                 qint32 dX,
                                 qint32 dY,
                                 double srcZ,
                                 double dstZ);

public:
    template<typename T>
    double getScaledFontWidth(T x, FontFormatFlags flags = FontFormatFlags::NONE) const
    {
        return m_opengl.getFontWidth(x, flags)
               * static_cast<double>(0.022f / m_scaleFactor * m_currentStepScaleFactor);
    }

    double getScaledFontHeight() const
    {
        return m_opengl.getFontHeight()
               * static_cast<double>(0.007f / m_scaleFactor * m_currentStepScaleFactor);
    }
};

#endif // MMAPPER_MAP_CANVAS_ROOM_DRAWER_H
