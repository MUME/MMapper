// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "testparser.h"

#include "../src/expandoracommon/property.h"
#include "../src/global/Charset.h"
#include "../src/global/TextUtils.h"
#include "../src/global/parserutils.h"
#include "../src/map/mmapper2room.h"
#include "../src/map/parseevent.h"

#include <memory>

#include <QDebug>
#include <QString>
#include <QtTest/QtTest>

TestParser::TestParser() = default;

TestParser::~TestParser() = default;

void TestParser::removeAnsiMarksTest()
{
    QString ansiString("\033[32mHello world\033[0m");
    QString expected("Hello world");
    ParserUtils::removeAnsiMarksInPlace(ansiString);
    QCOMPARE(ansiString, expected);
}

void TestParser::toAsciiTest()
{
    const QString qs("Nórui Nínui");
    QCOMPARE(qs.length(), 11);
    {
        const QString expectedAscii("Norui Ninui");
        QCOMPARE(expectedAscii.length(), 11);

        auto copy = qs;
        mmqt::toAsciiInPlace(copy);
        QCOMPARE(copy, expectedAscii);
    }
    {
        const auto latin1 = mmqt::toStdStringLatin1(qs);
        QCOMPARE(latin1.length(), 11);
        QCOMPARE(latin1[1], '\xF3');
        QCOMPARE(latin1[7], '\xED');
    }
    {
        const auto utf8 = mmqt::toStdStringUtf8(qs);
        QCOMPARE(utf8.length(), 13);
        QCOMPARE(utf8[1], '\xC3');
        QCOMPARE(utf8[2], '\xB3');

        QCOMPARE(utf8[8], '\xC3');
        QCOMPARE(utf8[9], '\xAD');
    }
}

void TestParser::createParseEventTest()
{
    RoomName roomName{"Room"};
    RoomDesc parsedRoomDescription{"Description"};
    RoomContents roomContents{"Contents"};
    auto terrain = RoomTerrainEnum::INDOORS;
    ExitsFlagsType eFlags;
    eFlags.setValid();
    PromptFlagsType pFlags;
    pFlags.setValid();
    ConnectedRoomFlagsType cFlags;
    cFlags.setValid();
    auto event = ParseEvent::createEvent(CommandEnum::NORTH,
                                         roomName,
                                         parsedRoomDescription,
                                         roomContents,
                                         terrain,
                                         eFlags,
                                         pFlags,
                                         cFlags);

    const ParseEvent &e = *event;
    qDebug() << e;
    QCOMPARE(e.getRoomName(), roomName);
    QCOMPARE(e.getRoomDesc(), parsedRoomDescription);
    QCOMPARE(e.getRoomContents(), roomContents);
    QCOMPARE(e.getExitsFlags(), eFlags);
    QCOMPARE(e.getPromptFlags(), pFlags);
    QCOMPARE(e.getConnectedRoomFlags(), cFlags);

    QCOMPARE(e.getMoveType(), CommandEnum::NORTH);
    QCOMPARE(e.getNumSkipped(), 0u);
    QCOMPARE(RoomName(e[0].getStdString()), roomName);
    QCOMPARE(RoomDesc(e[1].getStdString()), parsedRoomDescription);
    QCOMPARE(mmqt::toQStringLatin1(e[2].getStdString()),
             mmqt::toQStringLatin1(std::string(1, static_cast<char>(terrain))));
}

QTEST_MAIN(TestParser)
