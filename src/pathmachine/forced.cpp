// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "forced.h"

#include "../expandoracommon/RoomAdmin.h"
#include "../map/room.h"
#include "../mapfrontend/mapaction.h"

#include <memory>

void Forced::virt_receiveRoom(RoomAdmin *const sender, const Room *const perhaps)
{
    if (m_matchedRoom == nullptr) {
        m_matchedRoom = perhaps;
        m_owner = sender;
        if (m_update) {
            // Force update room with last event
            m_owner->scheduleAction(
                std::make_shared<SingleRoomAction>(std::make_unique<Update>(m_myEvent),
                                                   perhaps->getId()));
        }
    } else {
        sender->releaseRoom(*this, perhaps->getId());
    }
}

Forced::~Forced()
{
    if (m_owner != nullptr) {
        m_owner->releaseRoom(*this, m_matchedRoom->getId());
    }
}

Forced::Forced(const SigParseEvent &sigParseEvent, bool update)
    : m_myEvent{sigParseEvent.requireValid()}
    , m_update{update}
{}
