// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "FogOfWarManager.h"

#include "../global/JsonArray.h"
#include "../global/JsonValue.h"

FogOfWarManager::FogOfWarManager(QObject *parent)
    : QObject(parent)
{}

FogOfWarManager::~FogOfWarManager() = default;

void FogOfWarManager::onRoomKnownAdd(ServerRoomId id)
{
    if (id == INVALID_SERVER_ROOMID)
        return;

    const auto [it, inserted] = m_knownRooms.insert(id);
    if (inserted) {
        emit sig_fogDataChanged();
    }
}

void FogOfWarManager::onRoomKnownList(const JsonArray &ids)
{
    bool changed = false;

    for (const JsonValue &value : ids) {
        if (auto optInt = value.getInt()) {
            const JsonInt roomInt = *optInt;
            if (roomInt > 0) {
                const auto id = ServerRoomId{static_cast<uint32_t>(roomInt)};
                const auto [it, inserted] = m_knownRooms.insert(id);
                if (inserted)
                    changed = true;
            }
        }
    }

    m_listReceived = true;

    if (changed) {
        emit sig_fogDataChanged();
    }
}

void FogOfWarManager::onRoomKnownUpdated()
{
    // Server signals that the Room.Known list has changed
    // Clear current data and request new list
    m_knownRooms.clear();
    m_listReceived = false;
    m_listComplete = false;

    emit sig_requestRoomKnownList();
    emit sig_fogDataChanged();
}

void FogOfWarManager::setListComplete()
{
    m_listComplete = true;
}

bool FogOfWarManager::isRoomKnown(ServerRoomId id) const
{
    if (id == INVALID_SERVER_ROOMID)
        return true; // Don't fog invalid rooms

    return m_knownRooms.find(id) != m_knownRooms.end();
}

void FogOfWarManager::setCurrentCharacter(const CharacterName &name)
{
    if (m_currentCharacter != name) {
        // Character changed, clear current fog data
        m_currentCharacter = name;
        m_knownRooms.clear();
        m_listReceived = false;
        m_listComplete = false;

        // Request new list for this character
        emit sig_requestRoomKnownList();
        emit sig_fogDataChanged();
    }
}
