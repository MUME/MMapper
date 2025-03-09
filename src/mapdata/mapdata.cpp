// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mapdata.h"

#include "../display/MapCanvasRoomDrawer.h"
#include "../display/Textures.h"
#include "../global/AnsiOstream.h"
#include "../global/Timer.h"
#include "../global/logging.h"
#include "../global/progresscounter.h"
#include "../global/utils.h"
#include "../map/CommandId.h"
#include "../map/ExitDirection.h"
#include "../map/ExitFieldVariant.h"
#include "../map/RawRoom.h"
#include "../map/RoomRecipient.h"
#include "../map/World.h"
#include "../map/coordinate.h"
#include "../map/exit.h"
#include "../map/infomark.h"
#include "../map/mmapper2room.h"
#include "../map/room.h"
#include "../map/roomid.h"
#include "../mapfrontend/mapfrontend.h"
#include "../mapstorage/RawMapData.h"
#include "GenericFind.h"
#include "roomfilter.h"
#include "roomselection.h"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <sstream>
#include <tuple>
#include <vector>

#include <QApplication>
#include <QList>
#include <QString>

MapData::MapData(QObject *const parent)
    : MapFrontend(parent)
{}

DoorName MapData::getDoorName(const RoomId id, const ExitDirEnum dir)
{
    if (dir < ExitDirEnum::UNKNOWN) {
        if (auto optDoorName = getCurrentMap().findDoorName(id, dir)) {
            return *optDoorName;
        }
    }

    static const DoorName tmp{"exit"};
    return tmp;
}

ExitDirFlags MapData::getExitDirections(const Coordinate &pos)
{
    if (const auto room = findRoomHandle(pos)) {
        return computeExitDirections(room.getRaw());
    }
    return {};
}

template<typename Callback>
static void walk_path(const RoomHandle &input_room, const CommandQueue &dirs, Callback &&callback)
{
    const auto &map = input_room.getMap();
    auto room = input_room; // Caution: This is reassigned below.
    for (const CommandEnum cmd : dirs) {
        if (cmd == CommandEnum::LOOK) {
            continue;
        }

        if (!isDirectionNESWUD(cmd)) {
            break;
        }

        const auto &e = room.getExit(getDirection(cmd));
        if (!e.exitIsExit()) {
            // REVISIT: why does this continue but all of the others break?
            continue;
        }

        // REVISIT: if it's more than one, why not just pick one?
        if (e.getOutgoingSet().size() != 1) {
            break;
        }

        const RoomId next = e.getOutgoingSet().first();

        // NOTE: reassignment.
        room = map.getRoomHandle(next);

        callback(room.getRaw());
    }
}

std::vector<Coordinate> MapData::getPath(const RoomId start, const CommandQueue &dirs) const
{
    if (start == INVALID_ROOMID) {
        return {};
    }

    std::vector<Coordinate> ret;
    ret.reserve(static_cast<size_t>(dirs.size()));

    const Map &map = getCurrentMap();
    if (const auto from = map.findRoomHandle(start)) {
        walk_path(from, dirs, [&ret](const RawRoom &room) { ret.push_back(room.getPosition()); });
    }
    return ret;
}

std::optional<RoomId> MapData::getLast(const RoomId start, const CommandQueue &dirs) const
{
    if (start == INVALID_ROOMID) {
        return std::nullopt;
    }

    std::optional<RoomId> ret;
    const Map &map = getCurrentMap();
    if (const auto from = map.findRoomHandle(start)) {
        walk_path(from, dirs, [&ret](const RawRoom &room) { ret = room.getId(); });
    }
    return ret;
}

FutureSharedMapBatchFinisher MapData::generateBatches(const mctp::MapCanvasTexturesProxy &textures)
{
    return generateMapDataFinisher(textures, getCurrentMap());
}

void MapData::applyChangesToList(const RoomSelection &sel,
                                 const std::function<Change(const RawRoom &)> &callback)
{
    ChangeList changes;
    for (const RoomId id : sel) {
        if (const auto ptr = findRoomHandle(id)) {
            changes.add(callback(ptr.getRaw()));
        }
    }
    applyChanges(changes);
}

void MapData::virt_clear()
{
    log("cleared MapData");
}

void MapData::removeDoorNames(ProgressCounter &pc)
{
    this->applySingleChange(pc, Change{world_change_types::RemoveAllDoorNames{}});
}

void MapData::generateBaseMap(ProgressCounter &pc)
{
    this->applySingleChange(pc, Change{world_change_types::GenerateBaseMap{}});
}

NODISCARD RoomIdSet MapData::genericFind(const RoomFilter &f) const
{
    return ::genericFind(getCurrentMap(), f);
}

MapData::~MapData() = default;

bool MapData::removeMarker(const InfomarkId id)
{
    try {
        auto db = getInfomarkDb();
        db.removeMarker(id);
        setCurrentMarks(db);
        return true;
    } catch (const std::runtime_error & /*ex*/) {
        return false;
    }
}

void MapData::removeMarkers(const MarkerList &toRemove)
{
    try {
        auto db = getInfomarkDb();
        for (const InfomarkId id : toRemove) {
            // REVISIT: try-catch around each one and report if any failed, instead of this all-or-nothing approach?
            db.removeMarker(id);
        }
        setCurrentMarks(db);
    } catch (const std::runtime_error &ex) {
        MMLOG() << "ERROR removing multiple infomarks: " << ex.what();
    }
}

InfomarkId MapData::addMarker(const InfoMarkFields &im)
{
    try {
        auto db = getInfomarkDb();
        auto id = db.addMarker(im);
        setCurrentMarks(db);
        return id;
    } catch (const std::runtime_error &ex) {
        MMLOG() << "ERROR adding infomark: " << ex.what();
        return INVALID_INFOMARK_ID;
    }
}

bool MapData::updateMarker(const InfomarkId id, const InfoMarkFields &im)
{
    try {
        auto db = getInfomarkDb();
        auto modified = db.updateMarker(id, im);
        if (modified) {
            setCurrentMarks(db, modified);
        }
        return true;
    } catch (const std::runtime_error &ex) {
        MMLOG() << "ERROR updating infomark: " << ex.what();
        return false;
    }
}

bool MapData::updateMarkers(const std::vector<InformarkChange> &updates)
{
    try {
        auto db = getInfomarkDb();
        auto modified = db.updateMarkers(updates);
        if (modified) {
            setCurrentMarks(db, modified);
        }
        return true;
    } catch (const std::runtime_error &ex) {
        MMLOG() << "ERROR updating infomarks: " << ex.what();
        return false;
    }
}

void MapData::slot_scheduleAction(const SigMapChangeList &change)
{
    this->applyChanges(change.deref());
}

bool MapData::isEmpty() const
{
    // return (greatestUsedId == INVALID_ROOMID) && m_markers.empty();
    return getCurrentMap().empty() && getInfomarkDb().empty();
}

void MapData::removeMissing(RoomIdSet &set) const
{
    RoomIdSet invalid;

    for (const RoomId id : set) {
        if (!findRoomHandle(id)) {
            invalid.insert(id);
        }
    }

    for (const RoomId id : invalid) {
        set.erase(id);
    }
}

void MapData::setMapData(const MapLoadData &mapLoadData)
{
    // TODO: make all of this an all-or-nothing commit;
    // for now the catch + abort has that effect.
    try {
        MapFrontend &mf = *this;
        mf.block();
        {
            InfomarkDb markers = mapLoadData.markerData;
            setFileName(mapLoadData.filename, mapLoadData.readonly);
            setSavedMap(mapLoadData.mapPair.base);
            setCurrentMap(mapLoadData.mapPair.modified);
            setCurrentMarks(markers);
            setSavedMarks(markers);
            forcePosition(mapLoadData.position);

            // NOTE: The map may immediately report changes.
        }
        mf.unblock();
    } catch (...) {
        // REVISIT: should this be fatal, or just throw?
        qFatal("An exception occured while setting the map data.");
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#endif
        // the following is in case qFatal() isn't actually noreturn.
        // NOLINTNEXTLINE
        std::abort();
#ifdef __clang__
#pragma clang diagnostic pop
#endif
    }
}

// TODO: implement a better merge!
// The old "merge" algorithm was really unsophisticated;
// it just inserted map into with a position and ID offset.
//
// A better approach would be to look for the common subset,
// and then look for and prompt the user to approve changes like:
//  * typo-fixes
//  * flag changes
//  * added or removed door names
//  * added / removed connections within the common subset
//
// Finally, accept any additions, but do so at offset and nextid.
std::pair<Map, InfomarkDb> MapData::mergeMapData(ProgressCounter &counter,
                                                 const Map &currentMap,
                                                 const InfomarkDb &currentMarks,
                                                 RawMapLoadData newMapData)
{
    const Bounds newBounds = [&newMapData]() {
        const auto &rooms = newMapData.rooms;
        const auto &front = rooms.front().position;
        Bounds bounds{front, front};
        for (const auto &room : rooms) {
            bounds.insert(room.getPosition());
        }
        return bounds;
    }();

    const Coordinate mapOffset = [&currentMap, &newBounds]() -> Coordinate {
        const auto currentBounds = currentMap.getBounds().value();

        // NOTE: current and new map origins may not be at the same place relative to the bounds,
        // so we'll use the upper bound of the current map and the lower bound of the new map
        // to compute the offset.

        constexpr int margin = 1;
        Coordinate tmp = currentBounds.max - newBounds.min + Coordinate{1, 1, 0} * margin;
        // NOTE: The reason it's at a z=-1 offset is so the manual "merge up" command will work.
        tmp.z = -1;

        return tmp;
    }();

    const auto infomarkOffset = [&mapOffset]() -> Coordinate {
        const auto tmp = mapOffset.to_ivec3() * glm::ivec3{INFOMARK_SCALE, INFOMARK_SCALE, 1};
        return Coordinate{tmp.x, tmp.y, tmp.z};
    }();

    const Map newMap = Map::merge(counter, currentMap, std::move(newMapData.rooms), mapOffset);

    const InfomarkDb newMarks = [&newMapData, &currentMarks, &infomarkOffset, &counter]() {
        auto tmp = currentMarks;
        if (newMapData.markerData) {
            const auto &markers = newMapData.markerData.value().markers;
            counter.setNewTask(ProgressMsg{"adding infomarks"}, markers.size());
            for (const InfoMarkFields &mark : markers) {
                auto copy = mark.getOffsetCopy(infomarkOffset);
                std::ignore = tmp.addMarker(copy);
                counter.step();
            }
        }
        return tmp;
    }();

    return std::pair<Map, InfomarkDb>(newMap, newMarks);
}

void MapData::describeChanges(std::ostream &os) const
{
    if (!MapFrontend::isModified()) {
        os << "No changes since the last save.\n";
        return;
    }

    {
        const Map &savedMap = getSavedMap();
        const Map &currentMap = getCurrentMap();
        if (savedMap != currentMap) {
            const auto stats = getBasicDiffStats(savedMap, currentMap);
            auto printRoomDiff = [&os](const std::string_view what, const size_t count) {
                if (count == 0) {
                    return;
                }
                os << "Rooms " << what << ": " << count << ".\n";
            };
            printRoomDiff("removed", stats.numRoomsRemoved);
            printRoomDiff("added", stats.numRoomsAdded);
            printRoomDiff("changed", stats.numRoomsChanged);
        }

        if (getSavedMarks() != getCurrentMarks()) {
            // REVISIT: Can we get a better description of what changed?
            os << "Infomarks have changed.\n";
        }

        // REVISIT: Should we also include the time of the last update?
    }
}

std::string MapData::describeChanges() const
{
    std::ostringstream os;
    describeChanges(os);
    return std::move(os).str();
}
