#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <QtGlobal>

#include "../expandoracommon/RoomRecipient.h"
#include "../global/RuleOf5.h"
#include "../mapdata/mapdata.h"

class Room;
class RoomAdmin;

class RoomSaver final : public RoomRecipient
{
public:
    explicit RoomSaver(RoomAdmin &admin, ConstRoomList &list);
    ~RoomSaver() override;
    void receiveRoom(RoomAdmin *admin, const Room *room) override;
    quint32 getRoomsCount();

public:
    RoomSaver() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(RoomSaver);

private:
    ConstRoomList &m_roomList;
    RoomAdmin &m_admin;
};
