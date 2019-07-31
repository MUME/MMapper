#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <memory>
#include <optional>
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

template<RoadTagEnum Tag>
struct road_texture_array : private texture_array<RoadIndexMaskEnum>
{
    using base = texture_array<RoadIndexMaskEnum>;
    decltype(auto) operator[](TaggedRoadIndex<Tag> x) { return base::operator[](x.index); }
    using base::operator[];
    using base::begin;
    using base::destroyAll;
    using base::end;
    using base::size;
};

enum class CanvasMouseModeEnum {
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
        texture_array<RoomTerrainEnum> terrain{};
        texture_array<RoomLoadFlagEnum> load{};
        texture_array<RoomMobFlagEnum> mob{};
        road_texture_array<RoadTagEnum::TRAIL> trail{};
        road_texture_array<RoadTagEnum::ROAD> road{};
        std::unique_ptr<QOpenGLTexture> update = nullptr;

        void destroyAll();
    } m_textures{};

    float m_scaleFactor = 1.0f;
    float m_currentStepScaleFactor = 1.0f;
    Coordinate2i m_scroll{};
    qint16 m_currentLayer = 0;
    Coordinate2f m_visible1{}, m_visible2{};

    bool m_mouseRightPressed = false;
    bool m_mouseLeftPressed = false;
    bool m_altPressed = false;
    bool m_ctrlPressed = false;

    CanvasMouseModeEnum m_canvasMouseMode = CanvasMouseModeEnum::MOVE;

    // mouse selection
    MouseSel m_sel1{}, m_sel2{}, m_moveBackup{};

    bool m_selectedArea = false; // no area selected at start time
    SharedRoomSelection m_roomSelection{};

    struct RoomSelMove final
    {
        Coordinate2i pos{};
        bool wrongPlace = false;
        RoomSelMove()
            : pos{}
        {}
    };

    std::optional<RoomSelMove> m_roomSelectionMove{};

    InfoMarkSelection *m_infoMarkSelection = nullptr;

    struct InfoMarkSelectionMove final
    {
        Coordinate2f pos{};
        InfoMarkSelectionMove()
            : pos{}
        {}
    };
    std::optional<InfoMarkSelectionMove> m_infoMarkSelectionMove{};

    ConnectionSelection *m_connectionSelection = nullptr;

    MapData *m_data = nullptr;
    PrespammedPath *m_prespammedPath = nullptr;

    struct DrawLists final
    {
        EnumIndexedArray<XDisplayList, ExitDirEnum, NUM_EXITS_NESW> wall{};

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

        EnumIndexedArray<XDisplayList, ExitDirEnum, NUM_EXITS_NESWUD> door{};

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
            EnumIndexedArray<XDisplayList, ExitDirEnum, NUM_EXITS_NESWUD> begin{};
            EnumIndexedArray<XDisplayList, ExitDirEnum, NUM_EXITS_NESWUD> end{};
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
