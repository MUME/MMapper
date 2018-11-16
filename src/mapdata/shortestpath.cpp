/************************************************************************
**
** Authors:   ethorondil
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "shortestpath.h"

#include <memory>
#include <type_traits>
#include <utility>
#include <QBasicMutex>
#include <QSet>
#include <QVector>
#include <queue>

#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/enums.h"
#include "../global/roomid.h"
#include "ExitDirection.h"
#include "ExitFlags.h"
#include "mapdata.h"
#include "mmapper2room.h"
#include "roomfilter.h"

// Movement costs per terrain type.
// Same order as the RoomTerrainType enum.
// Values taken from https://github.com/nstockton/tintin-mume/blob/master/mapperproxy/mapper/constants.py

ShortestPathRecipient::~ShortestPathRecipient() = default;

static double terrain_cost(const RoomTerrainType type)
{
    switch (type) {
    case RoomTerrainType::UNDEFINED:
        return 1.0; // undefined
    case RoomTerrainType::INDOORS:
        return 0.75; // indoors
    case RoomTerrainType::CITY:
        return 0.75; // city
    case RoomTerrainType::FIELD:
        return 1.5; // field
    case RoomTerrainType::FOREST:
        return 2.15; // forest
    case RoomTerrainType::HILLS:
        return 2.45; // hills
    case RoomTerrainType::MOUNTAINS:
        return 2.8; // mountains
    case RoomTerrainType::SHALLOW:
        return 2.45; // shallow
    case RoomTerrainType::WATER:
        return 50.0; // water
    case RoomTerrainType::RAPIDS:
        return 60.0; // rapids
    case RoomTerrainType::UNDERWATER:
        return 100.0; // underwater
    case RoomTerrainType::ROAD:
        return 0.85; // road
    case RoomTerrainType::BRUSH:
        return 1.5; // brush
    case RoomTerrainType::TUNNEL:
        return 0.75; // tunnel
    case RoomTerrainType::CAVERN:
        return 0.75; // cavern
    case RoomTerrainType::DEATHTRAP:
        return 1000.0; // deathtrap
    }

    return 1.0;
}

static double getLength(const Exit &e, const Room *curr, const Room *nextr)
{
    double cost = terrain_cost(nextr->getTerrainType());
    auto flags = e.getExitFlags();
    if (flags.isRandom()) {
        cost += 30;
    }
    if (flags.isDoor()) {
        cost += 1;
    }
    if (flags.isClimb()) {
        cost += 2;
    }
    if (nextr->getRidableType() == RoomRidableType::NOT_RIDABLE) {
        cost += 3;
        // One non-ridable room means walking two rooms, plus dismount/mount.
        if (curr->getRidableType() != RoomRidableType::NOT_RIDABLE) {
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
                                 double max_dist)
{
    QMutexLocker locker(&mapLock);
    QVector<SPNode> sp_nodes;
    QSet<RoomId> visited;
    std::priority_queue<std::pair<double, int>> future_paths;
    sp_nodes.push_back(SPNode(origin, -1, 0, ExitDirection::UNKNOWN));
    future_paths.push(std::make_pair(0, 0));
    while (!future_paths.empty()) {
        int spindex = future_paths.top().second;
        future_paths.pop();
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
        for (const ExitDirection dir : enums::makeCountingIterator<ExitDirection>(exits)) {
            const Exit &e = exits[dir];
            if (!e.outIsUnique()) {
                // 0: Not mapped
                // 2+: Random, so no clear directions; skip it.
                continue;
            }
            if (!e.isExit()) {
                continue;
            }
            const Room *const nextr = roomIndex[e.outFirst()];
            if (visited.contains(nextr->getId())) {
                continue;
            }
            const double length = getLength(e, thisr, nextr);
            sp_nodes.push_back(
                SPNode(nextr, spindex, thisdist + length, static_cast<ExitDirection>(dir)));
            future_paths.push(std::make_pair(-(thisdist + length), sp_nodes.size() - 1));
        }
    }
}
