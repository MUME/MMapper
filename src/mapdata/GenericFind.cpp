// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "GenericFind.h"

#include "roomfilter.h"

RoomIdSet genericFind(const Map &map, const RoomFilter &f)
{
    RoomIdSet result;
    for (const RoomId id : map.getRooms()) {
        const auto &room = map.getRawRoom(id);
        if (f.filter(room)) {
            result.insert(id);
        }
    }
    return result;
}
