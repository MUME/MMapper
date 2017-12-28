#include "shortestpath.h"
#include "mmapper2exit.h"
#include "mmapper2room.h"
#include "mapdata.h"
#include<queue>

// Movement costs per terrain type.
// Same order as the RoomTerrainType enum.
// Values taken from https://github.com/nstockton/tintin-mume/blob/master/mapperproxy/mapper/constants.py
const double TERRAIN_COSTS[] = {1, // undef
                                0.75, // indoors
                                0.75, // city
                                1.5, // field
                                2.15, // forest
                                2.45, // hills
                                2.8, // mountains
                                2.45, // shallowwater
                                50.0, // water
                                60.0, // rapids
                                100.0, // underwater
                                0.85, // road
                                1.5, // brush
                                0.75, // tunnel
                                0.75, // cavern
                                1000.0, // Deathtrap
                                30.0 // random
                               };

double getLength(const Exit &e, const Room *curr, const Room *nextr)
{
    double cost = TERRAIN_COSTS[getTerrainType(nextr)];
    uint flags = getFlags(e);
    if (ISSET(flags, EF_DOOR))
        cost += 1;
    if (ISSET(flags, EF_CLIMB))
        cost += 2;
    if (getRidableType(nextr) == RRT_NOTRIDABLE) {
        cost += 3;
        // One non-ridable room means walking two rooms, plus dismount/mount.
        if (getRidableType(curr) != RRT_NOTRIDABLE)
            cost += 4;
    }
    if (ISSET(flags, EF_ROAD)) // Not sure if this is appropriate.
        cost -= 0.1;
    return cost;
}

void MapData::shortestPathSearch(const Room *origin, ShortestPathRecipient *recipient,
                                 const RoomFilter &f, int max_hits, double max_dist)
{
    QMutexLocker locker(&mapLock);
    QVector<SPNode> sp_nodes;
    QSet<uint> visited;
    std::priority_queue<std::pair<double, int> > future_paths;
    int q = 0;
    sp_nodes.push_back(SPNode(origin, -1, 0, ED_UNKNOWN));
    future_paths.push(std::make_pair(0, 0));
    while (future_paths.size()) {
        int spindex = future_paths.top().second;
        future_paths.pop();
        const Room *thisr = sp_nodes[spindex].r;
        float thisdist = sp_nodes[spindex].dist;
        int room_id = thisr->getId();
        if (visited.contains(room_id))
            continue;
        visited.insert(room_id);
        if (f.filter(thisr)) {
            recipient->receiveShortestPath(this, sp_nodes, spindex);
            if (--max_hits == 0)
                return;
        }
        if (max_dist && thisdist > max_dist)
            return;
        ExitsList exits = thisr->getExitsList();
        for (int dir = 0; dir < exits.size(); dir++) {
            const Exit &e = exits[dir];
            std::set<uint>::const_iterator outbegin = e.outBegin();
            std::set<uint>::const_iterator outend = e.outEnd();
            if (outbegin == outend) // Not mapped
                continue;
            ++outbegin;
            if (outbegin != outend) // Random, so no clear directions; skip it.
                continue;
            if (!(getFlags(e) & EF_EXIT))
                continue;
            const Room *nextr = roomIndex[*e.outBegin()];
            if (visited.contains(nextr->getId()))
                continue;
            double length = getLength(e, thisr, nextr);
            sp_nodes.push_back(SPNode(nextr, spindex, thisdist + length, (ExitDirection)dir));
            future_paths.push(std::make_pair(-(thisdist + length), sp_nodes.size() - 1));
        }
    }
}
