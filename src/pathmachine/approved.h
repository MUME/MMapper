#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <unordered_map>

#include "../expandoracommon/RoomRecipient.h"
#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/room.h"
#include "../global/RuleOf5.h"
#include "../global/roomid.h"

class ParseEvent;
class Room;
class RoomAdmin;

class NODISCARD Approved : public RoomRecipient
{
private:
    SigParseEvent myEvent;
    std::unordered_map<RoomId, ComparisonResultEnum> compareCache;
    const Room *matchedRoom = nullptr;
    RoomAdmin *owner = nullptr;
    const int matchingTolerance;
    bool moreThanOne = false;
    bool update = false;

public:
    explicit Approved(const SigParseEvent &sigParseEvent, int matchingTolerance);
    ~Approved() override;

public:
    void receiveRoom(RoomAdmin *, const Room *) override;
    NODISCARD const Room *oneMatch() const;
    NODISCARD bool needsUpdate() const { return update; }
    void releaseMatch();

public:
    Approved() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(Approved);
};
