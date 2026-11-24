// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "KnownRoomsCache.h"

#include <cmath>
#include <limits>
#include <utility>

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace {
constexpr qint64 MAX_CACHE_BYTES = 8 * 1024 * 1024;
constexpr qsizetype MAX_CACHE_ROOMS = 500000;

QString cacheFilename(const QString &directory, const QString &server, const QString &character)
{
    // Neither server names nor character names can introduce path components.
    const auto key = server.toUtf8() + '\0' + character.toCaseFolded().toUtf8();
    const auto hash = QCryptographicHash::hash(key, QCryptographicHash::Sha256).toHex();
    return QDir(directory).filePath(QString::fromLatin1(hash) + ".json");
}

void report(const QString &error)
{
    if (!error.isEmpty()) {
        qWarning().noquote() << "[KnownRooms cache]" << error;
    }
}
} // namespace

KnownRoomsCache::KnownRoomsCache(QString directory, QString server, Loaded loaded)
    : m_directory{std::move(directory)}
    , m_server{std::move(server)}
    , m_loaded{std::move(loaded)}
{
    m_timer.setInterval(25);
    QObject::connect(&m_timer, &QTimer::timeout, &m_timer, [this]() { poll(); });
}

KnownRoomsCache::~KnownRoomsCache()
{
    m_timer.stop();
    // Joining is restricted to teardown, never movement, disconnection, or character changes.
    if (m_future) {
        try {
            report(m_future->get().error);
        } catch (const std::exception &e) {
            qWarning() << "[KnownRooms cache]" << e.what();
        }
    }
    // Flush the final coalesced updates on ordinary application shutdown.
    for (const auto &[character, write] : m_writes) {
        report(writeFile(m_directory, m_server, character, write.rooms).error);
    }
}

void KnownRoomsCache::load(const QString &character, const uint64_t generation)
{
    m_read = Read{character, generation};
    if (!m_timer.isActive()) {
        m_timer.start();
    }
    poll();
}

void KnownRoomsCache::save(const QString &character, const Rooms &rooms, const bool immediate)
{
    const auto now = std::chrono::steady_clock::now();
    auto [it, inserted] = m_writes.try_emplace(character.toCaseFolded(),
                                               Write{rooms, now + std::chrono::seconds(2)});
    if (!inserted) {
        it->second.rooms = rooms;
    }
    // Keep the original deadline during continuous movement: writes cannot starve.
    if (immediate) {
        it->second.due = now;
    }
    if (!m_timer.isActive()) {
        m_timer.start();
    }
}

void KnownRoomsCache::poll()
{
    try {
        if (m_future) {
            if (m_future->wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
                return;
            }
            std::optional<Result> result;
            try {
                result = m_future->get();
            } catch (const std::exception &e) {
                qWarning() << "[KnownRooms cache]" << e.what();
            }
            m_future.reset();
            if (result) {
                report(result->error);
                if (result->read) {
                    m_loaded(result->read->character, result->read->generation, result->rooms);
                }
            }
        }
        // A newer in-memory snapshot must reach disk before a read for that character.
        // This also handles rapidly switching away from and back to a character.
        if (m_read) {
            if (auto it = m_writes.find(m_read->character.toCaseFolded()); it != m_writes.end()) {
                it->second.due = std::chrono::steady_clock::now();
            } else {
                const auto read = std::exchange(m_read, std::nullopt).value();
                m_future = std::async(std::launch::async,
                                      [directory = m_directory, server = m_server, read]() {
                                          return readFile(directory, server, read);
                                      });
                return;
            }
        }
        const auto now = std::chrono::steady_clock::now();
        for (auto it = m_writes.begin(); it != m_writes.end(); ++it) {
            if (it->second.due <= now) {
                const auto character = it->first;
                const auto rooms = it->second.rooms;
                m_writes.erase(it);
                m_future
                    = std::async(std::launch::async,
                                 [directory = m_directory, server = m_server, character, rooms]() {
                                     return writeFile(directory, server, character, rooms);
                                 });
                return;
            }
        }
        if (!m_read && m_writes.empty()) {
            m_timer.stop();
        }
    } catch (const std::exception &e) {
        // A cache failure must not stop play or the authoritative GMCP transfer.
        qWarning() << "[KnownRooms cache]" << e.what();
        m_read.reset();
        m_timer.stop();
    }
}

KnownRoomsCache::Result KnownRoomsCache::readFile(const QString &directory,
                                                  const QString &server,
                                                  const Read &read)
{
    Result result;
    result.read = read;
    QFile file(cacheFilename(directory, server, read.character));
    if (!file.open(QIODevice::ReadOnly) || file.size() > MAX_CACHE_BYTES) {
        return result;
    }
    const auto bytes = file.read(MAX_CACHE_BYTES + 1);
    if (bytes.size() > MAX_CACHE_BYTES) {
        return result;
    }
    const auto doc = QJsonDocument::fromJson(bytes);
    if (!doc.isObject()) {
        return result;
    }
    const auto obj = doc.object();
    if (obj.value("version").toInt() != 1 || obj.value("server").toString() != server
        || obj.value("character").toString() != read.character.toCaseFolded()
        || !obj.value("rooms").isArray()) {
        return result;
    }
    const auto ids = obj.value("rooms").toArray();
    if (ids.size() > MAX_CACHE_ROOMS) {
        return result;
    }
    Rooms rooms;
    for (const auto value : ids) {
        const double number = value.toDouble();
        if (!value.isDouble() || !std::isfinite(number) || number < 1.0
            || number > static_cast<double>(std::numeric_limits<uint32_t>::max())
            || std::floor(number) < number) {
            return result;
        }
        rooms.insert(ServerRoomId{static_cast<uint32_t>(number)});
    }
    result.rooms = std::move(rooms);
    return result;
}

KnownRoomsCache::Result KnownRoomsCache::writeFile(const QString &directory,
                                                   const QString &server,
                                                   const QString &character,
                                                   const Rooms &rooms)
{
    Result result;
    if (!QDir().mkpath(directory)) {
        result.error = "Cannot create the known-room cache directory.";
        return result;
    }
    QJsonArray ids;
    rooms.for_each(
        [&ids](const ServerRoomId id) { ids.append(static_cast<double>(id.asUint32())); });
    const QJsonObject obj{{"version", 1},
                          {"server", server},
                          {"character", character.toCaseFolded()},
                          {"rooms", ids}};
    const auto bytes = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QSaveFile file(cacheFilename(directory, server, character));
    // QSaveFile defaults to atomic replacement; do not enable direct-write fallback.
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        result.error = "Cannot save the known-room cache: " + file.errorString();
    }
    return result;
}
