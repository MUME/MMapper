#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include <QObject>

class TestRoomMobs final : public QObject
{
    Q_OBJECT
public:
    TestRoomMobs();
    ~TestRoomMobs() final;

private:
private Q_SLOTS:
    void testAddMob();
    void testRemoveMobById();
    void testUpdateMob();
    void testResetMobs();
};
