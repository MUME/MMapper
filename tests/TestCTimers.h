#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include <QObject>

class TestCTimers final : public QObject
{
    Q_OBJECT
public:
    TestCTimers();
    ~TestCTimers() final;

private Q_SLOTS:
    void testAddRemoveTimer();
    void testAddRemoveCountdown();
    void testElapsedTime();
    void testCountdownCompletion();
    void testClearFunctionality();
    void testMultipleTimersAndCountdowns();
};
