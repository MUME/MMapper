// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "forced.h"

#include "../map/ChangeTypes.h"
#include "../map/room.h"
#include "../mapdata/mapdata.h"

#include <memory>

void Forced::virt_receiveRoom(const RoomHandle &perhaps)
{
    if (m_matchedRoom == std::nullopt) {
        m_matchedRoom = perhaps;
        if (m_update) {
            // Force update room with last event
            m_map.scheduleAction(
                Change{room_change_types::Update{perhaps.getId(), m_myEvent.deref()}});
        }
    } else {
        m_map.releaseRoom(*this, perhaps.getId());
    }
}

Forced::~Forced()
{
    m_map.releaseRoom(*this, deref(m_matchedRoom).getId());
}

Forced::Forced(MapFrontend &map, const SigParseEvent &sigParseEvent, const bool update)
    : m_map{map}
    , m_myEvent{sigParseEvent.requireValid()}
    , m_update{update}
{}
