// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mapdata.h"

#include "../clock/mumeclock.h"
#include "../clock/mumemoment.h"
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
#include <QTimer>

MapData::MapData(MumeClock &mumeClock, QObject *const parent)
    : MapFrontend(parent)
    , m_mumeClock(mumeClock)
{
    // Set up timer to periodically check for day/night transitions
    // Check every 10 seconds (MUME time changes relatively slowly)
    m_lightingUpdateTimer = new QTimer(this);
    connect(m_lightingUpdateTimer, &QTimer::timeout, this, &MapData::slot_checkTimeOfDay);
    m_lightingUpdateTimer->start(10000); // 10 seconds
}

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

void MapData::setRoom(const RoomId id)
{
    auto before = m_selectedRoom;
    if (const auto &room = findRoomHandle(id)) {
        m_selectedRoom = id;
    } else {
        clearSelectedRoom();
    }
    if (before != m_selectedRoom) {
        emit sig_onPositionChange();

        // Regenerate meshes when player moves (the lit player room changes position)
        // or when nighttime (outdoor rooms affected by player position)
        const bool playerHasLight = m_currentPromptFlags.isValid()
                                    && (m_currentPromptFlags.isLit()
                                        || !m_currentPromptFlags.isDark());
        const MumeMoment moment = m_mumeClock.getMumeMoment();
        const MumeTimeEnum timeOfDay = moment.toTimeOfDay();
        const bool isNight = (timeOfDay == MumeTimeEnum::NIGHT || timeOfDay == MumeTimeEnum::DUSK);

        if (playerHasLight || isNight) {
            log(QString("Player moved to new room, regenerating meshes"));
            notifyModified(RoomUpdateFlags{RoomUpdateEnum::RoomMeshNeedsUpdate});
        }
    }
}

void MapData::setPosition(const Coordinate &pos)
{
    if (const auto &room = findRoomHandle(pos)) {
        setRoom(room.getId());
    } else {
        auto before = m_selectedRoom;
        clearSelectedRoom();
        if (before != m_selectedRoom) {
            emit sig_onPositionChange();

            // Regenerate meshes when player position changes
            const bool playerHasLight = m_currentPromptFlags.isValid()
                                        && (m_currentPromptFlags.isLit()
                                            || !m_currentPromptFlags.isDark());
            const MumeMoment moment = m_mumeClock.getMumeMoment();
            const MumeTimeEnum timeOfDay = moment.toTimeOfDay();
            const bool isNight = (timeOfDay == MumeTimeEnum::NIGHT || timeOfDay == MumeTimeEnum::DUSK);

            if (playerHasLight || isNight) {
                log(QString("Player position cleared, regenerating meshes"));
                notifyModified(RoomUpdateFlags{RoomUpdateEnum::RoomMeshNeedsUpdate});
            }
        }
    }
}

void MapData::forcePosition(const Coordinate &pos)
{
    if (const auto &room = findRoomHandle(pos)) {
        forceToRoom(room.getId());
    } else {
        auto before = m_selectedRoom;
        clearSelectedRoom();
        if (before != m_selectedRoom) {
            emit sig_onForcedPositionChange();

            // Regenerate meshes when player position changes
            const bool playerHasLight = m_currentPromptFlags.isValid()
                                        && (m_currentPromptFlags.isLit()
                                            || !m_currentPromptFlags.isDark());
            const MumeMoment moment = m_mumeClock.getMumeMoment();
            const MumeTimeEnum timeOfDay = moment.toTimeOfDay();
            const bool isNight = (timeOfDay == MumeTimeEnum::NIGHT || timeOfDay == MumeTimeEnum::DUSK);

            if (playerHasLight || isNight) {
                log(QString("Player forced position cleared, regenerating meshes"));
                notifyModified(RoomUpdateFlags{RoomUpdateEnum::RoomMeshNeedsUpdate});
            }
        }
    }
}

void MapData::slot_checkTimeOfDay()
{
    const MumeMoment moment = m_mumeClock.getMumeMoment();
    const MumeTimeEnum timeOfDay = moment.toTimeOfDay();

    // Helper function to determine if a time is "night" (dark) or "day" (bright)
    auto isNightTime = [](MumeTimeEnum time) {
        return time == MumeTimeEnum::NIGHT || time == MumeTimeEnum::DUSK;
    };

    // Check if we've transitioned between day and night
    if (m_lastTimeOfDay != MumeTimeEnum::UNKNOWN) {
        const bool wasNight = isNightTime(m_lastTimeOfDay);
        const bool isNight = isNightTime(timeOfDay);

        if (wasNight != isNight) {
            log(QString("Day/Night transition detected (%1 -> %2), regenerating meshes")
                    .arg(wasNight ? "NIGHT" : "DAY")
                    .arg(isNight ? "NIGHT" : "DAY"));
            m_lastTimeOfDay = timeOfDay;
            notifyModified(RoomUpdateFlags{RoomUpdateEnum::RoomMeshNeedsUpdate});
        }
    } else {
        // Initialize on first check
        m_lastTimeOfDay = timeOfDay;
    }
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

FutureSharedMapBatchFinisher MapData::generateBatches(const mctp::MapCanvasTexturesProxy &textures,
                                                      const std::shared_ptr<const FontMetrics> &font)
{
    // Get current time of day from MumeClock
    const MumeMoment moment = m_mumeClock.getMumeMoment();
    const MumeTimeEnum timeOfDay = moment.toTimeOfDay();

    // Debug: Log current time and lighting state
    static MumeTimeEnum lastLoggedTime = MumeTimeEnum::UNKNOWN;
    if (timeOfDay != lastLoggedTime) {
        const char *timeStr = (timeOfDay == MumeTimeEnum::NIGHT) ? "NIGHT"
                            : (timeOfDay == MumeTimeEnum::DAWN)  ? "DAWN"
                            : (timeOfDay == MumeTimeEnum::DAY)   ? "DAY"
                            : (timeOfDay == MumeTimeEnum::DUSK)  ? "DUSK"
                                                                 : "UNKNOWN";
        log(QString("Time of day changed to: %1").arg(timeStr));
        lastLoggedTime = timeOfDay;
    }

    // Helper function to determine if a time is "bright" (day) or "dark" (night/dusk)
    auto isNightTime = [](MumeTimeEnum time) {
        return time == MumeTimeEnum::NIGHT || time == MumeTimeEnum::DUSK;
    };

    // Check if time transitioned between day and night
    if (m_lastTimeOfDay != MumeTimeEnum::UNKNOWN) {
        const bool wasNight = isNightTime(m_lastTimeOfDay);
        const bool isNight = isNightTime(timeOfDay);

        if (wasNight != isNight) {
            // Day↔Night transition detected - force mesh regeneration
            notifyModified(RoomUpdateFlags{RoomUpdateEnum::RoomMeshNeedsUpdate});
        }
    }

    // Update last time for next check
    m_lastTimeOfDay = timeOfDay;

    // Get player's current room and light status
    const std::optional<RoomId> playerRoom = m_selectedRoom;
    const bool playerHasLight = m_currentPromptFlags.isValid()
                                    && (m_currentPromptFlags.isLit()
                                        || !m_currentPromptFlags.isDark());

    return generateMapDataFinisher(textures, font, getCurrentMap(), timeOfDay, playerRoom, playerHasLight);
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

void MapData::slot_scheduleAction(const SigMapChangeList &change)
{
    this->applyChanges(change.deref());
}

bool MapData::isEmpty() const
{
    return getCurrentMap().empty();
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
            setFileName(mapLoadData.filename, mapLoadData.readonly);
            setSavedMap(mapLoadData.mapPair.base);
            setCurrentMap(mapLoadData.mapPair.modified);
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
Map MapData::mergeMapData(ProgressCounter &counter, const Map &currentMap, RawMapLoadData newMapData)
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

    return Map::merge(counter,
                      currentMap,
                      std::move(newMapData.rooms),
                      std::move(newMapData.markers),
                      mapOffset);
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

            if (savedMap.getInfomarkDb() != currentMap.getInfomarkDb()) {
                // REVISIT: Can we get a better description of what changed?
                os << "Infomarks have changed.\n";
            }
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
