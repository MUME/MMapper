#include <memory>
#include <QDebug>
#include <QtTest/QtTest>

#include "mmapper2event.h"
#include "parseevent.h"
#include "parserutils.h"
#include "property.h"
#include "testparser.h"

TestParser::TestParser()
    : QObject()
{}

TestParser::~TestParser() {}

int convertMumeRealTime(const QString &realTime)
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

void TestParser::createParseEventTest()
{
    QString roomName = "Room";
    QString roomDescription = "Dynamic Description";
    QString parsedRoomDescription = "Static Description";
    char terrain = 1;
    ExitsFlagsType eFlags = EXITS_FLAGS_VALID;
    PromptFlagsType pFlags = PROMPT_FLAGS_VALID + terrain;
    ConnectedRoomFlagsType cFlags = CONNECTED_ROOM_FLAGS_VALID;
    std::unique_ptr<ParseEvent> event(Mmapper2Event::createEvent(
        CID_NORTH, roomName, roomDescription, parsedRoomDescription, eFlags, pFlags, cFlags));

    ParseEvent e = *event.get();
    QCOMPARE(Mmapper2Event::getRoomName(e), roomName);
    QCOMPARE(Mmapper2Event::getRoomDesc(e), roomDescription);
    QCOMPARE(Mmapper2Event::getParsedRoomDesc(e), parsedRoomDescription);
    QCOMPARE(Mmapper2Event::getExitFlags(e), eFlags);
    QCOMPARE(Mmapper2Event::getPromptFlags(e), pFlags);
    QCOMPARE(Mmapper2Event::getConnectedRoomFlags(e), cFlags);

    QCOMPARE(e.getMoveType(), static_cast<uint>(CID_NORTH));
    QCOMPARE(e.getNumSkipped(), 0u);
    QCOMPARE(e.size(), static_cast<size_t>(3));
    QCOMPARE(QString(e.next()->data()), roomName);
    QCOMPARE(QString(e.next()->data()), parsedRoomDescription);
    QCOMPARE(QString(e.next()->data()), QString(terrain));
    QCOMPARE(e.next(), nullptr);
}

QTEST_MAIN(TestParser)
