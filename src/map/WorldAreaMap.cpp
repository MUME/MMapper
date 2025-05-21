// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2025 The MMapper Authors

#include "WorldAreaMap.h"

NODISCARD bool AreaInfo::operator==(const AreaInfo &other) const
{
    return roomSet == other.roomSet;
}

void AreaInfo::remove(const RoomId id)
{
    if (!roomSet.contains(id)) {
        return;
    }

    roomSet.erase(id);
}

AreaInfoMap::AreaInfoMap()
{
    m_map[RoomArea{}] = AreaInfo{};
    assert(contains(RoomArea{}));
}

AreaInfo *AreaInfoMap::find(const std::optional<RoomArea> &area)
{
    if (!area.has_value()) {
        return &m_global;
    }
    if (const auto it = m_map.find(area.value()); it != m_map.end()) {
        return &it->second;
    }
    return nullptr;
}

const AreaInfo *AreaInfoMap::find(const std::optional<RoomArea> &area) const
{
    if (!area.has_value()) {
        return &m_global;
    }
    if (const auto it = m_map.find(area.value()); it != m_map.end()) {
        return &it->second;
    }
    return nullptr;
}
AreaInfo &AreaInfoMap::get(const std::optional<RoomArea> &area)
{
    if (AreaInfo *const pArea = find(area)) {
        return *pArea;
    }
    throw std::runtime_error("invalid map area");
}
const AreaInfo &AreaInfoMap::get(const std::optional<RoomArea> &area) const
{
    if (const AreaInfo *const pArea = find(area)) {
        return *pArea;
    }
    throw std::runtime_error("invalid map area");
}

bool AreaInfoMap::operator==(const AreaInfoMap &other) const
{
    return m_map == other.m_map && m_global == other.m_global;
}

void AreaInfoMap::insert(const RoomArea &areaName, const RoomId id)
{
    m_global.roomSet.insert(id);
    if (!contains(areaName)) {
        m_map[areaName] = {};
    }
    get(areaName).roomSet.insert(id);
}

void AreaInfoMap::remove(const RoomArea &areaName, const RoomId id)
{
    m_global.remove(id);
    if (AreaInfo *const pArea = find(areaName)) {
        pArea->remove(id);
    }
}
