#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "MapSource.h"
#include "RawMapData.h"
#include "abstractmapstorage.h"

#include <memory>
#include <optional>

#include <QObject>

// Widget-free map-storage helpers shared by MainWindow's async load path
// (see mainwindow/mainwindow-async.cpp's getLoadOrMergeMapStorage()/
// background::load_map_data(), which delegate here).
//
// Neither function here touches QtWidgets, QProgressDialog, or MainWindow --
// they only need a QObject* to parent the storage object to (for lifetime/
// sig_log wiring, which callers still do themselves) and an
// AbstractMapStorage& with its ProgressCounter already attached. That keeps
// the format detection and background load logic reusable by any caller,
// without pulling in MainWindow's QProgressDialog/CanvasDisabler/
// ExtraBlockers machinery.
namespace maploadhelper {

// Sniffs pSource's IODevice against every known map format (MMapper2 binary,
// MMapper2/Pandora XML) and constructs the matching AbstractMapStorage
// subclass, parented to `parent`. Throws std::runtime_error("Unrecognized
// file format") if none match. Callers are responsible for connecting
// AbstractMapStorage::sig_log to their own log sink.
NODISCARD std::unique_ptr<AbstractMapStorage> detectAndCreateStorage(
    const std::shared_ptr<MapSource> &pSource, QObject *parent);

// Loads raw map data from storage and constructs a Map from it. Safe to call
// on a background thread; storage's ProgressCounter must already be set (via
// AbstractMapStorage::setProgressCounter()) before calling this.
NODISCARD std::optional<MapLoadData> loadMapData(AbstractMapStorage &storage);

// Loads raw map data from storage and merges it into currentMap. Safe to
// call on a background thread; storage's ProgressCounter must already be
// set before calling this. Returns std::nullopt if storage can't load, or
// if the loaded data has neither rooms nor markers.
NODISCARD std::optional<Map> mergeMapData(AbstractMapStorage &storage, const Map &currentMap);

} // namespace maploadhelper
