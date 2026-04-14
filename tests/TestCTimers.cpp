// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "TestCTimers.h"

#include "../src/global/TextUtils.h"
#include "../src/timers/CTimers.h"
#include "../src/timers/TimerModel.h"

#include <tuple>

#include <QtTest/QtTest>

TestCTimers::TestCTimers() = default;

TestCTimers::~TestCTimers() = default;

void TestCTimers::testAddRemoveTimer()
{
    CTimers timers(nullptr);
    QString timerName = "TestTimer";
    QString timerDesc = "Test Description";

    timers.addTimer(mmqt::toStdStringUtf8(timerName), mmqt::toStdStringUtf8(timerDesc));

    // Verify the added timer is present
    QString timersList = mmqt::toQStringUtf8(timers.getTimers());
    QVERIFY(timersList.contains(timerName));
    QVERIFY(timersList.contains(timerDesc));

    QVERIFY(timers.removeTimer(mmqt::toStdStringUtf8(timerName)));

    // Verify the timer is removed
    timersList = mmqt::toQStringUtf8(timers.getTimers());
    QVERIFY(!timersList.contains(timerName));
}

void TestCTimers::testAddRemoveCountdown()
{
    CTimers timers(nullptr);
    QString countdownName = "TestCountdown";
    QString countdownDesc = "Test Countdown Description";
    int64_t countdownTimeMs = 10000; // 10 seconds

    timers.addCountdown(mmqt::toStdStringUtf8(countdownName),
                        mmqt::toStdStringUtf8(countdownDesc),
                        countdownTimeMs);

    // Verify the added countdown is added
    QString countdownsList = mmqt::toQStringUtf8(timers.getCountdowns());
    QVERIFY(countdownsList.contains(countdownName));
    QVERIFY(countdownsList.contains(countdownDesc));

    QVERIFY(timers.removeCountdown(mmqt::toStdStringUtf8(countdownName)));

    // Verify the countdown is removed
    countdownsList = mmqt::toQStringUtf8(timers.getCountdowns());
    QVERIFY(!countdownsList.contains(countdownName));
}

void TestCTimers::testElapsedTime()
{
    CTimers timers(nullptr);
    QString timerName = "ElapsedTimeTestTimer";
    QString timerDesc = "Elapsed Time Test Description";

    timers.addTimer(mmqt::toStdStringUtf8(timerName), mmqt::toStdStringUtf8(timerDesc));

    QString timersList = mmqt::toQStringUtf8(timers.getTimers());
    QVERIFY(timersList.contains("up for - 0:00"));

    std::ignore = timers.removeTimer(mmqt::toStdStringUtf8(timerName));
}

void TestCTimers::testCountdownCompletion()
{
    CTimers timers(nullptr);
    QString countdownName = "CompletionTestCountdown";
    QString countdownDesc = "Countdown Completion Test";
    int64_t countdownTimeMs = 10000; // 10 seconds

    timers.addCountdown(mmqt::toStdStringUtf8(countdownName),
                        mmqt::toStdStringUtf8(countdownDesc),
                        countdownTimeMs);
    QString countdownsListBefore = mmqt::toQStringUtf8(timers.getCountdowns());
    QVERIFY(countdownsListBefore.contains(countdownName)); // Verify the added countdown is present
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
    QVERIFY(mmqt::toQStringUtf8(timers.getTimers()).isEmpty());
    QVERIFY(mmqt::toQStringUtf8(timers.getCountdowns()).isEmpty());
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
    QVERIFY(!mmqt::toQStringUtf8(timers.getTimers()).isEmpty());
    QVERIFY(!mmqt::toQStringUtf8(timers.getCountdowns()).isEmpty());

    // Remove a timer and a countdown
    QVERIFY(timers.removeTimer("Timer1"));
    QVERIFY(timers.removeCountdown("Countdown1"));

    // Verify removal
    QString timersList = mmqt::toQStringUtf8(timers.getTimers());
    QString countdownsList = mmqt::toQStringUtf8(timers.getCountdowns());
    QVERIFY(!timersList.contains("Timer1"));
    QVERIFY(!countdownsList.contains("Countdown1"));
}

void TestCTimers::testStopResetTimers()
{
    CTimers timers(nullptr);
    timers.addTimer("T1", "D1");
    timers.addCountdown("C1", "D2", 10000);

    // Stop
    timers.stopTimer("T1");
    timers.stopCountdown("C1");

    for (const auto &t : timers.allTimers()) {
        if (t.getName() == "T1" || t.getName() == "C1")
            QVERIFY(t.isExpired());
    }

    // Reset
    timers.resetTimer("T1");
    timers.resetCountdown("C1");

    for (const auto &t : timers.allTimers()) {
        if (t.getName() == "T1" || t.getName() == "C1")
            QVERIFY(!t.isExpired());
    }
}

void TestCTimers::testSignals()
{
    CTimers timers(nullptr);
    QSignalSpy spyAdded(&timers, &CTimers::sig_timerAdded);
    QSignalSpy spyRemoved(&timers, &CTimers::sig_timerRemoved);
    QSignalSpy spyUpdated(&timers, &CTimers::sig_timersUpdated);

    timers.addTimer("T1", "D1");
    QCOMPARE(spyAdded.count(), 1);

    timers.addCountdown("C1", "D2", 10000);
    QCOMPARE(spyAdded.count(), 2);

    timers.stopTimer("T1");
    QCOMPARE(spyUpdated.count(), 1);

    std::ignore = timers.removeTimer("T1");
    QCOMPARE(spyRemoved.count(), 1);

    timers.clear();
    QCOMPARE(spyRemoved.count(), 2);
}

void TestCTimers::testClearExpired()
{
    CTimers timers(nullptr);
    timers.addTimer("T1", "D1");
    timers.addCountdown("C1", "D2", 10000);

    timers.stopTimer("T1");
    timers.stopCountdown("C1");

    timers.clearExpired();

    QVERIFY(timers.allTimers().empty());
}

void TestCTimers::testModelBasicProperties()
{
    CTimers timers(nullptr);
    TimerModel model(timers, nullptr);

    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.columnCount(), 2);

    timers.addTimer("T1", "D1");
    QCOMPARE(model.rowCount(), 1);

    timers.addCountdown("C1", "D2", 10000);
    QCOMPARE(model.rowCount(), 2);
}

void TestCTimers::testModelDataRetrieval()
{
    CTimers timers(nullptr);
    TimerModel model(timers, nullptr);

    timers.addTimer("T1", "D1");

    QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toString(), QString("T1 <D1>"));

    QVariant timeVal = model.data(model.index(0, 1), Qt::DisplayRole);
    QVERIFY(timeVal.isValid());
}

void TestCTimers::testModelUpdates()
{
    CTimers timers(nullptr);
    TimerModel model(timers, nullptr);

    QSignalSpy spyReset(&model, &TimerModel::modelReset);

    timers.addTimer("T1", "D1");
    QCOMPARE(spyReset.count(), 1);

    std::ignore = timers.removeTimer("T1");
    QCOMPARE(spyReset.count(), 2);

    timers.clear();
    QCOMPARE(spyReset.count(), 3);
}

void TestCTimers::testMoveTimer()
{
    CTimers timers(nullptr);
    timers.addTimer("T1", "D1");
    timers.addTimer("T2", "D2");
    timers.addTimer("T3", "D3");

    // Order: T1, T2, T3
    timers.moveTimer(0, 2); // Move T1 to position 2 (before T3)
    // Order should be: T2, T1, T3
    auto all = timers.allTimers();
    auto it = all.begin();
    QCOMPARE(QString::fromStdString(it->getName()), QString("T2"));
    ++it;
    QCOMPARE(QString::fromStdString(it->getName()), QString("T1"));
    ++it;
    QCOMPARE(QString::fromStdString(it->getName()), QString("T3"));

    timers.moveTimer(2, 0); // Move T3 to position 0
    // Order should be: T3, T2, T1
    all = timers.allTimers();
    it = all.begin();
    QCOMPARE(QString::fromStdString(it->getName()), QString("T3"));
}

QTEST_MAIN(TestCTimers)
