// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "testclock.h"

#include "../src/clock/mumeclock.h"
#include "../src/global/HideQDebug.h"
#include "../src/observer/gameobserver.h"
#include "../src/proxy/GmcpMessage.h"

#include <QDebug>
#include <QtTest/QtTest>

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

void TestClock::getMumeMonthTest()
{
    {
        using E = MumeClock::WestronMonthNamesEnum;
        static_assert(std::is_same_v<int8_t, std::underlying_type_t<E>>);

#define X_CASE(x) \
    static_assert(static_cast<int8_t>(E::x) >= 0); \
    QCOMPARE(MumeClock::getMumeMonth(#x), static_cast<int8_t>(E::x));
        XFOREACH_WestronMonthNamesEnum(X_CASE)
#undef X_CASE
    }

    {
        using E = MumeClock::SindarinMonthNamesEnum;
        static_assert(std::is_same_v<int8_t, std::underlying_type_t<E>>);
#define X_CASE(x) \
    static_assert(static_cast<int8_t>(E::x) >= 0); \
    QCOMPARE(MumeClock::getMumeMonth(#x), static_cast<int8_t>(E::x));
        XFOREACH_SindarinMonthNamesEnum(X_CASE)
#undef X_CASE
    }

    {
        using E = MumeClock::WestronWeekDayNamesEnum;
        static_assert(std::is_same_v<int8_t, std::underlying_type_t<E>>);
#define X_CASE(x) \
    static_assert(static_cast<int8_t>(E::x) >= 0); \
    QCOMPARE(MumeClock::getMumeWeekday(#x), static_cast<int8_t>(E::x));
        XFOREACH_WestronWeekDayNamesEnum(X_CASE)
#undef X_CASE
    }

    {
        using E = MumeClock::SindarinWeekDayNamesEnum;
        static_assert(std::is_same_v<int8_t, std::underlying_type_t<E>>);
#define X_CASE(x) \
    static_assert(static_cast<int8_t>(E::x) >= 0); \
    QCOMPARE(MumeClock::getMumeWeekday(#x), static_cast<int8_t>(E::x));
        XFOREACH_SindarinWeekDayNamesEnum(X_CASE)
#undef X_CASE
    }

    // demonstrate that it no longer uses keyToValue()
    QCOMPARE(MumeClock::getMumeMonth("Narwain"), 0);
    QCOMPARE(MumeClock::getMumeMonth("Ninui"), 1);
    QCOMPARE(MumeClock::getMumeMonth("Narwain|Ninui"), -1);
}

void TestClock::parseWeatherClockSkewTest()
{
    GameObserver observer;
    MumeClock clock(observer);

    const QString snapShot1 = "3pm on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    // Real time is Wed Dec 20 07:03:27 2017 UTC.
    auto realTime1 = 1513753407;
    {
        // expect 3 warnings about out-of bounds values:
        // WARNING: soft assertion failure: month(-1) is not in the half-open interval '[0..12)'
        // WARNING: soft assertion failure: day(-12) is not in the half-open interval '[0..30)'
        // WARNING: soft assertion failure: hour(-8) is not in the half-open interval '[0..24)'

#if 0 // Enable this to hide the warnings...
        mmqt::HideQDebug forThisFnCall{mmqt::HideQDebugOptions{true, true, true}};
#endif
        clock.parseMumeTime(snapShot1, realTime1);
    }
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
    clock.onUserGmcp(GmcpMessage::fromRawBytes(R"(Event.Sun {"what":"rise"})"));
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    expectedTime = "6:00am on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.onUserGmcp(GmcpMessage::fromRawBytes(R"(Event.Sun {"what":"light"})"));
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    expectedTime = "9:00pm on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.onUserGmcp(GmcpMessage::fromRawBytes(R"(Event.Sun {"what":"set"})"));
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    expectedTime = "10:00pm on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.onUserGmcp(GmcpMessage::fromRawBytes(R"(Event.Sun {"what":"dark"})"));
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    clock.onUserGmcp(GmcpMessage::fromRawBytes(R"(Event.Darkness {"what":"start"})"));
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    clock.onUserGmcp(GmcpMessage::fromRawBytes(R"(Event.Moon {"what":"rise"})"));
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
    QCOMPARE(moment.untilMoonPosition(MumeMoonPositionEnum::INVISIBLE), 372);
    QCOMPARE(moment.untilMoonPosition(MumeMoonPositionEnum::EAST), 1118);
    QCOMPARE(moment.toMoonVisibilityCountDown(), "6:12");

    moment = clock.getMumeMoment(clock.getMumeStartEpoch() + 4 * MUME_MINUTES_PER_HOUR);
    QCOMPARE(moment.toMumeMoonTime(), "You can see a full moon to the southwest.");
    QCOMPARE(moment.moonZenithMinutes(), 8);
    QCOMPARE(moment.moonLevel(), 12);
    QCOMPARE(static_cast<int>(moment.moonPosition()),
             static_cast<int>(MumeMoonPositionEnum::SOUTHWEST));
    QCOMPARE(static_cast<int>(moment.moonPhase()), static_cast<int>(MumeMoonPhaseEnum::FULL_MOON));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::BRIGHT));
    QCOMPARE(moment.untilMoonPosition(MumeMoonPositionEnum::INVISIBLE), 132);
    QCOMPARE(moment.untilMoonPosition(MumeMoonPositionEnum::EAST), 878);
    QCOMPARE(moment.toMoonVisibilityCountDown(), "2:12");

    moment = clock.getMumeMoment(clock.getMumeStartEpoch() + 6 * MUME_MINUTES_PER_HOUR);
    QCOMPARE(moment.toMumeMoonTime(), "You can see a full moon to the west.");
    QCOMPARE(moment.moonZenithMinutes(), 12);
    QCOMPARE(moment.moonLevel(), 12);
    QCOMPARE(static_cast<int>(moment.moonPosition()), static_cast<int>(MumeMoonPositionEnum::WEST));
    QCOMPARE(static_cast<int>(moment.moonPhase()), static_cast<int>(MumeMoonPhaseEnum::FULL_MOON));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::BRIGHT));
    QCOMPARE(moment.untilMoonPosition(MumeMoonPositionEnum::INVISIBLE), 12);
    QCOMPARE(moment.untilMoonPosition(MumeMoonPositionEnum::EAST), 758);
    QCOMPARE(moment.toMoonVisibilityCountDown(), "0:12");

    moment = clock.getMumeMoment(clock.getMumeStartEpoch() + 7 * MUME_MINUTES_PER_HOUR);
    QCOMPARE(moment.toMumeMoonTime(), "The full moon is below the horizon.");
    QCOMPARE(moment.moonZenithMinutes(), 14);
    QCOMPARE(moment.moonLevel(), 12);
    QCOMPARE(static_cast<int>(moment.moonPosition()),
             static_cast<int>(MumeMoonPositionEnum::INVISIBLE));
    QCOMPARE(static_cast<int>(moment.moonPhase()), static_cast<int>(MumeMoonPhaseEnum::FULL_MOON));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::INVISIBLE));
    QCOMPARE(moment.untilMoonPosition(MumeMoonPositionEnum::INVISIBLE), 1443);
    QCOMPARE(moment.untilMoonPosition(MumeMoonPositionEnum::EAST), 698);
    QCOMPARE(moment.toMoonVisibilityCountDown(), "11:38");

    moment = clock.getMumeMoment(clock.getMumeStartEpoch() + 10 * MUME_MINUTES_PER_HOUR);
    QCOMPARE(moment.toMumeMoonTime(), "The full moon is below the horizon.");
    QCOMPARE(moment.moonZenithMinutes(), 20);
    QCOMPARE(moment.moonLevel(), 12);
    QCOMPARE(static_cast<int>(moment.moonPosition()),
             static_cast<int>(MumeMoonPositionEnum::INVISIBLE));
    QCOMPARE(static_cast<int>(moment.moonPhase()), static_cast<int>(MumeMoonPhaseEnum::FULL_MOON));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::INVISIBLE));
    QCOMPARE(moment.untilMoonPosition(MumeMoonPositionEnum::INVISIBLE), 1263);
    QCOMPARE(moment.untilMoonPosition(MumeMoonPositionEnum::EAST), 518);
    QCOMPARE(moment.toMoonVisibilityCountDown(), "8:38");

    moment = clock.getMumeMoment(clock.getMumeStartEpoch() + 11 * MUME_MINUTES_PER_HOUR);
    QCOMPARE(moment.toMumeMoonTime(), "The full moon is below the horizon.");
    QCOMPARE(moment.moonZenithMinutes(), 22);
    QCOMPARE(moment.moonLevel(), 12);
    QCOMPARE(static_cast<int>(moment.moonPosition()),
             static_cast<int>(MumeMoonPositionEnum::INVISIBLE));
    QCOMPARE(static_cast<int>(moment.moonPhase()), static_cast<int>(MumeMoonPhaseEnum::FULL_MOON));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::INVISIBLE));
    QCOMPARE(moment.untilMoonPosition(MumeMoonPositionEnum::INVISIBLE), 1203);
    QCOMPARE(moment.untilMoonPosition(MumeMoonPositionEnum::EAST), 458);
    QCOMPARE(moment.toMoonVisibilityCountDown(), "7:38");

    moment = clock.getMumeMoment(clock.getMumeStartEpoch() + 20 * MUME_MINUTES_PER_HOUR);
    QCOMPARE(moment.toMumeMoonTime(), "You can see a waning three-quarter moon to the east.");
    QCOMPARE(moment.moonZenithMinutes(), 40);
    QCOMPARE(moment.moonLevel(), 11);
    QCOMPARE(static_cast<int>(moment.moonPosition()), static_cast<int>(MumeMoonPositionEnum::EAST));
    QCOMPARE(static_cast<int>(moment.moonPhase()),
             static_cast<int>(MumeMoonPhaseEnum::WANING_GIBBOUS));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::BRIGHT));
    QCOMPARE(moment.untilMoonPosition(MumeMoonPositionEnum::INVISIBLE), 663);
    QCOMPARE(moment.untilMoonPosition(MumeMoonPositionEnum::EAST), 1408);
    QCOMPARE(moment.toMoonVisibilityCountDown(), "11:03");

    moment = clock.getMumeMoment(clock.getMumeStartEpoch() + MUME_MINUTES_PER_MOON_CYCLE / 2);
    QCOMPARE(moment.toMumeMoonTime(), "The new moon is below the horizon.");
    QCOMPARE(moment.moonZenithMinutes(), 720);
    QCOMPARE(moment.moonLevel(), 0);
    QCOMPARE(static_cast<int>(moment.moonPosition()),
             static_cast<int>(MumeMoonPositionEnum::INVISIBLE));
    QCOMPARE(static_cast<int>(moment.moonPhase()), static_cast<int>(MumeMoonPhaseEnum::NEW_MOON));
    QCOMPARE(static_cast<int>(moment.moonVisibility()),
             static_cast<int>(MumeMoonVisibilityEnum::INVISIBLE));
    QCOMPARE(moment.untilMoonPhase(MumeMoonPhaseEnum::WAXING_CRESCENT), 4429);
    QCOMPARE(moment.untilMoonPhase(MumeMoonPhaseEnum::FULL_MOON), 20376);
    QCOMPARE(moment.toMoonVisibilityCountDown(), "1:13:49");

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
    QCOMPARE(moment.untilMoonPhase(MumeMoonPhaseEnum::WAXING_CRESCENT),
             4429 - 14 * MUME_MINUTES_PER_HOUR);
    QCOMPARE(moment.untilMoonPhase(MumeMoonPhaseEnum::FULL_MOON),
             20376 - 14 * MUME_MINUTES_PER_HOUR);
    QCOMPARE(moment.toMoonVisibilityCountDown(), "59:49");

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
    QCOMPARE(moment.untilMoonPhase(MumeMoonPhaseEnum::WAXING_CRESCENT),
             4429 - MUME_MINUTES_PER_MOON_PHASE + MUME_MINUTES_PER_MOON_CYCLE);
    QCOMPARE(moment.untilMoonPhase(MumeMoonPhaseEnum::FULL_MOON),
             20376 - MUME_MINUTES_PER_MOON_PHASE);
    QCOMPARE(moment.toMoonVisibilityCountDown(), "10:24");

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
