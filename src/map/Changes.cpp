// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "Changes.h"

using namespace world_change_types;
using namespace exit_change_types;
using namespace room_change_types;

void connectToNeighbors(ChangeList &changes,
                        const RoomHandle &room,
                        const ConnectToNeighborsArgs &args)
{
    const auto ways = args.ways;

    auto add = [&changes, ways](const RoomId from, const ExitDirEnum dir, const RoomId to) {
        namespace ect = exit_change_types;
        changes.add(ect::ModifyExitConnection{ChangeTypeEnum::Add, from, dir, to, ways});
    };

    const Map map = room.getMap();
    auto connect = [&add, &map](const RoomId from, const Coordinate &pos, const ExitDirEnum dir) {
        if (const auto optRoom = map.findRoomHandle(pos)) {
            add(from, dir, optRoom.getId());
        }
    };

    const RoomId from = room.getId();
    const Coordinate &center = room.getPosition();
    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        const Coordinate pos = center + ::exitDir(dir);
        connect(from, pos, dir);
    }
}
