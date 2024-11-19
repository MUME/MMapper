// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: 'Elval' <ethorondil@gmail.com> (Elval)

#include "shortestpath.h"

#include "../global/enums.h"
#include "../global/utils.h"
#include "../map/ExitDirection.h"
#include "../map/ExitFlags.h"
#include "../map/exit.h"
#include "../map/mmapper2room.h"
#include "../map/room.h"
#include "../map/roomid.h"
#include "mapdata.h"
#include "roomfilter.h"

#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include <QBasicMutex>
#include <QSet>
#include <QVector>
#include <queue>

// Movement costs per terrain type.
// Same order as the RoomTerrainEnum enum.
// Values taken from https://github.com/nstockton/tintin-mume/blob/master/mapperproxy/mapper/constants.py

ShortestPathRecipient::~ShortestPathRecipient() = default;

NODISCARD static double terrain_cost(const RoomTerrainEnum type)
{
    switch (type) {
    case RoomTerrainEnum::UNDEFINED:
        return 1.0; // undefined
    case RoomTerrainEnum::INDOORS:
        return 0.75; // indoors
    case RoomTerrainEnum::CITY:
        return 0.75; // city
    case RoomTerrainEnum::FIELD:
        return 1.5; // field
    case RoomTerrainEnum::FOREST:
        return 2.15; // forest
    case RoomTerrainEnum::HILLS:
        return 2.45; // hills
    case RoomTerrainEnum::MOUNTAINS:
        return 2.8; // mountains
    case RoomTerrainEnum::SHALLOW:
        return 2.45; // shallow
    case RoomTerrainEnum::WATER:
        return 50.0; // water
    case RoomTerrainEnum::RAPIDS:
        return 60.0; // rapids
    case RoomTerrainEnum::UNDERWATER:
        return 100.0; // underwater
    case RoomTerrainEnum::ROAD:
        return 0.85; // road
    case RoomTerrainEnum::BRUSH:
        return 1.5; // brush
    case RoomTerrainEnum::TUNNEL:
        return 0.75; // tunnel
    case RoomTerrainEnum::CAVERN:
        return 0.75; // cavern
    case RoomTerrainEnum::DEATHTRAP:
        return 1000.0; // deathtrap
    }

    return 1.0;
}

NODISCARD static double getLength(const Exit &e, const Room *curr, const Room *nextr)
{
    double cost = terrain_cost(nextr->getTerrainType());
    auto flags = e.getExitFlags();
    if (flags.isRandom() || flags.isDamage() || flags.isFall()) {
        cost += 30;
    }
    if (flags.isDoor()) {
        cost += 1;
    }
    if (flags.isClimb()) {
        cost += 2;
    }
    if (nextr->getRidableType() == RoomRidableEnum::NOT_RIDABLE) {
        cost += 3;
        // One non-ridable room means walking two rooms, plus dismount/mount.
        if (curr->getRidableType() != RoomRidableEnum::NOT_RIDABLE) {
            cost += 4;
        }
    }
    if (flags.isRoad()) { // Not sure if this is appropriate.
        cost -= 0.1;
    }
    return cost;
}

void MapData::shortestPathSearch(const Room *origin,
                                 ShortestPathRecipient *recipient,
                                 const RoomFilter &f,
                                 int max_hits,
                                 const double max_dist)
{
    // the search stops if --max_hits == 0, so max_hits must be greater than 0,
    // but the default parameter is -1.
    assert(max_hits > 0 || max_hits == -1);

    // although the data probably won't ever contain more than 2 billion results,
    // let's at least pretend to care about potential signed integer underflow (UB)
    if (max_hits <= 0) {
        max_hits = std::numeric_limits<int>::max();
    }

    QMutexLocker locker(&mapLock);
    QVector<SPNode> sp_nodes;
    QSet<RoomId> visited;
    std::priority_queue<std::pair<double, int>> future_paths;
    sp_nodes.push_back(SPNode(origin, -1, 0, ExitDirEnum::UNKNOWN));
    future_paths.emplace(0, 0);
    while (!future_paths.empty()) {
        int spindex = utils::pop_top(future_paths).second;
        const Room *thisr = sp_nodes[spindex].r;
        auto thisdist = sp_nodes[spindex].dist;
        auto room_id = thisr->getId();
        if (visited.contains(room_id)) {
            continue;
        }
        visited.insert(room_id);
        if (f.filter(thisr)) {
            recipient->receiveShortestPath(this, sp_nodes, spindex);
            if (--max_hits == 0) {
                return;
            }
        }
        if ((max_dist != 0.0) && thisdist > max_dist) {
            return;
        }
        ExitsList exits = thisr->getExitsList();
        for (const ExitDirEnum dir : enums::makeCountingIterator<ExitDirEnum>(exits)) {
            const Exit &e = exits[dir];
            if (!e.outIsUnique()) {
                // 0: Not mapped
                // 2+: Random, so no clear directions; skip it.
                continue;
            }
            if (!e.isExit()) {
                continue;
            }
            const SharedConstRoom &nextr = roomIndex[e.outFirst()];
            if (!nextr) {
                qWarning() << "Source room" << thisr->getId().asUint32() << "("
                           << thisr->getName().toQString() << ") has target room"
                           << e.outFirst().asUint32() << "which does not exist!";
                continue;
            }
            if (visited.contains(nextr->getId())) {
                continue;
            }
            const double length = getLength(e, thisr, nextr.get());
            sp_nodes.push_back(SPNode(nextr.get(), spindex, thisdist + length, dir));
            future_paths.emplace(-(thisdist + length), sp_nodes.size() - 1);
        }
    }
}
