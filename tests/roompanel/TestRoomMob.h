#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include <QObject>

class TestRoomMob final : public QObject
{
    Q_OBJECT
public:
    TestRoomMob();
    ~TestRoomMob() final;

private:
private Q_SLOTS:
    void testInitialization();
    void testSetGetId();
    void testSetGetField();
    void testAllocAndUpdate();
    void testFlagsAndFields();
};
