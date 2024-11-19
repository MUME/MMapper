// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "roomsignalhandler.h"

#include "../map/AbstractChangeVisitor.h"
#include "../map/room.h"
#include "../map/roomid.h"
#include "../mapdata/mapdata.h"

#include <cassert>
#include <memory>

void RoomSignalHandler::hold(const RoomId room, RoomRecipient *const locker)
{
    // REVISIT: why do we allow locker to be null?
    owners.insert(room);
    if (lockers[room].empty()) {
        holdCount[room] = 0;
    }
    lockers[room].insert(locker);
    ++holdCount[room];
}

void RoomSignalHandler::release(const RoomId room)
{
    assert(holdCount[room]);
    if (--holdCount[room] == 0) {
        if (owners.contains(room)) {
            for (auto i = lockers[room].begin(); i != lockers[room].end(); ++i) {
                if (RoomRecipient *const recipient = *i) {
                    m_map.releaseRoom(*recipient, room);
                }
            }
        } else {
            assert(false);
        }

        lockers.erase(room);
        owners.erase(room);
    }
}

void RoomSignalHandler::keep(const RoomId room, const ExitDirEnum dir, const RoomId fromId)
{
    assert(holdCount[room] != 0);
    assert(owners.contains(room));

    static_assert(static_cast<uint32_t>(ExitDirEnum::UNKNOWN) + 1 == NUM_EXITS);
    if (isNESWUD(dir) || dir == ExitDirEnum::UNKNOWN) {
        ChangeList changes;
        changes.add(exit_change_types::ModifyExitConnection{ChangeTypeEnum::Add,
                                                            fromId,
                                                            dir,
                                                            room,
                                                            WaysEnum::OneWay});
        const auto wrapped = SigMapChangeList{std::make_shared<ChangeList>(changes)};
        emit sig_scheduleAction(wrapped);
    }

    if (!lockers[room].empty()) {
        if (RoomRecipient *const locker = *(lockers[room].begin())) {
            m_map.keepRoom(*locker, room);
            lockers[room].erase(locker);
        } else {
            assert(false);
        }
    }
    release(room);
}
