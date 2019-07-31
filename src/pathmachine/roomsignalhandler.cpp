// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "roomsignalhandler.h"

#include <cassert>

#include "../expandoracommon/RoomAdmin.h"
#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../mapfrontend/mapaction.h"

void RoomSignalHandler::hold(const Room *const room,
                             RoomAdmin *const owner,
                             RoomRecipient *const locker)
{
    owners[room] = owner;
    if (lockers[room].empty()) {
        holdCount[room] = 0;
    }
    lockers[room].insert(locker);
    ++holdCount[room];
}

void RoomSignalHandler::release(const Room *const room)
{
    assert(holdCount[room]);
    if (--holdCount[room] == 0) {
        if (RoomAdmin *const rcv = owners[room]) {
            for (auto i = lockers[room].begin(); i != lockers[room].end(); ++i) {
                if (RoomRecipient *const recipient = *i) {
                    rcv->releaseRoom(*recipient, room->getId());
                }
            }
        } else {
            assert(false);
        }

        lockers.erase(room);
        owners.erase(room);
    }
}

void RoomSignalHandler::keep(const Room *const room, const ExitDirEnum dir, const RoomId fromId)
{
    deref(room);
    assert(!room->isFake());
    assert(holdCount[room] != 0);

    RoomAdmin *const rcv = owners[room];
    if (static_cast<uint32_t>(dir) < NUM_EXITS) {
        emit scheduleAction(new AddExit(fromId, room->getId(), dir));
    }

    if (!lockers[room].empty()) {
        if (RoomRecipient *const locker = *(lockers[room].begin())) {
            rcv->keepRoom(*locker, room->getId());
            lockers[room].erase(locker);
        } else {
            assert(false);
        }
    }
    release(room);
}
