#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "room.h"

class RoomRecipient;
class MapAction;

class RoomAdmin
{
public:
    virtual ~RoomAdmin();

    // removes the lock on a room
    // after the last lock is removed, the room is deleted
    virtual void releaseRoom(RoomRecipient &, RoomId) = 0;

    // makes a lock on a room permanent and anonymous.
    // Like that the room can't be deleted via releaseRoom anymore.
    virtual void keepRoom(RoomRecipient &, RoomId) = 0;

    virtual void scheduleAction(MapAction *action) = 0;
};
