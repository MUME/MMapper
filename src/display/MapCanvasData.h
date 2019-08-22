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
#include "OpenGL.h"
#include "RoadIndex.h"
#include "connectionselection.h"
#include "prespammedpath.h"

class ConnectionSelection;
class MapData;
class PrespammedPath;
class InfoMarkSelection;

/* REVISIT: move this somewhere else? */
static constexpr const float CAMERA_Z_DISTANCE = 0.978f;

template<typename E>
struct texture_array : public EnumIndexedArray<std::unique_ptr<QOpenGLTexture>, E>
{
    ~texture_array() { destroyAll(); }

    void destroyAll()
    {
        for (auto &x : *this)
            x.reset();
    }
};

template<RoadIndexType Type>
struct road_texture_array : private texture_array<RoadIndex>
{
    using base = texture_array<RoadIndex>;
    decltype(auto) operator[](TaggedRoadIndex<Type> x) { return base::operator[](x.index); }
    using base::operator[];
    using base::begin;
    using base::destroyAll;
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

    explicit MapCanvasData(MapData *mapData, PrespammedPath *prespammedPath, QWidget &sizeWidget);
    ~MapCanvasData();

    QMatrix4x4 m_modelview{}, m_projection{};
    QVector3D project(const QVector3D &) const;
    QVector3D unproject(const QVector3D &) const;
    QVector3D unproject(const QMouseEvent *event) const;
    MouseSel getUnprojectedMouseSel(const QMouseEvent *event) const;

    QWidget &m_sizeWidget;
    auto width() const { return m_sizeWidget.width(); }
    auto height() const { return m_sizeWidget.height(); }
    auto rect() const { return m_sizeWidget.rect(); }

    struct Textures final
    {
        texture_array<RoomTerrainType> terrain{};
        texture_array<RoomLoadFlag> load{};
        texture_array<RoomMobFlag> mob{};
        road_texture_array<RoadIndexType::TRAIL> trail{};
        road_texture_array<RoadIndexType::ROAD> road{};
        std::unique_ptr<QOpenGLTexture> update = nullptr;

        void destroyAll();
    } m_textures{};

    float m_scaleFactor = 1.0f;
    float m_currentStepScaleFactor = 1.0f;
    Coordinate2i m_scroll{};
    qint16 m_currentLayer = 0;
    Coordinate2f m_visible1{}, m_visible2{};

    bool m_mouseRightPressed{false};
    bool m_mouseLeftPressed{false};
    bool m_altPressed{false};
    bool m_ctrlPressed{false};

    CanvasMouseMode m_canvasMouseMode{CanvasMouseMode::MOVE};

    // mouse selection
    MouseSel m_sel1{}, m_sel2{}, m_moveBackup{};

    bool m_selectedArea = false; // no area selected at start time
    SharedRoomSelection m_roomSelection{};

    struct RoomSelMove final
    {
        Coordinate2i pos{};
        bool inUse = false;
        bool wrongPlace = false;
    } m_roomSelectionMove{};

    InfoMarkSelection *m_infoMarkSelection = nullptr;

    struct InfoMarkSelectionMove final
    {
        Coordinate2f pos{};
        bool inUse = false;

    } m_infoMarkSelectionMove{};

    ConnectionSelection *m_connectionSelection = nullptr;

    MapData *m_data = nullptr;
    PrespammedPath *m_prespammedPath = nullptr;

    struct DrawLists final
    {
        EnumIndexedArray<XDisplayList, ExitDirection, NUM_EXITS_NESW> wall{};

        struct ExitUpDown final
        {
            struct OpaqueTransparent final
            {
                XDisplayList opaque{};
                XDisplayList transparent{};
                void destroyAll()
                {
                    opaque.destroy();
                    transparent.destroy();
                }
            } up{}, down{};
            void destroyAll()
            {
                up.destroyAll();
                down.destroyAll();
            }
        } exit{};

        EnumIndexedArray<XDisplayList, ExitDirection, NUM_EXITS_NESWUD> door{};

        XDisplayList room{};
        struct
        {
            XDisplayList outline{};
            XDisplayList filled{};

            void destroyAll()
            {
                outline.destroy();
                filled.destroy();
            }
        } room_selection{}, character_hint{};

        struct
        {
            EnumIndexedArray<XDisplayList, ExitDirection, NUM_EXITS_NESWUD> begin{};
            EnumIndexedArray<XDisplayList, ExitDirection, NUM_EXITS_NESWUD> end{};
            void destroyAll()
            {
                for (auto &x : begin)
                    x.destroy();
                for (auto &x : end)
                    x.destroy();
            }
        } flow;

        void destroyAll()
        {
            for (auto &x : wall)
                x.destroy();
            exit.destroyAll();
            for (auto &x : door)
                x.destroy();
            room.destroy();
            room_selection.destroyAll();
            character_hint.destroyAll();
            flow.destroyAll();
        }
    } m_gllist{};

    void destroyAllGLObjects();
};
