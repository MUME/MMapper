// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "testparser.h"

#include "../src/global/Charset.h"
#include "../src/global/TextUtils.h"
#include "../src/global/parserutils.h"
#include "../src/map/mmapper2room.h"
#include "../src/map/parseevent.h"
#include "../src/map/sanitizer.h"

#include <memory>

#include <QDebug>
#include <QString>
#include <QtTest/QtTest>

// FIXME: This is effectively a duplication of the map sanitizer functions.
namespace { // anonymous
template<typename T>
T sanitize(const T &input) = delete;
template<>
RoomName sanitize<>(const RoomName &nameStr)
{
    return RoomName{sanitizer::sanitizeOneLine(nameStr.toStdStringUtf8())};
}
template<>
RoomDesc sanitize(const RoomDesc &descStr)
{
    return RoomDesc{sanitizer::sanitizeMultiline(descStr.toStdStringUtf8())};
}
template<>
RoomContents sanitize(const RoomContents &descStr)
{
    return RoomContents{sanitizer::sanitizeMultiline(descStr.toStdStringUtf8())};
}
} // namespace

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
    const QString qs("N\u00F3rui N\u00EDnui");
    QCOMPARE(qs.length(), 11);
    {
        const QString expectedAscii("Norui Ninui");
        QCOMPARE(expectedAscii.length(), 11);

        auto copy = qs;
        mmqt::toAsciiInPlace(copy);
        QCOMPARE(copy, expectedAscii);
    }
    {
        const auto latin1 = mmqt::toStdStringLatin1(qs); // sanity checking
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
    static constexpr auto terrain = RoomTerrainEnum::INDOORS;
    auto check = [](const RoomName &roomName,
                    const RoomDesc &parsedRoomDescription,
                    const PromptFlagsType pFlags,
                    const size_t expectSkipped) {
        RoomContents roomContents = mmqt::makeRoomContents("Contents");
        ExitsFlagsType eFlags;
        eFlags.setValid();
        ConnectedRoomFlagsType cFlags;
        cFlags.setValid();
        const ParseEvent e = ParseEvent::createEvent(CommandEnum::NORTH,
                                                     INVALID_SERVER_ROOMID,
                                                     roomName,
                                                     parsedRoomDescription,
                                                     roomContents,
                                                     ServerExitIds{},
                                                     terrain,
                                                     eFlags,
                                                     pFlags,
                                                     cFlags);

        if ((false)) {
            qDebug() << e;
        }
        QCOMPARE(e.getRoomName(), sanitize(roomName));
        QCOMPARE(e.getRoomDesc(), sanitize(parsedRoomDescription));
        QCOMPARE(e.getRoomContents(), sanitize(roomContents));
        QCOMPARE(e.getExitsFlags(), eFlags);
        QCOMPARE(e.getPromptFlags(), pFlags);
        QCOMPARE(e.getConnectedRoomFlags(), cFlags);

        QCOMPARE(e.getMoveType(), CommandEnum::NORTH);
        QCOMPARE(e.getNumSkipped(), expectSkipped);
    };

    const auto name = RoomName{"Room"};
    const auto desc = makeRoomDesc("Description");
    const auto promptFlags = []() {
        PromptFlagsType pf{};
        pf.setValid();
        return pf;
    }();

    // all 3 valid
    check(name, desc, promptFlags, 0);

    // one missing
    check(name, desc, {}, 1);
    check(name, {}, promptFlags, 1);
    check({}, desc, promptFlags, 1);

    // two missing
    check(name, {}, {}, 2);
    check({}, desc, {}, 2);
    check({}, {}, promptFlags, 2);

    // all three missing
    check({}, {}, {}, 3);
}

QTEST_MAIN(TestParser)
