/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#include "roomsignalhandler.h"

#include <cassert>

#include "../expandoracommon/room.h"
#include "../expandoracommon/roomadmin.h"
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

void RoomSignalHandler::keep(const Room *const room, const ExitDirection dir, const RoomId fromId)
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
