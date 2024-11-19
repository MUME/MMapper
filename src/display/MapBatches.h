#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "Connections.h"
#include "MapCanvasData.h"

#include <map>

template<typename T>
using RoomTintArray = EnumIndexedArray<T, RoomTintEnum, NUM_ROOM_TINTS>;

struct NODISCARD LayerMeshes final
{
    UniqueMeshVector terrain;
    UniqueMeshVector trails;
    RoomTintArray<UniqueMesh> tints;
    UniqueMeshVector overlays;
    UniqueMeshVector doors;
    UniqueMeshVector walls;
    UniqueMeshVector dottedWalls;
    UniqueMeshVector upDownExits;
    UniqueMeshVector streamIns;
    UniqueMeshVector streamOuts;
    UniqueMesh layerBoost;
    bool isValid = false;

    LayerMeshes() = default;
    DEFAULT_MOVES_DELETE_COPIES(LayerMeshes);
    ~LayerMeshes() = default;

    void render(int thisLayer, int focusedLayer);
    explicit operator bool() const { return isValid; }
};

// This must be ordered so we can iterate over the layers from lowest to highest.
using BatchedMeshes = std::map<int, LayerMeshes>;

struct NODISCARD MapBatches final
{
    BatchedMeshes batchedMeshes;
    BatchedConnections connectionDrawerBuffers;
    BatchedConnectionMeshes connectionMeshes;
    BatchedRoomNames roomNameBatches;
    OptBounds redrawMargin;

    MapBatches() = default;
    ~MapBatches() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(MapBatches);
};
