#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"

class Room;
class RoomAdmin;

/*! \brief Interface giving briefly access to a mutex-protected room.
 *
 * See MapFrontend::lookingForRooms().
 */
class RoomRecipient
{
public:
    explicit RoomRecipient();
    virtual ~RoomRecipient();
    virtual void receiveRoom(RoomAdmin *admin, const Room *room) = 0;

public:
    DELETE_CTORS_AND_ASSIGN_OPS(RoomRecipient);
};
