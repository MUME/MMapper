#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../expandoracommon/RoomRecipient.h"
#include "../expandoracommon/parseevent.h"
#include "../global/RuleOf5.h"

class ParseEvent;
class Room;
class RoomAdmin;

class Approved : public RoomRecipient
{
private:
    SigParseEvent myEvent;
    const Room *matchedRoom = nullptr;
    const int matchingTolerance;
    RoomAdmin *owner = nullptr;
    bool moreThanOne = false;
    bool update = false;

public:
    explicit Approved(const SigParseEvent &sigParseEvent, int matchingTolerance);
    ~Approved() override;

public:
    void receiveRoom(RoomAdmin *, const Room *) override;
    const Room *oneMatch() const;
    bool needsUpdate() const { return update; }
    void reset();

public:
    Approved() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(Approved);
};
