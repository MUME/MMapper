// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "KnownRooms.h"

#include "../proxy/GmcpMessage.h"
#include "KnownRoomsCache.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

namespace {
NODISCARD std::optional<ServerRoomId> roomId(const QJsonValue &value)
{
    if (!value.isDouble()) {
        return std::nullopt;
    }
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < 1.0
        || number > static_cast<double>(std::numeric_limits<uint32_t>::max())
        || std::floor(number) < number) {
        return std::nullopt;
    }
    return ServerRoomId{static_cast<uint32_t>(number)};
}
} // namespace

KnownRooms::KnownRooms() = default;
KnownRooms::~KnownRooms() = default;

void KnownRooms::configureCache(const QString &directory,
                                const QString &server,
                                std::function<void()> changed,
                                std::function<void()> missing)
{
    if (m_cache && m_cache->matches(directory, server)) {
        return;
    }
    reset();
    std::erase_if(m_retiredCaches, [](const auto &cache) { return cache->isIdle(); });
    if (m_cache) {
        m_retiredCaches.push_back(std::move(m_cache));
    }
    m_cacheChanged = std::move(changed);
    m_cacheMissing = std::move(missing);
    const auto existing = std::find_if(m_retiredCaches.begin(),
                                       m_retiredCaches.end(),
                                       [&directory, &server](const auto &cache) {
                                           return cache->matches(directory, server);
                                       });
    if (existing != m_retiredCaches.end()) {
        // Reuse the same queue on a rapid round trip; never race two writers for one server.
        m_cache = std::move(*existing);
        m_retiredCaches.erase(existing);
        return;
    }
    m_cache = std::make_unique<KnownRoomsCache>(
        directory,
        server,
        [this](const QString &character,
               const uint64_t generation,
               const std::optional<KnownRoomsCache::Rooms> &rooms) {
            if (rooms) {
                restoreCache(character, generation, *rooms);
            } else if (character == m_character && generation == m_snapshot.generation
                       && !m_snapshot.complete && m_enabled && !m_cacheWarningIssued) {
                m_cacheWarningIssued = true;
                if (m_cacheMissing) {
                    m_cacheMissing();
                }
            }
        });
}

void KnownRooms::loadCache()
{
    if (m_cache && !m_character.isEmpty()) {
        m_cache->load(m_character, m_snapshot.generation);
    }
}

void KnownRooms::saveCache(const bool immediate)
{
    if (m_cache && m_snapshot.complete && !m_character.isEmpty()) {
        m_cache->save(m_character, m_snapshot.rooms, immediate);
    }
}

void KnownRooms::restoreCache(const QString &character,
                              const uint64_t generation,
                              const ImmUnorderedSet<ServerRoomId> &rooms)
{
    if (character != m_character || generation != m_snapshot.generation || m_authoritative
        || m_snapshot.complete) {
        return; // A late read must never replace a newer character or server truth.
    }
    m_snapshot.rooms = rooms;
    m_liveKnown.for_each([this](const ServerRoomId id) { m_snapshot.rooms.insert(id); });
    m_snapshot.complete = true;
    ++m_snapshot.revision;
    if (!m_liveKnown.empty()) {
        saveCache();
    }
    if (m_cacheChanged) {
        m_cacheChanged();
    }
}

void KnownRooms::invalidate()
{
    m_snapshot.rooms = ImmUnorderedSet<ServerRoomId>{};
    m_snapshot.complete = false;
    m_authoritative = false;
    m_liveKnown = ImmUnorderedSet<ServerRoomId>{};
    ++m_snapshot.revision;
    m_snapshot.generation = m_snapshot.revision;
}

void KnownRooms::reset()
{
    invalidate();
    m_pending = ImmUnorderedSet<ServerRoomId>{};
    m_character.clear();
    m_cacheWarningIssued = false;
    m_receiving = false;
    m_discard = false;
    m_refresh = false;
}

bool KnownRooms::requestList()
{
    // GMCP has no request IDs. Drain an old response before requesting a replacement.
    if (m_receiving) {
        m_discard = true;
        m_refresh = m_enabled && !m_character.isEmpty();
        return false;
    }
    if (!m_enabled || m_character.isEmpty()) {
        return false;
    }
    m_pending = ImmUnorderedSet<ServerRoomId>{};
    m_receiving = true;
    m_discard = false;
    m_refresh = false;
    return true;
}

bool KnownRooms::setEnabled(const bool enabled)
{
    if (m_enabled == enabled) {
        return false;
    }
    m_enabled = enabled;
    if (m_cache) {
        // Disabling the overlay does not discard an in-flight refresh or live discoveries.
        if (!enabled || (m_receiving && !m_discard)) {
            return false;
        }
        return requestList();
    }
    invalidate();
    if (!enabled) {
        m_discard = true;
        m_refresh = false;
        return false;
    }
    return requestList();
}

bool KnownRooms::receive(const GmcpMessage &message)
{
    if (message.isCoreGoodbye()) {
        invalidate();
        m_character.clear();
        m_discard = true;
        m_refresh = false;
        return false;
    }
    if (message.isCharName()) {
        const auto &doc = message.getJsonDocument();
        const auto obj = doc ? doc->getObject() : std::nullopt;
        const auto name = obj ? obj->getString("name") : std::nullopt;
        if (!name || name->isEmpty()) {
            invalidate();
            m_character.clear();
            m_discard = true;
            m_refresh = false;
            return false;
        }
        if (*name != m_character) {
            invalidate();
            m_character = *name;
            m_cacheWarningIssued = false;
            loadCache();
            return requestList();
        }
        if (!m_snapshot.complete && !m_receiving) {
            return requestList();
        }
        return false;
    }
    if (message.isRoomKnownUpdated()) {
        // Keep a usable provisional view, but never save an incomplete replacement.
        if (m_cache && m_snapshot.complete) {
            m_authoritative = false;
        } else {
            invalidate();
            loadCache();
        }
        return requestList();
    }
    if (!message.isRoomKnownList() && !message.isRoomKnownAdd()) {
        return false;
    }
    // Still drain a response when the option is disabled during a transfer.
    if (!m_enabled && !m_cache && (!message.isRoomKnownList() || !m_receiving)) {
        return false;
    }

    const auto &json = message.getJson();
    if (!json) {
        if (message.isRoomKnownList() && m_receiving) {
            m_discard = true;
        }
        return false;
    }
    // Wrapping permits strict parsing of scalar JSON on Qt versions where QJsonDocument
    // only accepts arrays and objects, including the protocol's null/false sentinels.
    const auto wrapper = QJsonDocument::fromJson("[" + json->toQByteArray() + "]");
    if (!wrapper.isArray() || wrapper.array().size() != 1) {
        if (message.isRoomKnownList() && m_receiving) {
            m_discard = true;
        }
        return false;
    }
    const auto value = wrapper.array().at(0);
    if (message.isRoomKnownAdd()) {
        if (m_enabled || m_cache) {
            if (const auto id = roomId(value)) {
                m_liveKnown.insert(*id);
                if (m_receiving && !m_discard) {
                    m_pending.insert(*id);
                }
                if (m_snapshot.complete && !m_snapshot.rooms.contains(*id)) {
                    m_snapshot.rooms.insert(*id);
                    ++m_snapshot.revision;
                    saveCache();
                }
            }
        }
        return false;
    }
    if (!m_receiving) {
        return false;
    }
    if (value.isNull() || (value.isBool() && !value.toBool())) {
        m_receiving = false;
        if (value.isNull() && !m_discard && (m_enabled || m_cache)) {
            m_snapshot.rooms = std::move(m_pending);
            m_snapshot.complete = true;
            m_authoritative = true;
            ++m_snapshot.revision;
            saveCache(true);
        }
        m_pending = ImmUnorderedSet<ServerRoomId>{};
        if (m_refresh) {
            return requestList();
        }
        return false;
    }
    if (!value.isArray()) {
        m_discard = true;
        return false;
    }
    if (!m_discard) {
        for (const auto entry : value.toArray()) {
            if (const auto id = roomId(entry)) {
                m_pending.insert(*id);
            } else {
                // A damaged list cannot establish that a room is unknown.
                m_discard = true;
                break;
            }
        }
    }
    return false;
}
