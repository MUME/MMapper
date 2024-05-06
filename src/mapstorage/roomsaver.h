#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../expandoracommon/RoomRecipient.h"
#include "../global/RuleOf5.h"
#include "../mapdata/mapdata.h"

#include <QtGlobal>

class Room;
class RoomAdmin;

class RoomSaver final : public RoomRecipient
{
private:
    ConstRoomList &m_roomList;
    RoomAdmin &m_admin;

public:
    explicit RoomSaver(RoomAdmin &admin, ConstRoomList &list);
    ~RoomSaver() override;

public:
    RoomSaver() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(RoomSaver);

private:
    void virt_receiveRoom(RoomAdmin *admin, const Room *room) final;

public:
    NODISCARD quint32 getRoomsCount();
};
