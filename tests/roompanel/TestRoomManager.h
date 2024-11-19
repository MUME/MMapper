#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "roompanel/RoomManager.h"

#include <QObject>

class TestRoomManager final : public QObject
{
    Q_OBJECT

private:
    RoomManager &m_manager;

public:
    explicit TestRoomManager(RoomManager &manager);
    ~TestRoomManager() final;

private:
private Q_SLOTS:
    void init();
    void cleanup();
    void testSlotReset();
    void testParseGmcpAddValidMessage();
    void testParseGmcpInvalidMessage();
    void testParseGmcpUpdateValidMessage();
};
