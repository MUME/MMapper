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

#ifndef MMAPPER_MAP_CANVAS_DATA_H
#define MMAPPER_MAP_CANVAS_DATA_H

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
#include "OpenGL.h"
#include "RoadIndex.h"
#include "connectionselection.h"
#include "prespammedpath.h"

class ConnectionSelection;
class MapData;
class PrespammedPath;
class RoomSelection;
class InfoMarkSelection;

/* REVISIT: move this somewhere else? */
static constexpr const float CAMERA_Z_DISTANCE = 0.978f;

template<typename E>
using texture_array = EnumIndexedArray<QOpenGLTexture *, E>;

template<RoadIndexType Type>
struct road_texture_array : private texture_array<RoadIndex>
{
    using base = texture_array<RoadIndex>;
    decltype(auto) operator[](TaggedRoadIndex<Type> x) { return base::operator[](x.index); }
    using base::operator[];
    using base::begin;
    using base::end;
    using base::size;
};

enum class CanvasMouseMode {
    NONE,
    MOVE,
    SELECT_ROOMS,
    SELECT_CONNECTIONS,
    CREATE_ROOMS,
    CREATE_CONNECTIONS,
    CREATE_ONEWAY_CONNECTIONS,
    SELECT_INFOMARKS,
    CREATE_INFOMARKS
};

struct MapCanvasData
{
    static QColor g_noFleeColor;

    explicit MapCanvasData(MapData *mapData, PrespammedPath *prespammedPath, QWidget &sizeWidget)
        : m_sizeWidget(sizeWidget)
        , m_data(mapData)
        , m_prespammedPath(prespammedPath)

    {}

    QMatrix4x4 m_modelview{}, m_projection{};
    QVector3D project(const QVector3D &) const;
    QVector3D unproject(const QVector3D &) const;
    QVector3D unproject(QMouseEvent *event) const;

    QWidget &m_sizeWidget;
    auto width() const { return m_sizeWidget.width(); }
    auto height() const { return m_sizeWidget.height(); }
    auto rect() const { return m_sizeWidget.rect(); }

    struct
    {
        texture_array<RoomTerrainType> terrain{};
        texture_array<RoomLoadFlag> load{};
        texture_array<RoomMobFlag> mob{};
        road_texture_array<RoadIndexType::TRAIL> trail{};
        road_texture_array<RoadIndexType::ROAD> road{};
        QOpenGLTexture *update = nullptr;
    } m_textures{};

    float m_scaleFactor = 1.0f;
    struct
    {
        int x = 0;
        int y = 0;
    } m_scroll{};
    qint16 m_currentLayer = 0;

    struct
    {
        float x = 0.0f;
        float y = 0.0f;
    } m_visible1{}, m_visible2{};

    bool m_mouseRightPressed{false};
    bool m_mouseLeftPressed{false};
    bool m_altPressed{false};
    bool m_ctrlPressed{false};

    CanvasMouseMode m_canvasMouseMode{CanvasMouseMode::MOVE};

    // mouse selection
    struct
    {
        float x = 0.0f;
        float y = 0.0f;
        int layer = 0;
    } m_sel1{}, m_sel2{}, m_moveBackup{};

    bool m_selectedArea = false; // no area selected at start time
    const RoomSelection *m_roomSelection = nullptr;

    struct
    {
        bool inUse = false;
        bool wrongPlace = false;
        int x = 0;
        int y = 0;
    } m_roomSelectionMove{};

    InfoMarkSelection *m_infoMarkSelection = nullptr;

    struct
    {
        bool inUse = false;
        float x = 0;
        float y = 0;
    } m_infoMarkSelectionMove{};

    ConnectionSelection *m_connectionSelection = nullptr;

    MapData *m_data = nullptr;
    PrespammedPath *m_prespammedPath = nullptr;

    struct DrawLists
    {
        EnumIndexedArray<XDisplayList, ExitDirection, NUM_EXITS_NESW> wall{};

        struct ExitUpDown
        {
            struct OpaqueTransparent
            {
                XDisplayList opaque{};
                XDisplayList transparent{};
            } up{}, down{};
        } exit{};

        EnumIndexedArray<XDisplayList, ExitDirection, NUM_EXITS_NESWUD> door{};

        XDisplayList room{};
        struct
        {
            XDisplayList outer{};
            XDisplayList inner{};
        } room_selection{}, character_hint{};

        struct
        {
            EnumIndexedArray<XDisplayList, ExitDirection, NUM_EXITS_NESWUD> begin{};
            EnumIndexedArray<XDisplayList, ExitDirection, NUM_EXITS_NESWUD> end{};
        } flow;
    } m_gllist{};
};

#endif // MMAPPER_MAP_CANVAS_DATA_H
