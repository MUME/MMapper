#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "../map/Map.h"
#include "../map/infomark.h"
#include "../mapdata/MarkerList.h"

#include <memory>
#include <optional>
#include <vector>

struct NODISCARD RawMarkerData final
{
    std::vector<InfoMarkFields> markers;
    NODISCARD size_t size() const { return markers.size(); }
    NODISCARD bool empty() const { return size() == 0; }
};

struct NODISCARD RawMapLoadData final
{
    std::vector<ExternalRawRoom> rooms;
    std::optional<RawMarkerData> markerData;
    Coordinate position;
    QString filename;
    bool readonly = true;
};

struct NODISCARD MapLoadData final
{
    MapPair mapPair;
    InfomarkDb markerData;
    Coordinate position;
    QString filename;
    bool readonly = true;
};

// TODO: share common data with RawMapLoadData, or better yet: just get rid of this.
struct NODISCARD RawMapData final
{
    MapPair mapPair;
    std::optional<RawMarkerData> markerData;
    Coordinate position;
    QString filename;
    bool readonly = true;
};
