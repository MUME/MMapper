// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "roomsaver.h"

#include "../expandoracommon/RoomAdmin.h"
#include "../expandoracommon/room.h"
#include "../mapdata/mapdata.h"

#include <cassert>

RoomSaver::RoomSaver(RoomAdmin &admin, ConstRoomList &list)
    : m_roomList(list)
    , m_admin(admin)
{}

void RoomSaver::virt_receiveRoom(RoomAdmin *const admin, const Room *const room)
{
    assert(admin == &m_admin);
    if (room->isTemporary()) {
        m_admin.releaseRoom(*this, room->getId());
    } else {
        m_roomList.emplace_back(room->shared_from_this());
    }
}

uint32_t RoomSaver::getRoomsCount()
{
    return static_cast<uint32_t>(m_roomList.size());
}

RoomSaver::~RoomSaver()
{
    for (auto &room : m_roomList) {
        if (room != nullptr) {
            m_admin.releaseRoom(*this, room->getId());
            room = nullptr;
        }
    }
}
