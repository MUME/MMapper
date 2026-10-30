#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/ImmUnorderedSet.h"
#include "../map/roomid.h"

#include <chrono>
#include <functional>
#include <future>
#include <map>
#include <optional>

#include <QString>
#include <QTimer>

// Serial background I/O. Only immutable values cross the thread boundary.
// Qt's standard application-data path is supplied by the caller; tests use a temporary directory.
class NODISCARD KnownRoomsCache final
{
public:
    using Rooms = ImmUnorderedSet<ServerRoomId>;
    using Loaded = std::function<void(const QString &, uint64_t, const std::optional<Rooms> &)>;

private:
    struct NODISCARD Read final
    {
        QString character;
        uint64_t generation = 0;
    };
    struct NODISCARD Write final
    {
        Rooms rooms;
        std::chrono::steady_clock::time_point due;
    };
    struct NODISCARD Result final
    {
        std::optional<Read> read;
        std::optional<Rooms> rooms;
        QString error;
    };

    QString m_directory;
    QString m_server;
    Loaded m_loaded;
    QTimer m_timer;
    std::optional<Read> m_read;
    std::map<QString, Write> m_writes;
    std::optional<std::future<Result>> m_future;

    void poll();
    NODISCARD static Result readFile(const QString &directory,
                                     const QString &server,
                                     const Read &read);
    NODISCARD static Result writeFile(const QString &directory,
                                      const QString &server,
                                      const QString &character,
                                      const Rooms &rooms);

public:
    KnownRoomsCache(QString directory, QString server, Loaded loaded);
    ~KnownRoomsCache();
    NODISCARD bool isIdle() const { return !m_future && !m_read && m_writes.empty(); }
    void load(const QString &character, uint64_t generation);
    void save(const QString &character, const Rooms &rooms, bool immediate = false);
    NODISCARD bool matches(const QString &directory, const QString &server) const
    {
        return m_directory == directory && m_server == server;
    }
};
