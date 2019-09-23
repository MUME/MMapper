// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "testparser.h"

#include <memory>
#include <QDebug>
#include <QString>
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
    RoomName roomName{"Room"};
    RoomDynamicDesc roomDescription{"Dynamic Description"};
    RoomStaticDesc parsedRoomDescription{"Static Description"};
    auto terrain = RoomTerrainEnum::INDOORS;
    ExitsFlagsType eFlags;
    eFlags.setValid();
    auto pFlags = PromptFlagsType::fromRoomTerrainType(terrain);
    ConnectedRoomFlagsType cFlags;
    cFlags.setValid();
    auto event = ParseEvent::createEvent(CommandEnum::NORTH,
                                         roomName,
                                         roomDescription,
                                         parsedRoomDescription,
                                         eFlags,
                                         pFlags,
                                         cFlags);

    const ParseEvent &e = *event;
    qDebug() << e;
    QCOMPARE(e.getRoomName(), roomName);
    QCOMPARE(e.getDynamicDesc(), roomDescription);
    QCOMPARE(e.getStaticDesc(), parsedRoomDescription);
    QCOMPARE(e.getExitsFlags(), eFlags);
    QCOMPARE(e.getPromptFlags(), pFlags);
    QCOMPARE(e.getConnectedRoomFlags(), cFlags);

    QCOMPARE(e.getMoveType(), CommandEnum::NORTH);
    QCOMPARE(e.getNumSkipped(), 0u);
    QCOMPARE(RoomName(e[0].getStdString()), roomName);
    QCOMPARE(RoomStaticDesc(e[1].getStdString()), parsedRoomDescription);
    QCOMPARE(QString::fromStdString(e[2].getStdString()),
             QString::fromStdString(std::string(1, static_cast<char>(terrain))));
}

QTEST_MAIN(TestParser)
