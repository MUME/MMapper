#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../group/mmapper2character.h"
#include "../map/roomid.h"

#include <unordered_set>

#include <QObject>

class JsonArray;

class NODISCARD_QOBJECT FogOfWarManager final : public QObject
{
    Q_OBJECT

private:
    CharacterName m_currentCharacter;
    std::unordered_set<ServerRoomId> m_knownRooms;
    bool m_listReceived = false;
    bool m_listComplete = false;

public:
    explicit FogOfWarManager(QObject *parent);
    ~FogOfWarManager() final;

    // GMCP message handlers
    void onRoomKnownAdd(ServerRoomId id);
    void onRoomKnownList(const JsonArray &ids);
    void onRoomKnownUpdated();
    void setListComplete();

    // Query
    NODISCARD bool isRoomKnown(ServerRoomId id) const;
    NODISCARD bool isListReceived() const { return m_listReceived; }

    // Character management
    void setCurrentCharacter(const CharacterName &name);
    NODISCARD const CharacterName &getCurrentCharacter() const { return m_currentCharacter; }

    // For debugging
    NODISCARD size_t getKnownRoomCount() const { return m_knownRooms.size(); }

signals:
    void sig_fogDataChanged();
    void sig_requestRoomKnownList();
};
