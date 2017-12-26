#include <QtTest/QtTest>
#include <QDebug>

#include "testparser.h"
#include "parserutils.h"

using namespace std;

TestParser::TestParser()
    : QObject()
{}

TestParser::~TestParser()
{
}

int convertMumeRealTime(const QString& realTime)
{
    // Real time is Wed Dec 20 07:03:27 2017 UTC.
    QString dateString = realTime.mid(13, 24);
    QDateTime parsedTime = QDateTime::fromString(dateString, "ddd MMM dd hh:mm:ss yyyy");
    return parsedTime.toTime_t();
}

void TestParser::removeAnsiMarksTest()
{
    QString ansiString("\033[32mHello world\033[0m");
    QString expected("Hello world");
    ParserUtils::removeAnsiMarks(ansiString);
    QCOMPARE(ansiString, expected);
}

void TestParser::latinToAsciiTest()
{
    QString latin("Nórui Nínui");
    QString expectedAscii("Norui Ninui");
    ParserUtils::latinToAscii(latin);
    QCOMPARE(latin, expectedAscii);
}

QTEST_MAIN(TestParser)
