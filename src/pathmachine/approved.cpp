/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "approved.h"

#include "../expandoracommon/AbstractRoomFactory.h"
#include "../expandoracommon/RoomAdmin.h"
#include "../expandoracommon/room.h"
#include "../mapfrontend/mapaction.h"

void Approved::receiveRoom(RoomAdmin *sender, const Room *perhaps)
{
    auto &event = myEvent.deref();

    if (matchedRoom == nullptr) {
        ComparisonResult indicator = factory->compare(perhaps, event, matchingTolerance);
        if (indicator != ComparisonResult::DIFFERENT) {
            matchedRoom = perhaps;
            owner = sender;
            if (indicator == ComparisonResult::TOLERANCE && event.getNumSkipped() == 0) {
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
