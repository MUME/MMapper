// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../global/CaseUtils.h"
#include "../global/progresscounter.h"
#include "../global/utils.h"
#include "World.h"

#include <array>
#include <deque>

void World::apply(ProgressCounter &pc, const world_change_types::GenerateBaseMap & /* unused */)
{
    RoomIdSet baseRooms;
    std::deque<RoomId> roomsTodo;

    auto receiveRoom = [&baseRooms, &roomsTodo](const RawRoom &room) {
        // const auto id = room.getId();
        for (const auto &exit : room.getExits()) {
            if (exit.doorIsHidden() || exit.exitIsNoMatch()) {
                // m_impl->secretLinks.emplace(RoomLink{id, to_ext});
            } else {
                for (const RoomId to_id : exit.getOutgoingSet()) {
                    baseRooms.insert(to_id);
                    roomsTodo.emplace_back(to_id);
                }
            }
        }
    };

    pc.setNewTask(ProgressMsg{"seeding rooms"}, getRoomSet().size());

    // Seed rooms
    static const std::array<std::string_view, 2> seeds = {"The Fountain Square", "Cosy Room"};
    for (auto id : getRoomSet()) {
        const RawRoom &room = deref(getRoom(id));
        if (room.isPermanent()) {
            auto rname = room.getName().getStdStringViewUtf8();
            for (auto &seed : seeds) {
                if (areEqualAsLowerUtf8(seed, rname)) {
                    baseRooms.insert(id);
                    roomsTodo.push_back(id);
                }
            }
        }
        pc.step();
    }

    if (baseRooms.empty()) {
        qWarning() << "Unable to filter the map.";
        return;
    }

    pc.setNewTask(ProgressMsg{"find all accessible rooms"}, getRoomSet().size());

    // Walk the whole map through non-hidden exits without recursing
    RoomIdSet considered;
    while (!roomsTodo.empty()) {
        const RoomId todo = utils::pop_front(roomsTodo);
        if (considered.contains(todo)) {
            // Don't process the same room twice (ending condition)
            continue;
        }

        considered.insert(todo);
        if (const RawRoom *pRoom = getRoom(todo)) {
            receiveRoom(*pRoom);
        }
        pc.step();
    }

    // REVISIT: This is done in two passes because doing it in a single pass
    // fails to remove NO_EXIT flags. That might be a "feature" of room removal?
    //
    // As a single pass: 5494 is removed before 5499 tries to nuke the exit,
    // which somehow leaves the NO_EXIT flag west from 5499, and that causes
    // mmapper to display the a fork in the road instead of a bend.
    //
    // As two passes: 5499 nukes the exit, and then 5494 is removed.
    // This correctly removes the NO_EXIT flag and displays a bend.
    const auto copy = getRoomSet();
    {
        pc.setNewTask(ProgressMsg{"removing hidden exits"}, copy.size());
        size_t removedExits = 0;
        for (auto id : copy) {
            if (baseRooms.contains(id)) {
                // Use a copy instead of a reference, to avoid crashing when trying out different
                // immer-like backend implementations that use copy-on-write.
                const RawRoom room = deref(getRoom(id));
                for (const auto dir : ALL_EXITS7) {
                    if (room.hasTrivialExit(dir)) {
                        continue;
                    }
                    const auto &exit = room.getExit(dir);
                    if (exit.doorIsHidden() || exit.exitIsNoMatch()) {
                        nukeExit(id, dir, WaysEnum::OneWay);
                        ++removedExits;
                    }
                }
            }
            pc.step();
        }
        qInfo() << "GenerateBaseMap removed" << removedExits << "hidden or no-match exits(s)";
    }
    {
        pc.setNewTask(ProgressMsg{"removing inaccessible rooms"}, copy.size());
        size_t removedRooms = 0;
        for (auto id : copy) {
            if (!baseRooms.contains(id)) {
                removeFromWorld(id, true);
                ++removedRooms;
            }
            pc.step();
        }

        qInfo() << "GenerateBaseMap removed" << removedRooms << "inaccessible rooms(s)";
    }
}
