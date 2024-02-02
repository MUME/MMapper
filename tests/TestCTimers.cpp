// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "TestCTimers.h"

#include <QtTest/QtTest>

#include "../src/timers/CTimers.h"

TestCTimers::TestCTimers() = default;

TestCTimers::~TestCTimers() = default;

void TestCTimers::testAddRemoveTimer()
{
    CTimers timers(nullptr);
    QString timerName = "TestTimer";
    QString timerDesc = "Test Description";

    timers.addTimer(timerName.toStdString(), timerDesc.toStdString());

    // Verify the added timer is present
    QString timersList = QString::fromStdString(timers.getTimers());
    QVERIFY(timersList.contains(timerName));
    QVERIFY(timersList.contains(timerDesc));

    QVERIFY(timers.removeTimer(timerName.toStdString()));

    // Verify the timer is removed
    timersList = QString::fromStdString(timers.getTimers());
    QVERIFY(!timersList.contains(timerName));
}

void TestCTimers::testAddRemoveCountdown()
{
    CTimers timers(nullptr);
    QString countdownName = "TestCountdown";
    QString countdownDesc = "Test Countdown Description";
    int64_t countdownTimeMs = 10000; // 10 seconds

    timers.addCountdown(countdownName.toStdString(), countdownDesc.toStdString(), countdownTimeMs);

    // Verify the added countdown is added
    QString countdownsList = QString::fromStdString(timers.getCountdowns());
    QVERIFY(countdownsList.contains(countdownName));
    QVERIFY(countdownsList.contains(countdownDesc));

    QVERIFY(timers.removeCountdown(countdownName.toStdString()));

    // Verify the countdown is removed
    countdownsList = QString::fromStdString(timers.getCountdowns());
    QVERIFY(!countdownsList.contains(countdownName));
}

void TestCTimers::testElapsedTime()
{
    CTimers timers(nullptr);
    QString timerName = "ElapsedTimeTestTimer";
    QString timerDesc = "Elapsed Time Test Description";

    timers.addTimer(timerName.toStdString(), timerDesc.toStdString());

//    QTest::qWait(1100); // Wait 1.1 seconds

    QString timersList = QString::fromStdString(timers.getTimers());
    QVERIFY(timersList.contains("up for - 0:00"));

    timers.removeTimer(timerName.toStdString());
}

void TestCTimers::testCountdownCompletion()
{
    CTimers timers(nullptr);
    QString countdownName = "CompletionTestCountdown";
    QString countdownDesc = "Countdown Completion Test";
    int64_t countdownTimeMs = 10000; // 10 seconds

    timers.addCountdown(countdownName.toStdString(), countdownDesc.toStdString(), countdownTimeMs);
    QString countdownsListBefore = QString::fromStdString(timers.getCountdowns());
    QVERIFY(countdownsListBefore.contains(countdownName)); // Verify the added countdown is present
    QString countdownsList = QString::fromStdString(timers.getCountdowns());
    QVERIFY(countdownsList.contains("(up for - 0:00, left - 0:10)"));
}

void TestCTimers::testClearFunctionality()
{
    CTimers timers(nullptr);

    // Add multiple timers and countdowns
    timers.addTimer("Timer1", "Description1");
    timers.addTimer("Timer2", "Description1");
    timers.addCountdown("Countdown1", "Description1", 5000);
    timers.addCountdown("Countdown2", "Description2", 5000);

    // Clear all timers and countdowns
    timers.clear();

    // Verify all timers and countdowns are cleared
    QVERIFY(QString::fromStdString(timers.getTimers()).isEmpty());
    QVERIFY(QString::fromStdString(timers.getCountdowns()).isEmpty());
}

void TestCTimers::testMultipleTimersAndCountdowns()
{
    CTimers timers(nullptr);

    // Add multiple timers
    timers.addTimer("Timer1", "Description1");
    timers.addTimer("Timer2", "Description2");

    // Add multiple countdowns
    timers.addCountdown("Countdown1", "Description1", 5000);
    timers.addCountdown("Countdown2", "Description2", 10000);

    // Verify timers and countdowns are added
    QVERIFY(!QString::fromStdString(timers.getTimers()).isEmpty());
    QVERIFY(!QString::fromStdString(timers.getCountdowns()).isEmpty());

    // Remove a timer and a countdown
    QVERIFY(timers.removeTimer("Timer1"));
    QVERIFY(timers.removeCountdown("Countdown1"));

    // Verify removal
    QString timersList = QString::fromStdString(timers.getTimers());
    QString countdownsList = QString::fromStdString(timers.getCountdowns());
    QVERIFY(!timersList.contains("Timer1"));
    QVERIFY(!countdownsList.contains("Countdown1"));
}

QTEST_MAIN(TestCTimers)
