// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "testadventure.h"
#include "../src/adventure/adventuresession.h"
#include "../src/adventure/lineparsers.h"

#include <QtTest/QtTest>

TestAdventure::TestAdventure() = default;

TestAdventure::~TestAdventure() = default;

void TestAdventure::testSessionHourlyRateXP()
{
    AdventureSession session{"ChillbroBaggins"};

    session.updateXP(0.0);
    session.updateXP(60000.0);
    session.endSession(); // must endSession() else will keep internally using now() for endTime

    QCOMPARE(session.xp().gainedSession(), 60000.0);

    // 20 minutes, hourly rate = 60k x 3 = 180k
    session.m_endTimePoint = session.m_startTimePoint + std::chrono::minutes(20);
    QCOMPARE(session.calculateHourlyRateXP(), 180000.0);

    // 30 minutes, hourly rate = 60k x 2 = 120k
    session.m_endTimePoint = session.m_startTimePoint + std::chrono::minutes(30);
    QCOMPARE(session.calculateHourlyRateXP(), 120000.0);

    // 60 minutes, hourly rate = 60k x 1 = 60k
    session.m_endTimePoint = session.m_startTimePoint + std::chrono::minutes(60);
    QCOMPARE(session.calculateHourlyRateXP(), 60000.0);

    // 90 minutes, hourly rate = 60k x 2/3 = 40k
    session.m_endTimePoint = session.m_startTimePoint + std::chrono::minutes(90);
    QCOMPARE(session.calculateHourlyRateXP(), 40000.0);

    // 120 minutes, hourly rate = 60k x 1/2 = 30k
    session.m_endTimePoint = session.m_startTimePoint + std::chrono::minutes(120);
    QCOMPARE(session.calculateHourlyRateXP(), 30000.0);

    // 600 minutes, hourly rate = 60k x 1/10 = 10k
    session.m_endTimePoint = session.m_startTimePoint + std::chrono::minutes(600);
    QCOMPARE(session.calculateHourlyRateXP(), 6000.0);
}

void TestAdventure::testAccomplishedTaskParser() {}

void TestAdventure::testAchievementParser() {}

void TestAdventure::testDiedParser() {}

void TestAdventure::testGainedLevelParser() {}

void TestAdventure::testHintParser() {}

void TestAdventure::testKillAndXPParser()
{
    std::vector<QString> lines = {"foo", "bar", "baz"};
    std::vector<bool> parseResults{};

    KillAndXPParser parser{};

    //std::for_each(lines.begin(), lines.end(), [parser, parseResults](QString l){parseResults.insert(parser.parse(l))})
}

QTEST_MAIN(TestAdventure)
