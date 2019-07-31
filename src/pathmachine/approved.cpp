// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "approved.h"

#include "../expandoracommon/AbstractRoomFactory.h"
#include "../expandoracommon/RoomAdmin.h"
#include "../expandoracommon/room.h"
#include "../mapfrontend/mapaction.h"

void Approved::receiveRoom(RoomAdmin *sender, const Room *perhaps)
{
    auto &event = myEvent.deref();

    if (matchedRoom == nullptr) {
        const ComparisonResultEnum indicator = factory->compare(perhaps, event, matchingTolerance);
        if (indicator != ComparisonResultEnum::DIFFERENT) {
            matchedRoom = perhaps;
            owner = sender;
            if (indicator == ComparisonResultEnum::TOLERANCE && event.getNumSkipped() == 0) {
                update = true;
            }
        } else {
            sender->releaseRoom(*this, perhaps->getId());
        }
    } else {
        moreThanOne = true;
        sender->releaseRoom(*this, perhaps->getId());
    }
}

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

Approved::Approved(AbstractRoomFactory *const in_factory,
                   const SigParseEvent &sigParseEvent,
                   const int tolerance)
    : myEvent{sigParseEvent.requireValid()}
    , matchingTolerance{tolerance}
    , factory{in_factory}
{}

const Room *Approved::oneMatch() const
{
    return moreThanOne ? nullptr : matchedRoom;
}

void Approved::reset()
{
    if (matchedRoom != nullptr) {
        owner->releaseRoom(*this, matchedRoom->getId());
    }
    update = false;
    matchedRoom = nullptr;
    moreThanOne = false;
    owner = nullptr;
}

RoomAdmin *Approved::getOwner() const
{
    return moreThanOne ? nullptr : owner;
}
