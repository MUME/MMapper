#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#ifndef FORCED_H
#define FORCED_H

#include "../expandoracommon/RoomRecipient.h"
#include "../expandoracommon/parseevent.h"

class ParseEvent;
class Room;
class RoomAdmin;

class Forced : public RoomRecipient
{
private:
    RoomAdmin *owner = nullptr;
    const Room *matchedRoom = nullptr;
    SigParseEvent myEvent;
    bool update = false;

public:
    explicit Forced(const SigParseEvent &sigParseEvent, bool update = false);
    ~Forced() override;
    void receiveRoom(RoomAdmin *, const Room *) override;
    const Room *oneMatch() const { return matchedRoom; }

public:
    Forced() = delete;
    Forced(Forced &&) = delete;
    Forced(const Forced &) = delete;
    Forced &operator=(Forced &&) = delete;
    Forced &operator=(const Forced &) = delete;
};

#endif
