#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../src/global/macros.h"

#include <QObject>

class NODISCARD_QOBJECT TestProxy final : public QObject
{
    Q_OBJECT

public:
    TestProxy() = default;
    ~TestProxy() override = default;

private Q_SLOTS:
    static void escapeTest();
    static void gmcpMessageDeserializeTest();
    static void gmcpMessageSerializeTest();
    static void gmcpModuleTest();
    static void telnetFilterTest();
    static void knownRoomsHistoryTest();
    static void knownRoomsSwitchTest();
    static void knownRoomsUnavailableTest();
    static void knownRoomsMalformedTest();
    static void knownRoomsRefreshTest();
    static void knownRoomsObserverTest();
    static void knownRoomsCacheMissingWarningTest();
    static void knownRoomsCacheRestartTest();
    static void knownRoomsCacheLiveTest();
    static void knownRoomsCacheInterruptedTest();
    static void knownRoomsCacheIdentityTest();
    static void knownRoomsCacheServerSwitchTest();
    static void knownRoomsCacheCaseFoldTest();
    static void knownRoomsCacheCorruptTest();
    static void knownRoomsCacheTruthWinsTest();
};
