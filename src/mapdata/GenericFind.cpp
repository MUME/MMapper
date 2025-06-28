// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "GenericFind.h"

#include "../map/Map.h"
#include "roomfilter.h"

RoomIdSet genericFind(const Map &map, const RoomFilter &f)
{
    RoomIdSet result;
    map.getRooms().for_each([&result, &map, &f](const RoomId id) {
        const auto &room = map.getRawRoom(id);
        if (f.filter(room)) {
            result.insert(id);
        }
    });
    return result;
}
