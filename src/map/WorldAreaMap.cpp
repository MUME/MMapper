// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2025 The MMapper Authors

#include "WorldAreaMap.h"

NODISCARD bool AreaInfo::operator==(const AreaInfo &other) const
{
    return roomSet == other.roomSet;
}

bool AreaInfo::contains(const RoomId id) const
{
    return roomSet.contains(id);
}

void AreaInfo::remove(const RoomId id)
{
    if (!contains(id)) {
        return;
    }
    roomSet.erase(id);
}

AreaInfoMap::AreaInfoMap()
{
    m_map.set(RoomArea{}, AreaInfo{});
    assert(find(RoomArea{}) != nullptr);
}

void AreaInfoMap::init(const std::unordered_map<RoomArea, AreaInfo> &map,
                       const std::set<RoomId> &global)
{
    m_map.init(map);
    m_global = ImmRoomIdSet{global};
}

const AreaInfo *AreaInfoMap::find(const RoomArea &area) const
{
    return m_map.find(area);
}

const AreaInfo &AreaInfoMap::get(const RoomArea &area) const
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
    m_global.insert(id);

    // REVISIT: use update()?
    m_map.set(areaName, [id, ptr = m_map.find(areaName)]() -> AreaInfo {
        AreaInfo tmp;
        if (ptr != nullptr) {
            tmp = *ptr;
        }
        tmp.roomSet.insert(id);
        return tmp;
    }());
}

void AreaInfoMap::remove(const RoomArea &areaName, const RoomId id)
{
    if (m_global.contains(id)) {
        m_global.erase(id);
    }

    if (const AreaInfo *const it = m_map.find(areaName)) {
        const AreaInfo &info = *it;
        if (info.roomSet.contains(id)) {
            // special case: remove the area when the last room is removed
            if (info.roomSet.size() == 1) {
                m_map.erase(areaName);
            } else {
                m_map.update(areaName, [id](AreaInfo &area) { area.remove(id); });
            }
        }
    }
}
