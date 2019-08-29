#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <QObject>

class TestClock : public QObject
{
    Q_OBJECT
public:
    TestClock();
    ~TestClock() override;

private:
private Q_SLOTS:
    // MumeClock
    void mumeClockTest();
    void parseMumeTimeTest();
    void parseWeatherTest();
    void parseWeatherClockSkewTest();
    void parseClockTimeTest();
    void precsionTimeoutTest();
    void moonClockTest();
};
