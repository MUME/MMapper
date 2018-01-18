#include <QtTest/QtTest>
#include <QDebug>

#include "testclock.h"
#include "mumeclock.h"

using namespace std;

TestClock::TestClock()
    : QObject()
{}

TestClock::~TestClock()
{
}

int convertMumeRealTime(const QString &realTime)
{
    // Real time is Wed Dec 20 07:03:27 2017 UTC.
    QString dateString = realTime.mid(13, 24);
    QDateTime parsedTime = QDateTime::fromString(dateString, "ddd MMM dd hh:mm:ss yyyy");
    parsedTime.setTimeSpec(Qt::UTC);
    return parsedTime.toTime_t();
}

QString testMumeStartEpochTime(MumeClock &clock, int time)
{
    return clock.toMumeTime(clock.getMumeMoment(clock.getMumeStartEpoch() + time));
}

void TestClock::mumeClockTest()
{
    MumeClock clock;
    clock.setPrecision(MUMECLOCK_HOUR);

    QString zeroTime = "12am on the 1st of Afteryule, Year 2850 of the Third Age.";
    QString actualTime = testMumeStartEpochTime(clock, 0);
    QCOMPARE(actualTime, zeroTime);

    QString oneSecond = "12am on the 1st of Afteryule, Year 2850 of the Third Age.";
    actualTime = testMumeStartEpochTime(clock, 1);
    QCOMPARE(actualTime, oneSecond);

    QString oneTick = "1am on the 1st of Afteryule, Year 2850 of the Third Age.";
    actualTime = testMumeStartEpochTime(clock, 60);
    QCOMPARE(actualTime, oneTick);

    QString oneDay = "12am on the 2nd of Afteryule, Year 2850 of the Third Age.";
    actualTime = testMumeStartEpochTime(clock, 60 * 24);
    QCOMPARE(actualTime, oneDay);

    QString oneMonth = "12am on the 1st of Solmath, Year 2850 of the Third Age.";
    actualTime = testMumeStartEpochTime(clock, 60 * 24 * 30);
    QCOMPARE(actualTime, oneMonth);

    QString oneYear = "12am on the 1st of Afteryule, Year 2851 of the Third Age.";
    actualTime = testMumeStartEpochTime(clock, 60 * 24 * 30 * 12);
    QCOMPARE(actualTime, oneYear);
}

void TestClock::parseMumeTimeTest()
{
    MumeClock clock;

    // Defaults to epoch time of zero
    QString expectedZeroEpoch = "1st of Afteryule, Year 2850 of the Third Age.";
    QCOMPARE(testMumeStartEpochTime(clock, 0), expectedZeroEpoch);

    // Real time is Wed Dec 20 07:03:27 2017 UTC.
    QString snapShot1 = "3pm on Highday, the 18th of Halimath, Year 3030 of the Third Age.";
    QString expected1 = "3pm on the 18th of Halimath, Year 3030 of the Third Age.";
    clock.parseMumeTime(snapShot1);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expected1);

    // Real time is Wed Dec 20 07:18:02 2017 UTC.
    QString snapShot2 = "5am on Sterday, the 19th of Halimath, Year 3030 of the Third Age.";
    QString expected2 = "5am on the 19th of Halimath, Year 3030 of the Third Age.";
    clock.parseMumeTime(snapShot2);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expected2);

    // Real time is Wed Dec 20 07:38:44 2017 UTC.
    QString snapShot3 = "2am on Sunday, the 20th of Halimath, Year 3030 of the Third Age.";
    QString expected3 = "2am on the 20th of Halimath, Year 3030 of the Third Age.";
    clock.parseMumeTime(snapShot3);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expected3);

    // Real time is Thu Dec 21 05:27:35 2017 UTC.
    QString snapShot4 = "3pm on Highday, the 14th of Blotmath, Year 3030 of the Third Age.";
    QString expected4 = "3pm on the 14th of Blotmath, Year 3030 of the Third Age.";
    clock.parseMumeTime(snapShot4);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expected4);

    // Sindarin Calendar
    QString sindarin = "3pm on Oraearon, the 14th of Hithui, Year 3030 of the Third Age.";
    QString expectedSindarin = expected4;
    clock.parseMumeTime(sindarin);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedSindarin);
}

void TestClock::parseWeatherClockSkewTest()
{
    MumeClock clock;

    QString snapShot1 = "3pm on Highday, the 18th of Halimath, Year 3030 of the Third Age.";
    QString expected1 = "3pm on the 18th of Halimath, Year 3030 of the Third Age.";
    int realTime1 = convertMumeRealTime("Real time is Wed Dec 20 07:03:27 2017 UTC.");
    clock.parseMumeTime(snapShot1, realTime1);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1)), expected1);

    // First sync
    int timeOccuredAt = 1;
    QString expectedTime = "6:00am on the 18th of Halimath, Year 3030 of the Third Age.";
    clock.parseWeather("The day has begun.", realTime1 + timeOccuredAt);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1 + timeOccuredAt)), expectedTime);

    // Mume running on time
    timeOccuredAt = 1 + 60;
    expectedTime = "7:00am on the 18th of Halimath, Year 3030 of the Third Age.";
    clock.parseWeather("The evil power begins to regress...", realTime1 + timeOccuredAt);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1 + timeOccuredAt)), expectedTime);

    // Mume running fast
    timeOccuredAt = 1 + 60 + 58;
    expectedTime = "8:00am on the 18th of Halimath, Year 3030 of the Third Age.";
    clock.parseWeather("The evil power begins to regress...", realTime1 + timeOccuredAt);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1 + timeOccuredAt)), expectedTime);

    // Mume running on time
    timeOccuredAt = 1 + 60 + 58 + 60;
    expectedTime = "9:00am on the 18th of Halimath, Year 3030 of the Third Age.";
    clock.parseWeather("The evil power begins to regress...", realTime1 + timeOccuredAt);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1 + timeOccuredAt)), expectedTime);

    // Mume running slow
    timeOccuredAt = 1 + 60 + 58 + 60 + 65;
    expectedTime = "10:00am on the 18th of Halimath, Year 3030 of the Third Age.";
    clock.parseWeather("The evil power begins to regress...", realTime1 + timeOccuredAt);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment(realTime1 + timeOccuredAt)), expectedTime);
}

void TestClock::parseWeatherTest()
{
    MumeClock clock;

    QString snapShot1 = "3pm on Highday, the 18th of Halimath, Year 3030 of the Third Age.";
    QString expectedTime = "3pm on the 18th of Halimath, Year 3030 of the Third Age.";
    clock.parseMumeTime(snapShot1);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    expectedTime = "5:00am on the 18th of Halimath, Year 3030 of the Third Age.";
    clock.parseWeather("Light gradually filters in, proclaiming a new sunrise outside.");
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    expectedTime = "6:00am on the 18th of Halimath, Year 3030 of the Third Age.";
    clock.parseWeather("It seems as if the day has begun.");
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    expectedTime = "9:00pm on the 18th of Halimath, Year 3030 of the Third Age.";
    clock.parseWeather("The deepening gloom announces another sunset outside.");
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);

    expectedTime = "10:00pm on the 18th of Halimath, Year 3030 of the Third Age.";
    clock.parseWeather("It seems as if the night has begun.");
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expectedTime);
}

void TestClock::parseClockTimeTest()
{
    MumeClock clock;

    // Clock set to coarse
    // Real time is Wed Dec 20 07:03:27 2017 UTC.
    QString snapShot1 = "3pm on Highday, the 18th of Halimath, Year 3030 of the Third Age.";
    QString expected1 = "3pm on the 18th of Halimath, Year 3030 of the Third Age.";
    clock.parseMumeTime(snapShot1);
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), expected1);

    QString afternoon = "12:34pm on the 18th of Halimath, Year 3030 of the Third Age.";
    clock.parseClockTime("The current time is 12:34 pm.");
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), afternoon);

    QString midnight = "12:51am on the 18th of Halimath, Year 3030 of the Third Age.";
    clock.parseClockTime("The current time is 12:51 am.");
    QCOMPARE(clock.toMumeTime(clock.getMumeMoment()), midnight);

}

QTEST_MAIN(TestClock)
