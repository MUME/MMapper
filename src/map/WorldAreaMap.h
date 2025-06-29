#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2025 The MMapper Authors

#include "../global/ImmUnorderedMap.h"
#include "../global/macros.h"
#include "RoomIdSet.h"
#include "mmapper2room.h"

#include <unordered_map>

struct NODISCARD AreaInfo final
{
    ImmUnorderedRoomIdSet roomSet;

    NODISCARD bool contains(RoomId id) const;
    NODISCARD bool operator==(const AreaInfo &other) const;
    void remove(RoomId id);
};

// Note: RoomArea{} is not the same as the global area.
// RoomArea{} contains rooms that do not specify an area,
// while the global area contains all rooms.
struct NODISCARD AreaInfoMap final
{
private:
    using Map = ImmUnorderedMap<RoomArea, AreaInfo>;
    Map m_map;
    // Note: global area must be ordered
    ImmRoomIdSet m_global;

public:
    NODISCARD explicit AreaInfoMap();
    void init(const std::unordered_map<RoomArea, AreaInfo> &map, const ImmRoomIdSet &global);

public:
    NODISCARD const ImmRoomIdSet &getGlobal() const { return m_global; }

public:
    NODISCARD const AreaInfo *find(const RoomArea &area) const;

    // get will throw if not found or wrong type is requested implicitly
    NODISCARD const AreaInfo &get(const RoomArea &area) const;

    NODISCARD bool operator==(const AreaInfoMap &other) const;
    NODISCARD size_t numAreas() const { return m_map.size(); }

    NODISCARD auto begin() const { return m_map.begin(); }
    NODISCARD auto end() const { return m_map.end(); }

public:
    void insert(const RoomArea &areaName, RoomId id);
    void remove(const RoomArea &areaName, RoomId id);
};
