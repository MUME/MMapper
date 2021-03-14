// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "testparser.h"

#include <memory>
#include <QDebug>
#include <QString>
#include <QtTest/QtTest>

#include "../src/expandoracommon/parseevent.h"
#include "../src/expandoracommon/property.h"
#include "../src/global/TextUtils.h"
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
    QString utf8("Nórui Nínui");
    const QString expectedAscii("Norui Ninui");
    ParserUtils::toAsciiInPlace(utf8);
    QCOMPARE(utf8, expectedAscii);
}

void TestParser::createParseEventTest()
{
    RoomName roomName{"Room"};
    RoomStaticDesc parsedRoomDescription{"Static Description"};
    RoomDynamicDesc roomDescription{"Dynamic Description"};
    auto terrain = RoomTerrainEnum::INDOORS;
    ExitsFlagsType eFlags;
    eFlags.setValid();
    auto pFlags = PromptFlagsType::fromRoomTerrainType(terrain);
    ConnectedRoomFlagsType cFlags;
    cFlags.setValid();
    auto event = ParseEvent::createEvent(CommandEnum::NORTH,
                                         roomName,
                                         parsedRoomDescription,
                                         roomDescription,
                                         eFlags,
                                         pFlags,
                                         cFlags);

    const ParseEvent &e = *event;
    qDebug() << e;
    QCOMPARE(e.getRoomName(), roomName);
    QCOMPARE(e.getStaticDesc(), parsedRoomDescription);
    QCOMPARE(e.getDynamicDesc(), roomDescription);
    QCOMPARE(e.getExitsFlags(), eFlags);
    QCOMPARE(e.getPromptFlags(), pFlags);
    QCOMPARE(e.getConnectedRoomFlags(), cFlags);

    QCOMPARE(e.getMoveType(), CommandEnum::NORTH);
    QCOMPARE(e.getNumSkipped(), 0u);
    QCOMPARE(RoomName(e[0].getStdString()), roomName);
    QCOMPARE(RoomStaticDesc(e[1].getStdString()), parsedRoomDescription);
    QCOMPARE(::toQStringLatin1(e[2].getStdString()),
             ::toQStringLatin1(std::string(1, static_cast<char>(terrain))));
}

QTEST_MAIN(TestParser)
