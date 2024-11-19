#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/room.h"
#include "../global/Array.h"
#include "../global/roomid.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/infomark.h"
#include "../opengl/Font.h"
#include "Connections.h"
#include "Infomarks.h"
#include "MapBatches.h"
#include "MapCanvasData.h"
#include "RoadIndex.h"

#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QColor>
#include <QtCore>

class InfoMark;
class MapCanvasRoomDrawer;
struct MapCanvasTextures;
class OpenGL;
class QOpenGLTexture;
class Room;

using RoomVector = std::vector<const Room *>;
using LayerToRooms = std::map<int, RoomVector>;

struct NODISCARD Batches final
{
    std::optional<MapBatches> mapBatches;
    std::optional<BatchedInfomarksMeshes> infomarksMeshes;

    Batches() = default;
    ~Batches() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(Batches);

    void resetAll()
    {
        mapBatches.reset();
        infomarksMeshes.reset();
    }
};

class NODISCARD MapCanvasRoomDrawer final
{
private:
    OpenGL &m_opengl;
    GLFont &m_font;
    const MapCanvasTextures &m_textures;
    std::optional<MapBatches> &m_batches;

public:
    explicit MapCanvasRoomDrawer(const MapCanvasTextures &textures,
                                 OpenGL &opengl,
                                 GLFont &font,
                                 std::optional<MapBatches> &batches)
        : m_opengl{opengl}
        , m_font{font}
        , m_textures{textures}
        , m_batches{batches}
    {}

private:
    NODISCARD auto &getOpenGL() const { return m_opengl; }

public:
    void generateBatches(const LayerToRooms &layerToRooms,
                         const RoomIndex &roomIndex,
                         const OptBounds &bounds);

public:
    NODISCARD inline GLFont &getFont() { return m_font; }
};
