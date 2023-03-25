// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "testadventure.h"
#include "../src/adventure/adventuresession.h"

#include <QtTest/QtTest>

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

void TestAdventure::testParser(AbstractLineParser &parser, std::vector<TestLine> testLines)
{
    for (auto &tl : testLines) {
        QVERIFY2(parser.parse(tl.line) == tl.expected, qPrintable(tl.errorMsg()));
    }
}

void TestAdventure::testAccomplishedTaskParser() {}

void TestAdventure::testAchievementParser() {}

void TestAdventure::testHintParser() {}

void TestAdventure::testKillAndXPParser()
{
    KillAndXPParser parser{};

    testParser(parser,
               {{false, "You cleave a husky smuggler's right leg extremely hard and shatter it."},
                {false, "You receive your share of experience."},
                {false, "Congratulations! This is the first time you've killed it!"},
                {true, "A husky smuggler is dead! R.I.P."}});
    QCOMPARE(parser.getLastSuccessVal(), "A husky smuggler");

    testParser(parser,
               {{false, "You cleave a wild bull (x)'s body extremely hard and shatter it."},
                {false, "Your victim is shocked by your hit!"},
                {false, "You receive your share of experience."},
                {false, "Congratulations! This is the first time you've killed it!"},
                {true, "A wild bull (x) is dead! R.I.P."}});
    QCOMPARE(parser.getLastSuccessVal(), "A wild bull (x)"); // TODO FIXME remove the (label)

    testParser(parser,
               {{false, "You cleave a tree-snake's body extremely hard and shatter it."},
                {false, "Your victim is shocked by your hit!"},
                {false, "You receive your share of experience."},
                {false, "Yes! You're beginning to get the idea."},
                {true, "A tree-snake is dead! R.I.P."}});
    QCOMPARE(parser.getLastSuccessVal(), "A tree-snake");
}

QTEST_MAIN(TestAdventure)
