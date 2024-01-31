#pragma once

#include <QObject>

class TestCTimers final : public QObject
{
    Q_OBJECT
public:
    TestCTimers();
    ~TestCTimers() final;

private:
private Q_SLOTS:
    void testAddRemoveTimer();
    void testAddRemoveCountdown();
    void testElapsedTime();
    void testCountdownCompletion();
    void testClearFunctionality();
    void testMultipleTimersAndCountdowns();
};
