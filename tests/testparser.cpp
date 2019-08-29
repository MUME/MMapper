// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "testparser.h"

#include <memory>
#include <QDebug>
#include <QtTest/QtTest>

#include "../src/expandoracommon/parseevent.h"
#include "../src/expandoracommon/property.h"
#include "../src/mapdata/mmapper2room.h"
#include "../src/parser/parserutils.h"

TestParser::TestParser() = default;

TestParser::~TestParser() = default;

void TestParser::removeAnsiMarksTest()
{
    QString ansiString("\033[32mHello world\033[0m");
    QString expected("Hello world");
    ParserUtils::removeAnsiMarksInPlace(ansiString);
    QCOMPARE(ansiString, expected);
}

void TestParser::latinToAsciiTest()
{
    QString latin("Nórui Nínui");
    QString expectedAscii("Norui Ninui");
    ParserUtils::latinToAsciiInPlace(latin);
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
    auto pFlags = PromptFlagsType::fromRoomTerrainType(terrain);
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
    qDebug() << e;
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
