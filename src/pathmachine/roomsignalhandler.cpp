// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "roomsignalhandler.h"

#include "../mapdata/mapdata.h"

#include <cassert>

void RoomSignalHandler::hold(const RoomId room)
{
    m_owners.insert(room);
    m_holdCount[room]++;
}

void RoomSignalHandler::release(const RoomId room)
{
    auto it_hold_count = m_holdCount.find(room);
    assert(it_hold_count != m_holdCount.end() && it_hold_count->second > 0);

    if (--it_hold_count->second == 0) {
        if (m_owners.contains(room)) {
            std::ignore = m_map.tryRemoveTemporary(room);
        } else {
            assert(false);
        }

        m_owners.erase(room);
        m_holdCount.erase(room);
    }
}

void RoomSignalHandler::keep(const RoomId room,
                             const ExitDirEnum dir,
                             const RoomId fromId,
                             ChangeList &changes)
{
    auto it_hold_count = m_holdCount.find(room);
    assert(it_hold_count != m_holdCount.end() && it_hold_count->second != 0);
    assert(m_owners.contains(room));

    static_assert(static_cast<uint32_t>(ExitDirEnum::UNKNOWN) + 1 == NUM_EXITS);
    if (isNESWUD(dir) || dir == ExitDirEnum::UNKNOWN) {
        changes.add(exit_change_types::ModifyExitConnection{ChangeTypeEnum::Add,
                                                            fromId,
                                                            dir,
                                                            room,
                                                            WaysEnum::OneWay});
    }

    std::ignore = m_map.tryMakePermanent(room);
    release(room);
}

size_t RoomSignalHandler::getNumHolders(const RoomId room) const
{
    auto it = m_holdCount.find(room);
    if (it != m_holdCount.end()) {
        return it->second;
    }
    return 0;
}
