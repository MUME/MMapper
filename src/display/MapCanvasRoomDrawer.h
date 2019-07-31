#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <map>
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
struct Vec3f;

/* TODO: move these elsewhere */
static constexpr const float ROOM_Z_DISTANCE = 7.0f;
static constexpr const float ROOM_Z_LAYER_BUMP = 0.0001f;
static constexpr const float ROOM_BOOST_BUMP = 0.01f;
static constexpr const float ROOM_WALLS_BUMP = 0.009f; // was 0.005f but should be below boost

using RoomVector = std::vector<const Room *>;
using LayerToRooms = std::map<int, RoomVector>;

class MapCanvasRoomDrawer final
{
private:
    const MapCanvasData &m_mapCanvasData;
    OpenGL &m_opengl;
    const Coordinate2f &m_visible1;
    const Coordinate2f &m_visible2;
    const qint16 &m_currentLayer;
    const float &m_scaleFactor;
    const MapCanvasData::DrawLists &m_gllist;
    const MapCanvasData::Textures &m_textures;

public:
    explicit MapCanvasRoomDrawer(const MapCanvasData &data, OpenGL &opengl)
        : m_mapCanvasData(data)
        , m_opengl(opengl)
        , m_visible1{m_mapCanvasData.m_visible1}
        , m_visible2{m_mapCanvasData.m_visible2}
        , m_currentLayer{m_mapCanvasData.m_currentLayer}
        , m_scaleFactor{m_mapCanvasData.m_scaleFactor}
        , m_gllist{m_mapCanvasData.m_gllist}
        , m_textures{m_mapCanvasData.m_textures}
    {}

private:
    auto &getOpenGL() const { return m_opengl; }

public:
    void drawRooms(const LayerToRooms &layerToRooms,
                   const RoomIndex &roomIndex,
                   const RoomLocks &locks);
    void drawWallsAndExits(const Room *room, const RoomIndex &rooms);

    void drawInfoMark(InfoMark *);

    void drawRoomDoorName(const Room *sourceRoom,
                          ExitDirEnum sourceDir,
                          const Room *targetRoom,
                          ExitDirEnum targetDir);
    void drawConnection(const Room *leftRoom,
                        const Room *rightRoom,
                        ExitDirEnum startDir,
                        ExitDirEnum endDir,
                        bool oneway,
                        bool inExitFlags = true);
    void drawFlow(const Room *room, const RoomIndex &rooms, ExitDirEnum exitDirection);
    void drawVertical(const Room *room,
                      const RoomIndex &rooms,
                      qint32 layer,
                      ExitDirEnum direction,
                      const MapCanvasData::DrawLists::ExitUpDown::OpaqueTransparent &exlists,
                      const XDisplayList &doorlist);

    void drawExit(const Room *room, const RoomIndex &rooms, qint32 layer, ExitDirEnum dir);
    void drawRoomConnectionsAndDoors(const Room *room, const RoomIndex &rooms);
    void drawInfoMarks();

    void renderText(float x,
                    float y,
                    const QString &text,
                    const QColor &color = Qt::white,
                    const FontFormatFlags &fontFormatFlags = {},
                    float rotationAngle = 0.0f);

    void alphaOverlayTexture(QOpenGLTexture *texture);
    void drawLineStrip(const std::vector<Vec3f> &points);
    void drawListWithLineStipple(const XDisplayList &list, const QColor &color);

    void drawTextBox(const QString &name, float x, float y, float width, float height);
    void drawRoom(const Room *room, bool wantExtraDetail);
    void drawBoost(const Room *room, const RoomLocks &locks);

private:
    void drawConnEndTriUpDownUnknown(qint32 dX, qint32 dY, float dstZ);
    void drawConnEndTriNone(qint32 dX, qint32 dY, float dstZ);

private:
    void drawConnStartTri(ExitDirEnum startDir, float srcZ);
    void drawConnEndTri(ExitDirEnum endDir, qint32 dX, qint32 dY, float dstZ);
    void drawConnEndTri1Way(ExitDirEnum endDir, qint32 dX, qint32 dY, float dstZ);

private:
    void drawConnectionLine(ExitDirEnum startDir,
                            ExitDirEnum endDir,
                            bool oneway,
                            bool neighbours,
                            qint32 dX,
                            qint32 dY,
                            float srcZ,
                            float dstZ);

    void drawConnectionTriangles(ExitDirEnum startDir,
                                 ExitDirEnum endDir,
                                 bool oneway,
                                 qint32 dX,
                                 qint32 dY,
                                 float srcZ,
                                 float dstZ);

public:
    float getScaledFontWidth(const QString &x, const FontFormatFlags &flags = {}) const;
    float getScaledFontHeight() const;
};
