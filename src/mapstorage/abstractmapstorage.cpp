// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "abstractmapstorage.h"

#include "../mapdata/mapdata.h"

#include <optional>
#include <stdexcept>

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

    MapLoadData rawMapData{};
    rawMapData.mapPair.modified = mapData.getCurrentMap();
    rawMapData.mapPair.modified.checkConsistency(getProgressCounter());
    rawMapData.position = mapData.tryGetPosition().value_or(Coordinate{});
    rawMapData.filename = getFilename();
    rawMapData.readonly = false; // otherwise we couldn't save

    if (baseMapOnly) {
        auto &map = rawMapData.mapPair.modified;
        map = map.filterBaseMap(getProgressCounter());
    }

    return virt_saveData(rawMapData);
}

const QString &AbstractMapStorage::getFilename() const
{
    if (m_data.saveDestination) {
        return m_data.saveDestination->getFileName();
    }
    return m_data.loadSource->getFileName();
}

QIODevice &AbstractMapStorage::getDevice() const
{
    if (m_data.saveDestination) {
        if (m_data.destinationType == Data::Type::Directory) {
            throw std::runtime_error("QIODevice not available for directory-based saves");
        }
        auto device = m_data.saveDestination->getIODevice();
        if (!device) {
            throw std::runtime_error("No QIODevice available");
        }
        return *device;
    }
    return *m_data.loadSource->getIODevice();
}
