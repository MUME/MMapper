#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/RuleOf5.h"
#include "../map/RoomHandle.h"
#include "../map/parseevent.h"
#include "../map/room.h"
#include "../map/roomid.h"
#include "pathprocessor.h"

#include <unordered_map>

class MapFrontend;
class ParseEvent;

/*!
 * @brief PathProcessor strategy for the "Approved" pathfinding state.
 *
 * Used when PathMachine is confident of the current room. It attempts to find
 * a single, unambiguous match for incoming event data among directly accessible
 * rooms or by server ID. Manages temporary room cleanup via ChangeList if
 * rooms don't match or if multiple matches occur.
 */
class NODISCARD Approved final : public PathProcessor
{
private:
    SigParseEvent m_myEvent;
    std::unordered_map<RoomId, ComparisonResultEnum> m_compareCache;
    RoomHandle m_matchedRoom;
    MapFrontend &m_map;
    const int m_matchingTolerance;
    bool m_moreThanOne = false;
    bool m_update = false;

public:
    explicit Approved(MapFrontend &map, const SigParseEvent &sigParseEvent, int matchingTolerance);
    ~Approved() final;

public:
    Approved() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(Approved);

private:
    void virt_receiveRoom(const RoomHandle &) final;

public:
    NODISCARD RoomHandle oneMatch() const;
    NODISCARD bool needsUpdate() const { return m_update; }
    void releaseMatch();
};
