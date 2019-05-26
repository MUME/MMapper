#include "testclock.h"
#include "mumeclock.h"
#include <QDebug>
#include <QtTest/QtTest>

TestClock::TestClock() = default;

TestClock::~TestClock() = default;

static QString testMumeStartEpochTime(MumeClock &clock, int64_t time)
{
    return clock.toMumeTime(clock.getMumeMoment(clock.getMumeStartEpoch() + time));
}

void TestClock::mumeClockTest()
{
    MumeClock clock;
    clock.setPrecision(MumeClockPrecision::MUMECLOCK_HOUR);

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
    MumeClock clock;

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
    MumeClock clock;

    QString snapShot1 = "3pm on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    QString expected1 = snapShot1;
    // Real time is Wed Dec 20 07:03:27 2017 UTC.
    int realTime1 = 1513753407;
    clock.parseMumeTime(snapShot1, realTime1);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1)), expected1);

    // First sync
    int timeOccuredAt = 1;
    QString expectedTime = "6:00am on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.parseWeather("The day has begun.", realTime1 + timeOccuredAt);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1 + timeOccuredAt)), expectedTime);

    // Mume running on time
    timeOccuredAt = 1 + 60;
    expectedTime = "7:00am on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.parseWeather("The evil power begins to regress...", realTime1 + timeOccuredAt);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1 + timeOccuredAt)), expectedTime);

    // Mume running fast
    timeOccuredAt = 1 + 60 + 58;
    expectedTime = "8:00am on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.parseWeather("The evil power begins to regress...", realTime1 + timeOccuredAt);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1 + timeOccuredAt)), expectedTime);

    // Mume running on time
    timeOccuredAt = 1 + 60 + 58 + 60;
    expectedTime = "9:00am on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.parseWeather("The evil power begins to regress...", realTime1 + timeOccuredAt);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1 + timeOccuredAt)), expectedTime);

    // Mume running slow
    timeOccuredAt = 1 + 60 + 58 + 60 + 65;
    expectedTime = "10:00am on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.parseWeather("The evil power begins to regress...", realTime1 + timeOccuredAt);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1 + timeOccuredAt)), expectedTime);
}

void TestClock::parseWeatherTest()
{
    MumeClock clock;

    QString snapShot1 = "3pm on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    QString expectedTime = snapShot1;
    clock.parseMumeTime(snapShot1);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    expectedTime = "5:00am on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.parseWeather("Light gradually filters in, proclaiming a new sunrise outside.");
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    expectedTime = "6:00am on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.parseWeather("It seems as if the day has begun.");
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    expectedTime = "9:00pm on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.parseWeather("The deepening gloom announces another sunset outside.");
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    expectedTime = "10:00pm on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.parseWeather("It seems as if the night has begun.");
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);
}

void TestClock::parseClockTimeTest()
{
    MumeClock clock;

    // Clock set to coarse
    // Real time is Wed Dec 20 07:03:27 2017 UTC.
    QString snapShot1 = "3pm on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    QString expected1 = snapShot1;
    clock.parseMumeTime(snapShot1);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expected1);

    QString afternoon = "12:34pm on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.parseClockTime("The current time is 12:34pm.");
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), afternoon);

    QString midnight = "12:51am on Highday, the 18th of Halimath, year 3030 of the Third Age.";
    clock.parseClockTime("The current time is 12:51am.");
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), midnight);
}

void TestClock::precsionTimeoutTest()
{
    MumeClock clock;
    QCOMPARE(clock.getPrecision(), MumeClockPrecision::MUMECLOCK_UNSET);

    clock.setPrecision(MumeClockPrecision::MUMECLOCK_DAY);
    QCOMPARE(clock.getPrecision(), MumeClockPrecision::MUMECLOCK_DAY);

    clock.setPrecision(MumeClockPrecision::MUMECLOCK_HOUR);
    QCOMPARE(clock.getPrecision(), MumeClockPrecision::MUMECLOCK_DAY);

    clock.setPrecision(MumeClockPrecision::MUMECLOCK_MINUTE);
    QCOMPARE(clock.getPrecision(), MumeClockPrecision::MUMECLOCK_DAY);
}

void TestClock::moonClockTest()
{
    MumeClock clock;
    QString snapShot1 = "7pm on Hevensday, the 21st of Winterfilth, year 2929 of the Third Age.";
    clock.parseMumeTime(snapShot1);
    auto moment = clock.getMumeMoment();
    QCOMPARE(moment.moonLevel(), 2);
    QCOMPARE(moment.moonPosition(), 11);
    QCOMPARE(moment.moonRise(), 8);
    QCOMPARE(moment.moonCycle(), 2);
    QString mumeMoonStr1 = "You can see a New Moon (waxing) in the western part of the sky.";
    QCOMPARE(clock.getMumeMoment().toMumeMoonTime(), mumeMoonStr1);

    QString snapShot2 = "11am on Sterday, the 24th of Winterfilth, year 2929 of the Third Age.";
    clock.parseMumeTime(snapShot2);
    moment = clock.getMumeMoment();
    QCOMPARE(moment.moonLevel(), 5);
    QCOMPARE(moment.moonPosition(), 0);
    QCOMPARE(moment.moonRise(), 11);
    QCOMPARE(moment.moonCycle(), 5);
    QString mumeMoonStr2 = "You can see a Quarter Moon (waxing) in the eastern part of the sky.";
    QCOMPARE(clock.getMumeMoment().toMumeMoonTime(), mumeMoonStr2);

    QString snapShot3 = "4pm on Sterday, the 24th of Winterfilth, year 2929 of the Third Age.";
    clock.parseMumeTime(snapShot3);
    moment = clock.getMumeMoment();
    QCOMPARE(moment.moonRise(), 11);
    QCOMPARE(moment.moonLevel(), 5);
    QCOMPARE(moment.moonPosition(), 5);
    QString mumeMoonStr3 = "You can see a Quarter Moon (waxing) in the southern part of the sky.";
    QCOMPARE(clock.getMumeMoment().toMumeMoonTime(), mumeMoonStr3);

    QString snapShot4 = "7pm on Sterday, the 24th of Winterfilth, year 2929 of the Third Age.";
    clock.parseMumeTime(snapShot4);
    moment = clock.getMumeMoment();
    QCOMPARE(moment.moonRise(), 11);
    QCOMPARE(moment.moonLevel(), 5);
    QCOMPARE(moment.moonPosition(), 8);
    QString mumeMoonStr4 = "You can see a Quarter Moon (waxing) in the western part of the sky.";
    QCOMPARE(clock.getMumeMoment().toMumeMoonTime(), mumeMoonStr4);

    QString snapShot5 = "12pm on Sunday, the 25th of Winterfilth, year 2929 of the Third Age.";
    clock.parseMumeTime(snapShot5);
    moment = clock.getMumeMoment();
    QCOMPARE(moment.moonRise(), 12);
    QCOMPARE(moment.moonLevel(), 6);
    QCOMPARE(moment.moonPosition(), 0);
    QCOMPARE(moment.moonCycle(), 6);
    QString mumeMoonStr5 = "You can see a Half Moon (waxing) in the eastern part of the sky.";
    QCOMPARE(clock.getMumeMoment().toMumeMoonTime(), mumeMoonStr5);

    QString snapShot6 = "12am on Hevensday, the 28th of Winterfilth, year 2929 of the Third Age.";
    clock.parseMumeTime(snapShot6);
    moment = clock.getMumeMoment();
    QCOMPARE(moment.moonRise(), 15);
    QCOMPARE(moment.moonLevel(), 9);
    QCOMPARE(moment.moonPosition(), 9);
    QCOMPARE(moment.moonCycle(), 9);
    QString mumeMoonStr6
        = "You can see a Three-Quarter Moon (waxing) in the western part of the sky.";
    QCOMPARE(clock.getMumeMoment().toMumeMoonTime(), mumeMoonStr6);

    QString snapShot7 = "1am on Sterday, the 1st of Blotmath, year 2929 of the Third Age.";
    clock.parseMumeTime(snapShot7);
    moment = clock.getMumeMoment();
    QCOMPARE(moment.moonRise(), 18);
    QCOMPARE(moment.moonLevel(), 12);
    QCOMPARE(moment.moonPosition(), 7);
    QCOMPARE(moment.moonCycle(), 12);
    QString mumeMoonStr7 = "You can see a Full Moon (waning) in the southern part of the sky.";
    QCOMPARE(clock.getMumeMoment().toMumeMoonTime(), mumeMoonStr7);

    QString snapShot8 = "1am on Sterday, the 8th of Blotmath, year 2929 of the Third Age.";
    clock.parseMumeTime(snapShot8);
    moment = clock.getMumeMoment();
    QCOMPARE(moment.moonRise(), 1);
    QCOMPARE(moment.moonLevel(), 5);
    QCOMPARE(moment.moonPosition(), 0);
    QCOMPARE(moment.moonCycle(), 19);
    QString mumeMoonStr8 = "You can see a Quarter Moon (waning) in the eastern part of the sky.";
    QCOMPARE(clock.getMumeMoment().toMumeMoonTime(), mumeMoonStr8);
}

QTEST_MAIN(TestClock)
