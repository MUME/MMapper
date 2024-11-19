// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "testadventure.h"

#include "../src/adventure/adventuresession.h"
#include "../src/adventure/adventuretracker.h"
#include "../src/global/Charset.h"
#include "../src/global/HideQDebug.h"
#include "../src/global/tests.h"
#include "../src/observer/gameobserver.h"

#include <QtTest/QtTest>

namespace { // anonymous

template<size_t N>
NODISCARD std::string_view arrToStdSv(const std::array<char, N> &arr)
{
    TEST_ASSERT(N != 0);
    TEST_ASSERT(arr.back() != char_consts::C_NUL);
    return std::string_view{arr.data(), N};
}

void testQStringAsAscii(const QString &qs, std::string_view expected_ascii)
{
    const auto utf8 = mmqt::toStdStringUtf8(qs);
    const auto ascii = charset::conversion::utf8ToAscii(utf8);
    TEST_ASSERT(ascii == expected_ascii);
}

const QString dunadan = []() -> QString {
    const std::array<char, 7> latin1_bytes = {'D', '\xFA', 'n', 'a', 'd', 'a', 'n'};
    const std::array<char, 8> utf8_bytes = {'D', '\xC3', '\xBA', 'n', 'a', 'd', 'a', 'n'};
    TEST_ASSERT(charset::conversion::latin1ToAscii('\xFA') == 'u'); // sanity checking

    const QString result = "D\u00FAnadan";
    TEST_ASSERT(mmqt::toStdStringUtf8(result) == arrToStdSv(utf8_bytes));
    TEST_ASSERT(mmqt::toStdStringLatin1(result) == arrToStdSv(latin1_bytes)); // sanity checking
    testQStringAsAscii(result, "Dunadan");

    return result;
}();

const auto gomgal = []() {
    const std::array<char, 6> latin1_bytes = {'G', 'o', 'm', 'g', '\xE2', 'l'};
    const std::array<char, 7> utf8_bytes = {'G', 'o', 'm', 'g', '\xC3', '\xA2', 'l'};

    const QString result = "Gomg\u00E2l";
    TEST_ASSERT(mmqt::toStdStringUtf8(result) == arrToStdSv(utf8_bytes));
    TEST_ASSERT(mmqt::toStdStringLatin1(result) == arrToStdSv(latin1_bytes)); // sanity checking
    testQStringAsAscii(result, "Gomgal");

    return result;
}();

QDebug operator<<(QDebug debug, const LineParserResult &result)
{
    if (result) {
        debug << "(result: " << result.value() << ")";
    } else {
        debug << "(no result expected)";
    }
    return debug;
}

struct NODISCARD TestLine final
{
    QString line;
    LineParserResult expected;

    explicit TestLine(QString moved_line, LineParserResult moved_result)
        : line{std::move(moved_line)}
        , expected{std::move(moved_result)}
    {}

    friend QDebug operator<<(QDebug debug, const TestLine &test)
    {
        debug << "line:" << test.line;
        debug << "expected:" << test.expected;
        return debug;
    }
};

namespace TestLines {

const auto achievement1 = std::vector<TestLine>{
    TestLine{"An accomplished hunter says 'Good job, " + gomgal + "! One more to go!'",
             std::nullopt},
    TestLine{"You achieved something new!", std::nullopt},
    TestLine{"You aided the hunter in the Tower Hills by cleaning out a rat infestation.",
             LineParserResult{
                 "You aided the hunter in the Tower Hills by cleaning out a rat infestation."}}};

const auto hint1
    = std::vector<TestLine>{TestLine{"It seems to be latched.", std::nullopt},
                            TestLine{"", std::nullopt},
                            TestLine{"# Hint:", std::nullopt},
                            TestLine{"#   Type unlock hatch to unlatch the hatch.",
                                     LineParserResult{"Type unlock hatch to unlatch the hatch."}},
                            TestLine{"#   Type unlock hatch to unlatch the hatch.", std::nullopt}};

const auto killMob1 = std::vector<TestLine>{
    TestLine{"You cleave a husky smuggler's right leg extremely hard and shatter it.", std::nullopt},
    TestLine{"You receive your share of experience.", std::nullopt},
    TestLine{"Congratulations! This is the first time you've killed it!", std::nullopt},
    TestLine{"A husky smuggler is dead! R.I.P.", LineParserResult{"A husky smuggler"}}};

const auto killMob2 = std::vector<TestLine>{
    TestLine{"You cleave a wild bull (x)'s body extremely hard and shatter it.", std::nullopt},
    TestLine{"Your victim is shocked by your hit!", std::nullopt},
    TestLine{"You receive your share of experience.", std::nullopt},
    TestLine{"Congratulations! This is the first time you've killed it!", std::nullopt},
    TestLine{"A wild bull (x) is dead! R.I.P.", LineParserResult{"A wild bull (x)"}}};

const auto killMob3 = std::vector<TestLine>{
    TestLine{"You cleave a tree-snake's body extremely hard and shatter it.", std::nullopt},
    TestLine{"Your victim is shocked by your hit!", std::nullopt},
    TestLine{"You receive your share of experience.", std::nullopt},
    TestLine{"Yes! You're beginning to get the idea.", std::nullopt},
    TestLine{"A tree-snake is dead! R.I.P.", LineParserResult{"A tree-snake"}}};

const auto killMob4
    = std::vector<TestLine>{TestLine{"You cleave a spirit's body extremely hard and shatter it.",
                                     std::nullopt},
                            TestLine{"You receive your share of experience.", std::nullopt},
                            TestLine{"**Yawn** Boring kill, wasn't it?", std::nullopt},
                            TestLine{"A spirit disappears into nothing.",
                                     LineParserResult{"A spirit"}}};

const auto killPlayer1 = std::vector<TestLine>{
    TestLine{"You pierce *an Elf* (k)'s right hand extremely hard and shatter it.", std::nullopt},
    TestLine{"You feel more experienced.", std::nullopt},
    TestLine{"Congratulations! This is the first time you've killed it!", std::nullopt},
    TestLine{
        "You feel revitalized as the dark power within you drains the last bit of life from *an Elf* (k).",
        std::nullopt},
    TestLine{"You are surrounded by a misty shroud.", std::nullopt},
    TestLine{"You hear *an Elf* (k)'s death cry as he collapses.", std::nullopt},
    TestLine{"*an Elf* (k) has drawn his last breath! R.I.P.", LineParserResult{"*an Elf* (k)"}},
    TestLine{"A shadow slowly rises above the corpse of *an Elf* (k).", std::nullopt}};

const auto killPlayer2 = std::vector<TestLine>{
    TestLine{"You slash *a Half-Elf*'s right hand extremely hard and shatter it.", std::nullopt},
    TestLine{"Your victim is shocked by your hit!", std::nullopt},
    TestLine{"You feel more experienced.", std::nullopt},
    TestLine{"Yes! You're beginning to get the idea.", std::nullopt},
    TestLine{"You hear *a Half-Elf*'s death cry as she collapses.", std::nullopt},
    TestLine{"*a Half-Elf* has drawn her last breath! R.I.P.", LineParserResult{"*a Half-Elf*"}}};

const auto killPlayer3 = std::vector<TestLine>{
    TestLine{"You pierce *Gaer the " + dunadan + " Man*'s body extremely hard and shatter it.",
             std::nullopt},
    TestLine{"Your victim is shocked by your hit!", std::nullopt},
    TestLine{"You feel more experienced.", std::nullopt},
    TestLine{"Congratulations! This is the first time you've killed it!", std::nullopt},
    TestLine{"You gained some renown in this battle!", std::nullopt},
    TestLine{"You hear *Gaer the " + dunadan + " Man*'s death cry as he collapses.", std::nullopt},
    TestLine{"*Gaer the " + dunadan + " Man* has drawn his last breath! R.I.P.",
             LineParserResult{"*Gaer the " + dunadan + " Man*"}},
    TestLine{"A shadow slowly rises above the corpse of *Gaer the " + dunadan + " Man*.",
             std::nullopt}};
} // namespace TestLines

template<typename T>
void testParser(T &parser, const std::vector<TestLine> &testLines)
{
    for (const TestLine &tl : testLines) {
        const auto got = parser.parse(tl.line);
        static_assert(std::is_same_v<decltype(got), const LineParserResult>);
        if (got != tl.expected) {
            qInfo() << "while testing" << tl;
            qInfo() << "got" << got << "vs expected" << tl.expected;
            qFatal("invalid result");
        }
    }
}

template<auto parseFn>
struct NODISCARD OneLineMemoryParser final
{
private:
    QString m_prev = "";

public:
    NODISCARD LineParserResult parse(const QString &line)
    {
        using Callback = decltype(parseFn);
        static_assert(
            std::is_invocable_r_v<LineParserResult, Callback, const QString &, const QString &>);
        auto result = parseFn(m_prev, line);
        static_assert(std::is_same_v<decltype(result), LineParserResult>);
        m_prev = line;
        return result;
    }
};
} // namespace

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

void TestAdventure::testAchievementParser()
{
    OneLineMemoryParser<&AchievementParser::parse> parser;
    testParser(parser, TestLines::achievement1);
}

void TestAdventure::testHintParser()
{
    OneLineMemoryParser<&HintParser::parse> parser;
    testParser(parser, TestLines::hint1);
}

void TestAdventure::testKillAndXPParser()
{
    KillAndXPParser parser{};
    testParser(parser, TestLines::killMob1);
    testParser(parser, TestLines::killMob2);
    testParser(parser, TestLines::killMob3);
    testParser(parser, TestLines::killMob4);
    testParser(parser, TestLines::killPlayer1);
    testParser(parser, TestLines::killPlayer2);
    testParser(parser, TestLines::killPlayer3);
}

void TestAdventure::testE2E()
{
    GameObserver observer;
    AdventureTracker tracker{observer, nullptr};
    std::vector<QString> achievements;
    std::vector<QString> hints;
    std::vector<QString> killedMobs;

    connect(&tracker, &AdventureTracker::sig_achievedSomething, [&achievements](const QString &x) {
        achievements.push_back(x);
    });
    connect(&tracker, &AdventureTracker::sig_receivedHint, [&hints](const QString &x) {
        hints.push_back(x);
    });
    connect(&tracker, &AdventureTracker::sig_killedMob, [&killedMobs](const QString &x) {
        killedMobs.push_back(x);
    });

    auto pump = [&observer](const std::vector<TestLine> &lines) {
        mmqt::HideQDebug forThisFunction;
        for (const TestLine &tl : lines) {
            observer.slot_observeSentToUser(tl.line, true);
        }
    };

    pump(TestLines::achievement1);
    pump(TestLines::hint1);
    pump(TestLines::killMob1);
    pump(TestLines::killMob2);
    pump(TestLines::killMob3);
    pump(TestLines::killMob4);
    pump(TestLines::killPlayer1);
    pump(TestLines::killPlayer2);
    pump(TestLines::killPlayer3);

    QCOMPARE(achievements,
             std::vector<QString>{QString{
                 "You aided the hunter in the Tower Hills by cleaning out a rat infestation."}});

    QCOMPARE(hints, std::vector<QString>{QString{"Type unlock hatch to unlatch the hatch."}});

    const auto expectedMobs = std::vector<QString>{QString{"A husky smuggler"},
                                                   QString{"A wild bull (x)"},
                                                   QString{"A tree-snake"},
                                                   QString{"A spirit"},
                                                   QString{"*an Elf* (k)"},
                                                   QString{"*a Half-Elf*"},
                                                   QString{"*Gaer the " + dunadan + " Man*"}};
    QCOMPARE(killedMobs, expectedMobs);
}

QTEST_MAIN(TestAdventure)
