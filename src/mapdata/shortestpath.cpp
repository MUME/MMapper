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
    }

    return 1.0;
}

NODISCARD static double getLength(const RawExit &e, const RoomHandle &curr, const RoomHandle &nextr)
{
    double cost = terrain_cost(nextr.getTerrainType());
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
    if (nextr.getRidableType() == RoomRidableEnum::NOT_RIDABLE) {
        cost += 3;
        // One non-ridable room means walking two rooms, plus dismount/mount.
        if (curr.getRidableType() != RoomRidableEnum::NOT_RIDABLE) {
            cost += 4;
        }
    }
    if (flags.isRoad()) { // Not sure if this is appropriate.
        cost -= 0.1;
    }
    if (nextr.getLoadFlags().contains(RoomLoadFlagEnum::DEATHTRAP)) {
        cost += 1000.0;
    }
    return cost;
}

void MapData::shortestPathSearch(const RoomHandle &origin,
                                 ShortestPathRecipient &recipient,
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

    const Map &map = origin.getMap();

    QVector<SPNode> sp_nodes;
    RoomIdSet visited;
    std::priority_queue<std::pair<double, int>> future_paths;
    sp_nodes.push_back(SPNode{origin, -1, 0, ExitDirEnum::UNKNOWN});
    future_paths.emplace(0.0, 0);
    while (!future_paths.empty()) {
        const int spindex = utils::pop_top(future_paths).second;
        const auto &thisr = deref(sp_nodes[spindex].r);
        const auto thisdist = sp_nodes[spindex].dist;
        auto room_id = thisr.getId();
        if (visited.contains(room_id)) {
            continue;
        }
        visited.insert(room_id);
        if (f.filter(thisr.getRaw())) {
            recipient.receiveShortestPath(sp_nodes, spindex);
            if (--max_hits == 0) {
                return;
            }
        }
        if ((max_dist != 0.0) && thisdist > max_dist) {
            return;
        }
        for (const ExitDirEnum dir : ALL_EXITS7) {
            const auto &e = thisr.getExit(dir);
            if (!e.outIsUnique()) {
                // 0: Not mapped
                // 2+: Random, so no clear directions; skip it.
                continue;
            }
            if (!e.exitIsExit()) {
                continue;
            }

            const RoomId nextrId = e.getOutgoingSet().first();
            const auto &nextr = map.getRoomHandle(nextrId);

            if (!nextr) {
                /* DEAD CODE */
                qWarning() << "Source room" << thisr.getIdExternal().asUint32() << "("
                           << thisr.getName().toQString()
                           << ") dir=" << mmqt::toQStringLatin1(to_string_view(dir))
                           << "has target room with internal identifier" << nextrId.asUint32()
                           << "which does not exist!";
                qWarning() << mmqt::toQStringUtf8(thisr.toStdStringUtf8());
                // This would cause a segfault in the old map scheme, but maps are now rigorously
                // validated, so it should be impossible to have an exit to a room that does
                // not exist.
                assert(false);
                continue;
            }
            if (visited.contains(nextr.getId())) {
                continue;
            }

            const double length = getLength(e, thisr, nextr);
            sp_nodes.push_back(SPNode{nextr, spindex, thisdist + length, dir});
            future_paths.emplace(-(thisdist + length), sp_nodes.size() - 1);
        }
    }
}
