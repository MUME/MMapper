#include "testparser.h"

#include <memory>
#include <QDebug>
#include <QtTest/QtTest>

#include "../src/mapdata/mmapper2room.h"
#include "parseevent.h"
#include "parserutils.h"
#include "property.h"

TestParser::TestParser() = default;

TestParser::~TestParser() = default;

time_t convertMumeRealTime(const QString &realTime)
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
    auto terrain = RoomTerrainType::INDOORS;
    ExitsFlagsType eFlags{};
    eFlags.setValid();
    PromptFlagsType pFlags{terrain};
    pFlags.setValid();
    ConnectedRoomFlagsType cFlags{};
    cFlags.setValid();
    auto event = ParseEvent::createEvent(CommandIdType::NORTH,
                                         roomName,
                                         roomDescription,
                                         parsedRoomDescription,
                                         eFlags,
                                         pFlags,
                                         cFlags);

    ParseEvent &e = *event;
    QCOMPARE(e.getRoomName(), roomName);
    QCOMPARE(e.getDynamicDesc(), roomDescription);
    QCOMPARE(e.getStaticDesc(), parsedRoomDescription);
    QCOMPARE(e.getExitsFlags(), eFlags);
    QCOMPARE(e.getPromptFlags(), pFlags);
    QCOMPARE(e.getConnectedRoomFlags(), cFlags);

    QCOMPARE(e.getMoveType(), CommandIdType::NORTH);
    QCOMPARE(e.getNumSkipped(), 0u);
    QCOMPARE(e.size(), static_cast<size_t>(3));
    QCOMPARE(QString(e.next()->data()), roomName);
    QCOMPARE(QString(e.next()->data()), parsedRoomDescription);
    QCOMPARE(QString(e.next()->data()), QString(static_cast<int>(terrain)));
    QVERIFY(e.next() == nullptr);
}

QTEST_MAIN(TestParser)
