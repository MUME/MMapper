// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "abstractmapstorage.h"

#include "../mapdata/mapdata.h"

#include <optional>
#include <stdexcept>
#include <utility>

#include <QObject>

MapStorageError::~MapStorageError() = default;

AbstractMapStorage::AbstractMapStorage(const Data &data, QObject *const parent)
    : QObject(parent)
    , m_data(data)
{}

AbstractMapStorage::~AbstractMapStorage() = default;

std::optional<RawMapLoadData> AbstractMapStorage::loadData()
{
    if (!canLoad()) {
        throw std::runtime_error("format does not support loading");
    }

    return virt_loadData();
}

bool AbstractMapStorage::saveData(const MapData &mapData, const bool baseMapOnly)
{
    if (!canSave()) {
        throw std::runtime_error("format does not support saving");
    }

    RawMapData rawMapData{};
    rawMapData.mapPair.modified = mapData.getCurrentMap();
    rawMapData.position = mapData.tryGetPosition().value_or(Coordinate{});
    rawMapData.filename = getFilename();
    rawMapData.readonly = false; // otherwise we couldn't save

    if (baseMapOnly) {
        auto &map = rawMapData.mapPair.modified;
        map = map.filterBaseMap(getProgressCounter());
    }

    // REVIST: Convert infomarks to immutable data structure to make this copy O(1).
    if (const InfomarkDb &markers = mapData.getMarkersList(); !markers.empty()) {
        auto &data = rawMapData.markerData.emplace();
        auto &copy = data.markers;
        auto &pc = getProgressCounter();
        pc.setNewTask(ProgressMsg{"copying markers"}, markers.getIdSet().size());
        for (const InfomarkId id : markers.getIdSet()) {
            if (id != INVALID_INFOMARK_ID) {
                copy.emplace_back(markers.getRawCopy(id));
            }
            pc.step();
        }
    }

    return virt_saveData(rawMapData);
}
