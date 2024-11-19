#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"
#include "../global/macros.h"
#include "roomid.h"

class MapData;
class RoomHandle;

/*! \brief Interface giving briefly access to a mutex-protected room.
 *
 * See MapFrontend::lookingForRooms().
 */
class NODISCARD RoomRecipient
{
public:
    RoomRecipient();
    virtual ~RoomRecipient();

private:
    virtual void virt_receiveRoom(const RoomHandle &) = 0;

public:
    void receiveRoom(const RoomHandle &room) { virt_receiveRoom(room); }

public:
    DELETE_CTORS_AND_ASSIGN_OPS(RoomRecipient);
};
