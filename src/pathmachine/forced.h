#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"
#include "../map/RoomHandle.h"
#include "../map/RoomRecipient.h"
#include "../map/parseevent.h"

class ParseEvent;
class MapFrontend;

class NODISCARD Forced final : public RoomRecipient
{
private:
    MapFrontend &m_map;
    RoomHandle m_matchedRoom;
    SigParseEvent m_myEvent;
    bool m_update = false;

public:
    explicit Forced(MapFrontend &map, const SigParseEvent &sigParseEvent, bool update = false);
    Forced() = delete;
    ~Forced() final;
    DELETE_CTORS_AND_ASSIGN_OPS(Forced);

private:
    void virt_receiveRoom(const RoomHandle &) final;

public:
    NODISCARD RoomHandle oneMatch() const { return m_matchedRoom; }
};
