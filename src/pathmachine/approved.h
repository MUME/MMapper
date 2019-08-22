#pragma once
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

#include "../expandoracommon/RoomRecipient.h"
#include "../expandoracommon/parseevent.h"
#include "../global/RuleOf5.h"

class AbstractRoomFactory;
class ParseEvent;
class Room;
class RoomAdmin;

class Approved : public RoomRecipient
{
private:
    SigParseEvent myEvent;
    const Room *matchedRoom = nullptr;
    int matchingTolerance = 0;
    RoomAdmin *owner = nullptr;
    bool moreThanOne = false;
    bool update = false;
    AbstractRoomFactory *factory = nullptr;

public:
    explicit Approved(AbstractRoomFactory *in_factory,
                      const SigParseEvent &sigParseEvent,
                      int matchingTolerance);
    ~Approved() override;
    void receiveRoom(RoomAdmin *, const Room *) override;
    const Room *oneMatch() const;
    RoomAdmin *getOwner() const;
    bool needsUpdate() const { return update; }
    void reset();

public:
    Approved() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(Approved);
};
