// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "approved.h"

#include "../expandoracommon/RoomAdmin.h"
#include "../expandoracommon/room.h"
#include "../mapfrontend/mapaction.h"

Approved::Approved(const SigParseEvent &sigParseEvent, const int tolerance)
    : myEvent{sigParseEvent.requireValid()}
    , matchingTolerance{tolerance}
{}

Approved::~Approved()
{
    if (owner != nullptr) {
        if (moreThanOne) {
            owner->releaseRoom(*this, matchedRoom->getId());
        } else {
            owner->keepRoom(*this, matchedRoom->getId());
        }
    }
}

void Approved::receiveRoom(RoomAdmin *const sender, const Room *const perhaps)
{
    auto &event = myEvent.deref();

    const auto id = perhaps->getId();
    const auto cmp = Room::compare(perhaps, event, matchingTolerance);

    if (cmp == ComparisonResultEnum::DIFFERENT) {
        sender->releaseRoom(*this, id);
        return;
    }

    if (matchedRoom != nullptr) {
        // moreThanOne should only take effect if multiple distinct rooms match
        if (matchedRoom->getId() != id)
            moreThanOne = true;
        sender->releaseRoom(*this, id);
        return;
    }

    matchedRoom = perhaps;
    owner = sender;
    if (cmp == ComparisonResultEnum::TOLERANCE && event.getNumSkipped() == 0) {
        update = true;
    }
}

const Room *Approved::oneMatch() const
{
    return moreThanOne ? nullptr : matchedRoom;
}

void Approved::releaseMatch()
{
    // Release the current candidate in order to receive additional candidates
    if (matchedRoom != nullptr) {
        owner->releaseRoom(*this, matchedRoom->getId());
    }
    update = false;
    matchedRoom = nullptr;
    moreThanOne = false;
    owner = nullptr;
}
