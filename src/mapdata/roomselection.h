#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/Badge.h"
#include "../global/MmQtHandle.h"
#include "../global/NullPointerException.h"
#include "../global/RAII.h"
#include "../global/RuleOf5.h"
#include "../map/RoomRecipient.h"
#include "../map/roomid.h"
#include "roomfilter.h"

#include <memory>
#include <unordered_map>

class MapData;
class RoomSelection;
using SharedRoomSelection = std::shared_ptr<RoomSelection>;
using SigRoomSelection = MmQtHandle<RoomSelection>;

// Thin wrapper around RoomIdSet
class NODISCARD RoomSelection final : public std::enable_shared_from_this<RoomSelection>
{
private:
    RoomIdSet m_set;

public:
    explicit RoomSelection(Badge<RoomSelection>, RoomIdSet set);

public:
    NODISCARD static SharedRoomSelection createSelection(RoomIdSet set);

public:
    NODISCARD auto begin() const { return m_set.begin(); }
    NODISCARD auto end() const { return m_set.end(); }
    NODISCARD size_t size() const { return m_set.size(); }
    NODISCARD bool empty() const { return m_set.empty(); }
    NODISCARD bool contains(const RoomId id) const { return m_set.contains(id); }
    NODISCARD RoomId getFirstRoomId() const;

public:
    void insert(const RoomId id) { m_set.insert(id); }
    void insert(const RoomHandle &room);
    void erase(RoomId targetId) { m_set.erase(targetId); }
    void clear() { m_set = {}; }

public:
    void removeMissing(MapData &mapData);

public:
    DELETE_CTORS_AND_ASSIGN_OPS(RoomSelection);
    ~RoomSelection() = default;
};
