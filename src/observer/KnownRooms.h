#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/ImmUnorderedSet.h"
#include "../map/roomid.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include <QString>

class GmcpMessage;
class KnownRoomsCache;

// Per-connection, per-character state. A cache is usable but provisional; only a
// terminated Room.Known.List is authoritative.
// Persistent sets give the renderer cheap immutable snapshots without copying all room IDs.
class NODISCARD KnownRooms final
{
public:
    struct NODISCARD Snapshot final
    {
        ImmUnorderedSet<ServerRoomId> rooms;
        uint64_t revision = 0;
        uint64_t generation = 0;
        // A complete previous cache is also usable while the server list refreshes.
        bool complete = false;

        NODISCARD bool isUndiscovered(const ServerRoomId id) const
        {
            return complete && id != INVALID_SERVER_ROOMID && !rooms.contains(id);
        }
    };

private:
    Snapshot m_snapshot;
    ImmUnorderedSet<ServerRoomId> m_liveKnown;
    bool m_authoritative = false;
    ImmUnorderedSet<ServerRoomId> m_pending;
    QString m_character;
    bool m_enabled = false;
    bool m_receiving = false;
    bool m_discard = false;
    bool m_refresh = false;

    std::unique_ptr<KnownRoomsCache> m_cache;
    // Keep old server workers alive until idle; destroying std::async futures can wait.
    std::vector<std::unique_ptr<KnownRoomsCache>> m_retiredCaches;
    std::function<void()> m_cacheChanged;
    std::function<void()> m_cacheMissing;
    bool m_cacheWarningIssued = false;
    void loadCache();
    void saveCache(bool immediate = false);
    void restoreCache(const QString &character,
                      uint64_t generation,
                      const ImmUnorderedSet<ServerRoomId> &rooms);
    void invalidate();
    NODISCARD bool requestList();

public:
    KnownRooms();
    ~KnownRooms();
    void configureCache(const QString &directory,
                        const QString &server,
                        std::function<void()> changed,
                        std::function<void()> missing = {});
    NODISCARD const Snapshot &getSnapshot() const { return m_snapshot; }
    void reset();
    // True means the caller must send one Room.Known.List request.
    NODISCARD bool setEnabled(bool enabled);
    NODISCARD bool receive(const GmcpMessage &message);
};
