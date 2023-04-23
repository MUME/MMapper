// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "testclock.h"

#include <QDebug>
#include <QtTest/QtTest>

#include "../src/clock/mumeclock.h"
#include "../src/observer/gameobserver.h"
#include "../src/proxy/GmcpMessage.h"

TestClock::TestClock() = default;

TestClock::~TestClock() = default;

NODISCARD static QString testMumeStartEpochTime(MumeClock &clock, int64_t time)
{
    return clock.toMumeTime(clock.getMumeMoment(clock.getMumeStartEpoch() + time));
}

void TestClock::mumeClockTest()
{
    GameObserver observer;
    MumeClock clock(observer);
    clock.setPrecision(MumeClockPrecisionEnum::HOUR);

    QString zeroTime = "12am on Sunday, the 1st of Afteryule, year 2850 of the Third Age.";
    QString actualTime = testMumeStartEpochTime(clock, 0);
    QCOMPARE(actualTime, zeroTime);

    QString oneSecond = "12am on Sunday, the 1st of Afteryule, year 2850 of the Third Age.";
    actualTime = testMumeStartEpochTime(clock, 1);
    QCOMPARE(actualTime, oneSecond);

    QString oneTick = "1am on Sunday, the 1st of Afteryule, year 2850 of the Third Age.";
    actualTime = testMumeStartEpochTime(clock, 60);
    QCOMPARE(actualTime, oneTick);

    QString oneDay = "12am on Monday, the 2nd of Afteryule, year 2850 of the Third Age.";
    actualTime = testMumeStartEpochTime(clock, 60 * 24);
    QCOMPARE(actualTime, oneDay);

    QString twentyNineDays = "12am on Monday, the 30th of Afteryule, year 2850 of the Third Age.";
    actualTime = testMumeStartEpochTime(clock, 60 * 24 * 29);
    QCOMPARE(actualTime, twentyNineDays);

    QString oneMonth = "12am on Trewsday, the 1st of Solmath, year 2850 of the Third Age.";
    actualTime = testMumeStartEpochTime(clock, 60 * 24 * 30);
    QCOMPARE(actualTime, oneMonth);

    QString threeHundredFiftyNineDays
        = "12am on Trewsday, the 30th of Foreyule, year 2850 of the Third Age.";
    actualTime = testMumeStartEpochTime(clock, 60 * 24 * 359);
    QCOMPARE(actualTime, threeHundredFiftyNineDays);

    QString oneYear = "12am on Sunday, the 1st of Afteryule, year 2851 of the Third Age.";
    actualTime = testMumeStartEpochTime(clock, 60 * 24 * 30 * 12);
    QCOMPARE(actualTime, oneYear);
}

void TestClock::parseMumeTimeTest()
{
    GameObserver observer;
    MumeClock clock(observer);

    // Defaults to epoch time of zero
    QString expectedZeroEpoch = "Sunday, the 1st of Afteryule, year 2850 of the Third Age.";
    QCOMPARE(testMumeStartEpochTime(clock, 0), expectedZeroEpoch);

    // Real time is Wed Dec 20 07:03:27 2017 UTC.
    QString snapShot1 = "3pm on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    QString expected1 = snapShot1;
    clock.parseMumeTime(snapShot1);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expected1);

    // Real time is Wed Dec 20 07:18:02 2017 UTC.
    QString snapShot2 = "5am on Sterday, the 19th of Halimath, year 3030 of the Third Age.";
    QString expected2 = snapShot2;
    clock.parseMumeTime(snapShot2);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expected2);

    // Real time is Wed Dec 20 07:38:44 2017 UTC.
    QString snapShot3 = "2am on Sunday, the 20th of Halimath, year 3030 of the Third Age.";
    QString expected3 = snapShot3;
    clock.parseMumeTime(snapShot3);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expected3);

    // Real time is Thu Dec 21 05:27:35 2017 UTC.
    QString snapShot4 = "3pm on Highday, the 14th of Blotmath, year 3030 of the Third Age.";
    QString expected4 = snapShot4;
    clock.parseMumeTime(snapShot4);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expected4);

    // Sindarin Calendar
    QString sindarin = "3pm on Oraearon, the 14th of Hithui, year 3030 of the Third Age.";
    QString expectedSindarin = expected4;
    clock.parseMumeTime(sindarin);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedSindarin);

    // Real time is Sat Mar  2 20:43:30 2019 UTC.
    QString snapShot5 = "6pm on Mersday, the 22nd of Winterfilth, year 2915 of the Third Age.";
    QString expected5 = snapShot5;
    clock.parseMumeTime(snapShot5);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expected5);

    // Real time is Thu Mar  7 06:28:11 2019 UTC.
    QString snapShot6 = "2am on Sunday, the 17th of Afterlithe, year 2916 of the Third Age.";
    QString expected6 = snapShot6;
    clock.parseMumeTime(snapShot6);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expected6);
}

void TestClock::parseWeatherClockSkewTest()
{
    GameObserver observer;
    MumeClock clock(observer);

    const QString snapShot1 = "3pm on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    // Real time is Wed Dec 20 07:03:27 2017 UTC.
    auto realTime1 = 1513753407;
    clock.parseMumeTime(snapShot1, realTime1);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1)), snapShot1);

    // First sync
    auto timeOccuredAt = 1;
    clock.parseWeather(MumeTimeEnum::DAWN, realTime1 + timeOccuredAt);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1 + timeOccuredAt)),
             "5:00am on Highday, the 18th of Halimath, year 3030 of the Third Age.");

    // Mume running fast (but "day" event type synchronizes)
    timeOccuredAt = 1 + 58;
    clock.parseWeather(MumeTimeEnum::DAY, realTime1 + timeOccuredAt);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1 + timeOccuredAt)),
             "6:00am on Highday, the 18th of Halimath, year 3030 of the Third Age.");

    // Mume running on time
    timeOccuredAt = 1 + 60 + 58;
    clock.parseWeather(MumeTimeEnum::UNKNOWN, realTime1 + timeOccuredAt);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1 + timeOccuredAt)),
             "7:00am on Highday, the 18th of Halimath, year 3030 of the Third Age.");

    // Mume running on time
    timeOccuredAt = 1 + 60 + 58 + 60;
    clock.parseWeather(MumeTimeEnum::UNKNOWN, realTime1 + timeOccuredAt);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1 + timeOccuredAt)),
             "8:00am on Highday, the 18th of Halimath, year 3030 of the Third Age.");

    // Mume running slow
    timeOccuredAt = 1 + 60 + 58 + 60 + 65;
    clock.parseWeather(MumeTimeEnum::UNKNOWN, realTime1 + timeOccuredAt);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1 + timeOccuredAt)),
             "9:00am on Highday, the 18th of Halimath, year 3030 of the Third Age.");
}

void TestClock::parseWeatherTest()
{
    GameObserver observer;
    MumeClock clock(observer);

    QString snapShot1 = "3pm on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    QString expectedTime = snapShot1;
    clock.parseMumeTime(snapShot1);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    expectedTime = "5:00am on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.slot_onUserGmcp(GmcpMessage::fromRawBytes(R"(Event.Sun {"what":"rise"})"));
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    expectedTime = "6:00am on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.slot_onUserGmcp(GmcpMessage::fromRawBytes(R"(Event.Sun {"what":"light"})"));
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    expectedTime = "9:00pm on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.slot_onUserGmcp(GmcpMessage::fromRawBytes(R"(Event.Sun {"what":"set"})"));
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    expectedTime = "10:00pm on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.slot_onUserGmcp(GmcpMessage::fromRawBytes(R"(Event.Sun {"what":"dark"})"));
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    clock.slot_onUserGmcp(GmcpMessage::fromRawBytes(R"(Event.Darkness {"what":"start"})"));
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    clock.slot_onUserGmcp(GmcpMessage::fromRawBytes(R"(Event.Moon {"what":"rise"})"));
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);
}

void TestClock::parseClockTimeTest()
{
    GameObserver observer;
    MumeClock clock(observer);

    // Clock set to coarse
    // Real time is Wed Dec 20 07:03:27 2017 UTC.
    const QString snapShot1 = "3pm on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.parseMumeTime(snapShot1);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), snapShot1);

    // Afternoon
    clock.parseClockTime("The current time is 12:34pm.");
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()),
             "12:34pm on Highday, the 18th of Halimath, year 3030 of the Third Age.");

    // Midnight
    clock.parseClockTime("The current time is 12:51am.");
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()),
             "12:51am on Highday, the 18th of Halimath, year 3030 of the Third Age.");
}

void TestClock::precsionTimeoutTest()
{
    GameObserver observer;
    MumeClock clock(observer);

    QCOMPARE(clock.getPrecision(), MumeClockPrecisionEnum::UNSET);

    clock.setPrecision(MumeClockPrecisionEnum::DAY);
    QCOMPARE(clock.getPrecision(), MumeClockPrecisionEnum::DAY);

    clock.setPrecision(MumeClockPrecisionEnum::HOUR);
    QCOMPARE(clock.getPrecision(), MumeClockPrecisionEnum::DAY);

    clock.setPrecision(MumeClockPrecisionEnum::MINUTE);
    QCOMPARE(clock.getPrecision(), MumeClockPrecisionEnum::DAY);
}

void TestClock::moonClockTest()
{
    GameObserver observer;
    MumeClock clock(observer);

    auto moment = clock.getMumeMoment(clock.getMumeStartEpoch());
    QCOMPARE(moment.toMumeMoonTime(), "You can see a full moon to the south.");
    QCOMPARE(moment.moonZenithMinutes(), 0);
    QCOMPARE(moment.moonLevel(), 12);
    QCOMPARE(static_cast<int>(moment.moonPosition()), static_cast<int>(MumeMoonPositionEnum::SOUTH));
    QCOMPARE(static_cast<int>(moment.moonPhase()), static_cast<int>(MumeMoonPhaseEnum::FULL_MOON));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::BRIGHT));
    QCOMPARE(moment.toMoonVisibilityCountDown(), "6:00");

    moment = clock.getMumeMoment(clock.getMumeStartEpoch() + 4 * MUME_MINUTES_PER_HOUR);
    QCOMPARE(moment.toMumeMoonTime(), "You can see a full moon to the southwest.");
    QCOMPARE(moment.moonZenithMinutes(), 8);
    QCOMPARE(moment.moonLevel(), 12);
    QCOMPARE(static_cast<int>(moment.moonPosition()),
             static_cast<int>(MumeMoonPositionEnum::SOUTHWEST));
    QCOMPARE(static_cast<int>(moment.moonPhase()), static_cast<int>(MumeMoonPhaseEnum::FULL_MOON));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::BRIGHT));
    QCOMPARE(moment.toMoonVisibilityCountDown(), "2:08");

    moment = clock.getMumeMoment(clock.getMumeStartEpoch() + 6 * MUME_MINUTES_PER_HOUR);
    QCOMPARE(moment.toMumeMoonTime(), "You can see a full moon to the west.");
    QCOMPARE(moment.moonZenithMinutes(), 12);
    QCOMPARE(moment.moonLevel(), 12);
    QCOMPARE(static_cast<int>(moment.moonPosition()), static_cast<int>(MumeMoonPositionEnum::WEST));
    QCOMPARE(static_cast<int>(moment.moonPhase()), static_cast<int>(MumeMoonPhaseEnum::FULL_MOON));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::BRIGHT));
    QCOMPARE(moment.toMoonVisibilityCountDown(), "0:12");
    // REVISIT: The moon should have set according to the first test but we're at 0:12 instead

    moment = clock.getMumeMoment(clock.getMumeStartEpoch() + 10 * MUME_MINUTES_PER_HOUR);
    QCOMPARE(moment.toMumeMoonTime(), "The full moon is below the horizon.");
    QCOMPARE(moment.moonZenithMinutes(), 20);
    QCOMPARE(moment.moonLevel(), 12);
    QCOMPARE(static_cast<int>(moment.moonPosition()),
             static_cast<int>(MumeMoonPositionEnum::INVISIBLE));
    QCOMPARE(static_cast<int>(moment.moonPhase()), static_cast<int>(MumeMoonPhaseEnum::FULL_MOON));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::INVISIBLE));
    QCOMPARE(moment.toMoonVisibilityCountDown(), "9:10");

    moment = clock.getMumeMoment(clock.getMumeStartEpoch() + 20 * MUME_MINUTES_PER_HOUR);
    QCOMPARE(moment.toMumeMoonTime(), "You can see a waning three-quarter moon to the east.");
    QCOMPARE(moment.moonZenithMinutes(), 40);
    QCOMPARE(moment.moonLevel(), 11);
    QCOMPARE(static_cast<int>(moment.moonPosition()), static_cast<int>(MumeMoonPositionEnum::EAST));
    QCOMPARE(static_cast<int>(moment.moonPhase()),
             static_cast<int>(MumeMoonPhaseEnum::WANING_GIBBOUS));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::BRIGHT));
    QCOMPARE(moment.toMoonVisibilityCountDown(), "10:40");

    moment = clock.getMumeMoment(clock.getMumeStartEpoch() + MUME_MINUTES_PER_MOON_CYCLE / 2);
    QCOMPARE(moment.toMumeMoonTime(), "The new moon is below the horizon.");
    QCOMPARE(moment.moonZenithMinutes(), 720);
    QCOMPARE(moment.moonLevel(), 0);
    QCOMPARE(static_cast<int>(moment.moonPosition()),
             static_cast<int>(MumeMoonPositionEnum::INVISIBLE));
    QCOMPARE(static_cast<int>(moment.moonPhase()), static_cast<int>(MumeMoonPhaseEnum::NEW_MOON));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::INVISIBLE));
    QCOMPARE(moment.toMoonVisibilityCountDown(), "12:28");

    moment = clock.getMumeMoment(clock.getMumeStartEpoch() + MUME_MINUTES_PER_MOON_CYCLE / 2
                                 + 14 * MUME_MINUTES_PER_HOUR);
    QCOMPARE(moment.toMumeMoonTime(), "You can not see a new moon to the southeast.");
    QCOMPARE(moment.moonZenithMinutes(), 748);
    QCOMPARE(moment.moonLevel(), 0);
    QCOMPARE(static_cast<int>(moment.moonPosition()),
             static_cast<int>(MumeMoonPositionEnum::SOUTHEAST));
    QCOMPARE(static_cast<int>(moment.moonPhase()), static_cast<int>(MumeMoonPhaseEnum::NEW_MOON));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::INVISIBLE));
    QCOMPARE(moment.toMoonVisibilityCountDown(), "10:06");

    moment = clock.getMumeMoment(clock.getMumeStartEpoch() + MUME_MINUTES_PER_MOON_CYCLE / 2
                                 + MUME_MINUTES_PER_MOON_PHASE);
    QCOMPARE(moment.moonZenithMinutes(), 899);
    QCOMPARE(moment.moonLevel(), 3);
    QCOMPARE(moment.toMumeMoonTime(), "You can not see a waxing quarter moon to the southeast.");
    QCOMPARE(static_cast<int>(moment.moonPosition()),
             static_cast<int>(MumeMoonPositionEnum::SOUTHEAST));
    QCOMPARE(static_cast<int>(moment.moonPhase()),
             static_cast<int>(MumeMoonPhaseEnum::WAXING_CRESCENT));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::INVISIBLE));
    QCOMPARE(moment.toMoonVisibilityCountDown(), "10:02");

    clock.parseMumeTime("2:00 am on Sunday, the 19th of Forelithe, year 2997 of the Third Age.");
    moment = clock.getMumeMoment();
    QCOMPARE(moment.toMumeMoonTime(), "The waxing half moon is below the horizon.");
    QCOMPARE(moment.moonLevel(), 6);
    QCOMPARE(static_cast<int>(moment.moonPosition()),
             static_cast<int>(MumeMoonPositionEnum::INVISIBLE));
    QCOMPARE(static_cast<int>(moment.moonPhase()),
             static_cast<int>(MumeMoonPhaseEnum::FIRST_QUARTER));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::INVISIBLE));

    clock.parseMumeTime("10:00 pm on Sunday, the 30th of Astron, year 2995 of the Third Age.");
    moment = clock.getMumeMoment();
    QCOMPARE(moment.toMumeMoonTime(), "You can see a waxing quarter moon to the west.");
    QCOMPARE(moment.moonLevel(), 5);
    QCOMPARE(static_cast<int>(moment.moonPosition()), static_cast<int>(MumeMoonPositionEnum::WEST));
    QCOMPARE(static_cast<int>(moment.moonPhase()),
             static_cast<int>(MumeMoonPhaseEnum::WAXING_CRESCENT));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::BRIGHT));

    clock.parseMumeTime("1:00 am on Sterday, the 15th of Astron, year 2995 of the Third Age.");
    moment = clock.getMumeMoment();
    QCOMPARE(moment.toMumeMoonTime(), "You can see a waning half moon to the southeast.");
    QCOMPARE(moment.moonLevel(), 8);
    QCOMPARE(static_cast<int>(moment.moonPosition()),
             static_cast<int>(MumeMoonPositionEnum::SOUTHEAST));
    QCOMPARE(static_cast<int>(moment.moonPhase()),
             static_cast<int>(MumeMoonPhaseEnum::THIRD_QUARTER));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::BRIGHT));

    clock.parseMumeTime("4:00 am on Sterday, the 15th of Astron, year 2995 of the Third Age.");
    moment = clock.getMumeMoment();
    QCOMPARE(moment.toMumeMoonTime(), "You can see a waning half moon to the south.");
    QCOMPARE(moment.moonLevel(), 7);
    QCOMPARE(static_cast<int>(moment.moonPosition()), static_cast<int>(MumeMoonPositionEnum::SOUTH));
    QCOMPARE(static_cast<int>(moment.moonPhase()),
             static_cast<int>(MumeMoonPhaseEnum::THIRD_QUARTER));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::BRIGHT));

    clock.parseMumeTime("7:00 am on Sterday, the 15th of Astron, year 2995 of the Third Age.");
    moment = clock.getMumeMoment();
    QCOMPARE(moment.toMumeMoonTime(), "You can see a waning half moon to the southwest.");
    QCOMPARE(moment.moonLevel(), 7);
    QCOMPARE(static_cast<int>(moment.moonPosition()),
             static_cast<int>(MumeMoonPositionEnum::SOUTHWEST));
    QCOMPARE(static_cast<int>(moment.moonPhase()),
             static_cast<int>(MumeMoonPhaseEnum::THIRD_QUARTER));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::BRIGHT));

    clock.parseMumeTime("10:00 pm on Monday, the 20th of Forelithe, year 2997 of the Third Age.");
    moment = clock.getMumeMoment();
    QCOMPARE(moment.toMumeMoonTime(), "You can see a waxing half moon to the southwest.");
    QCOMPARE(moment.moonLevel(), 7);
    QCOMPARE(static_cast<int>(moment.moonPosition()),
             static_cast<int>(MumeMoonPositionEnum::SOUTHWEST));
    QCOMPARE(static_cast<int>(moment.moonPhase()),
             static_cast<int>(MumeMoonPhaseEnum::FIRST_QUARTER));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::BRIGHT));
}

QTEST_MAIN(TestClock)
