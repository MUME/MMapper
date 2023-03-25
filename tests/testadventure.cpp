// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "testadventure.h"
#include "../src/adventure/adventuresession.h"
#include "adventure/adventuretracker.h"
#include "observer/gameobserver.h"

#include <algorithm>
#include <QtTest/QtTest>

struct
{
    std::vector<TestLine> achievement1
        = {{false, "An accomplished hunter says 'Good job, Gomgâl! One more to go!'"},
           {false, "You achieved something new!"},
           {true, "You aided the hunter in the Tower Hills by cleaning out a rat infestation."}};
    const char *achievement1Success
        = "You aided the hunter in the Tower Hills by cleaning out a rat infestation.";

    std::vector<TestLine> hint1 = {{false, "It seems to be latched."},
                                   {false, ""},
                                   {false, "# Hint:"},
                                   {true, "#   Type unlock hatch to unlatch the hatch."}};
    const char *hint1Success = "Type unlock hatch to unlatch the hatch.";

    std::vector<TestLine> killMob1
        = {{false, "You cleave a husky smuggler's right leg extremely hard and shatter it."},
           {false, "You receive your share of experience."},
           {false, "Congratulations! This is the first time you've killed it!"},
           {true, "A husky smuggler is dead! R.I.P."}};
    const char *killMob1Success = "A husky smuggler";

    std::vector<TestLine> killMob2
        = {{false, "You cleave a wild bull (x)'s body extremely hard and shatter it."},
           {false, "Your victim is shocked by your hit!"},
           {false, "You receive your share of experience."},
           {false, "Congratulations! This is the first time you've killed it!"},
           {true, "A wild bull (x) is dead! R.I.P."}};
    const char *killMob2Success = "A wild bull (x)"; // TODO FIXME remove the (label) when parsing

    std::vector<TestLine> killMob3
        = {{false, "You cleave a tree-snake's body extremely hard and shatter it."},
           {false, "Your victim is shocked by your hit!"},
           {false, "You receive your share of experience."},
           {false, "Yes! You're beginning to get the idea."},
           {true, "A tree-snake is dead! R.I.P."}};
    const char *killMob3Success = "A tree-snake";

} TestLines;

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

void TestAdventure::testAchievementParser()
{
    AchievementParser parser{};
    testParser(parser, TestLines.achievement1);
    QCOMPARE(parser.getLastSuccessVal(), TestLines.achievement1Success);
}

void TestAdventure::testHintParser()
{
    HintParser parser{};
    testParser(parser, TestLines.hint1);
    QCOMPARE(parser.getLastSuccessVal(), TestLines.hint1Success);
}

void TestAdventure::testKillAndXPParser()
{
    KillAndXPParser parser{};

    testParser(parser, TestLines.killMob1);
    QCOMPARE(parser.getLastSuccessVal(), TestLines.killMob1Success);

    testParser(parser, TestLines.killMob2);
    QCOMPARE(parser.getLastSuccessVal(), TestLines.killMob2Success);

    testParser(parser, TestLines.killMob3);
    QCOMPARE(parser.getLastSuccessVal(), TestLines.killMob3Success);
}

void TestAdventure::testE2E()
{
    GameObserver *observer = new GameObserver();
    AdventureTracker *tracker = new AdventureTracker(*observer);

    std::vector<QString> achievements;
    std::vector<QString> hints;
    std::vector<QString> killedMobs;

    connect(tracker, &AdventureTracker::sig_achievedSomething, [&achievements](QString x) {
        achievements.push_back(x);
    });
    connect(tracker, &AdventureTracker::sig_receivedHint, [&hints](QString x) {
        hints.push_back(x);
    });
    connect(tracker, &AdventureTracker::sig_killedMob, [&killedMobs](QString x) {
        killedMobs.push_back(x);
    });

    auto pump = [observer](std::vector<TestLine> lines) {
        for (auto tl : lines) {
            observer->slot_observeSentToUser(qPrintable(tl.line), true);
        }
    };

    pump(TestLines.achievement1);
    pump(TestLines.hint1);
    pump(TestLines.killMob1);
    pump(TestLines.killMob2);
    pump(TestLines.killMob3);

    QCOMPARE(achievements.size(), 1u);
    QCOMPARE(hints.size(), 1u);
    QCOMPARE(killedMobs.size(), 3u);
}

QTEST_MAIN(TestAdventure)
