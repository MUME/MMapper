#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../global/ImmUnorderedMap.h"
#include "../global/macros.h"
#include "room.h"

class AnsiOstream;
class ProgressCounter;

struct NODISCARD ServerIdMap final
{
private:
    ImmUnorderedMap<ServerRoomId, RoomId> m_serverToInternal;

public:
    NODISCARD auto empty() const { return m_serverToInternal.empty(); }
    NODISCARD auto size() const { return m_serverToInternal.size(); }
    NODISCARD bool contains(const ServerRoomId serverId) const
    {
        return lookup(serverId).has_value();
    }
    NODISCARD std::optional<RoomId> lookup(const ServerRoomId serverId) const
    {
        if (auto it = m_serverToInternal.find(serverId); it != nullptr) {
            return *it;
        }
        return std::nullopt;
    }
    void set(const ServerRoomId serverId, const RoomId id)
    {
        if (serverId != INVALID_SERVER_ROOMID && id != INVALID_ROOMID) {
            m_serverToInternal.set(serverId, id);
        }
    }
    void remove(const ServerRoomId serverId)
    {
        if (serverId != INVALID_SERVER_ROOMID) {
            m_serverToInternal.erase(serverId);
        }
    }

    // Callback = void(ServerRoomId, RoomId);
    template<typename Callback>
    void for_each(Callback &&callback) const
    {
        static_assert(std::is_invocable_r_v<void, Callback, ServerRoomId, RoomId>);
        m_serverToInternal.for_each([&callback](const auto &p) { callback(p.first, p.second); });
    }

    void printStats(ProgressCounter &pc, AnsiOstream &os) const;

    NODISCARD bool operator==(const ServerIdMap &rhs) const
    {
        return m_serverToInternal == rhs.m_serverToInternal;
    }
    NODISCARD bool operator!=(const ServerIdMap &rhs) const { return !(rhs == *this); }
};
