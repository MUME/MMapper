// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "forced.h"

#include <memory>

#include "../expandoracommon/RoomAdmin.h"
#include "../expandoracommon/room.h"
#include "../mapfrontend/mapaction.h"

void Forced::virt_receiveRoom(RoomAdmin *const sender, const Room *const perhaps)
{
    if (matchedRoom == nullptr) {
        matchedRoom = perhaps;
        owner = sender;
        if (update) {
            // Force update room with last event
            owner->scheduleAction(
                std::make_shared<SingleRoomAction>(std::make_unique<Update>(myEvent),
                                                   perhaps->getId()));
        }
    } else {
        sender->releaseRoom(*this, perhaps->getId());
    }
}

Forced::~Forced()
{
    if (owner != nullptr) {
        owner->releaseRoom(*this, matchedRoom->getId());
    }
}

Forced::Forced(const SigParseEvent &sigParseEvent, bool update)
    : myEvent{sigParseEvent.requireValid()}
    , update{update}
{}
