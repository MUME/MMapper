#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../expandoracommon/RoomRecipient.h"

#include "../expandoracommon/parseevent.h"
#include "../global/RuleOf5.h"

class ParseEvent;
class Room;
class RoomAdmin;

class NODISCARD Forced final : public RoomRecipient
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
    NODISCARD const Room *oneMatch() const { return matchedRoom; }

public:
    Forced() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(Forced);
};
