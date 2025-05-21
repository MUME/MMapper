// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "testparser.h"

#include "../src/global/Charset.h"
#include "../src/global/TextUtils.h"
#include "../src/global/parserutils.h"
#include "../src/map/RawExit.h"
#include "../src/map/mmapper2room.h"
#include "../src/map/parseevent.h"
#include "../src/map/sanitizer.h"

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

static QDebug operator<<(QDebug debug, const ExitDirEnum dir)
{
#define X_CASE(UPPER) \
    case ExitDirEnum::UPPER: \
        return debug << "ExitDirEnum::" #UPPER
    switch (dir) {
        X_CASE(NORTH);
        X_CASE(SOUTH);
        X_CASE(EAST);
        X_CASE(WEST);
        X_CASE(UP);
        X_CASE(DOWN);
        X_CASE(UNKNOWN);
        X_CASE(NONE);
    default:
        return debug << "*error*";
    }
#undef X_CASE
}

static QDebug operator<<(QDebug debug, const ExitFlagEnum flag)
{
#define X_CASE(_UPPER, _lower, _Camel, _Friendly) \
    case ExitFlagEnum::_UPPER: \
        return debug << "ExitFlagEnum::" #_UPPER;
    switch (flag) {
        XFOREACH_EXIT_FLAG(X_CASE)
    }
    return debug << "*error*";
#undef X_CASE
}

static QDebug operator<<(QDebug debug, const ExitFlags flags)
{
    auto &&ns = debug.nospace();
    ns << "ExitFlags{";
    auto prefix = "";
    for (auto f : flags) {
        ns << prefix;
        prefix = " | ";
        ns << f;
    }
    ns << "}";

    return debug;
}

static QDebug operator<<(QDebug debug, const ExitsFlagsType f)
{
    auto &&ns = debug.nospace();
    ns << "ExitsFlagsType{";
    if (f != ExitsFlagsType{}) {
        ns << ".valid=" << f.isValid();
        for (auto dir : ALL_EXITS_NESWUD) {
            auto x = f.get(dir);
            if (!x.empty()) {
                ns << ", [" << dir << "] = " << x;
            }
        }
    }
    ns << "}";
    return debug;
}

void TestParser::createParseEventTest()
{
    static constexpr auto terrain = RoomTerrainEnum::INDOORS;
    auto check = [](const RoomName &roomName,
                    const RoomDesc &parsedRoomDescription,
                    const PromptFlagsType pFlags,
                    const size_t expectSkipped) {
        RoomContents roomContents = mmqt::makeRoomContents("Contents");
        ConnectedRoomFlagsType cFlags;
        cFlags.setValid();
        const ParseEvent e = ParseEvent::createEvent(CommandEnum::NORTH,
                                                     INVALID_SERVER_ROOMID,
                                                     RoomArea{},
                                                     roomName,
                                                     parsedRoomDescription,
                                                     roomContents,
                                                     ServerExitIds{},
                                                     terrain,
                                                     RawExits{},
                                                     pFlags,
                                                     cFlags);

        if ((false)) {
            qDebug() << e;
        }
        QCOMPARE(e.getRoomName(), sanitize(roomName));
        QCOMPARE(e.getRoomDesc(), sanitize(parsedRoomDescription));
        QCOMPARE(e.getRoomContents(), sanitize(roomContents));
        if (e.getExitsFlags() != ExitsFlagsType{}) {
            qInfo() << e.getExitsFlags() << " vs " << ExitsFlagsType{};
        }
        QCOMPARE(e.getExitsFlags(), ExitsFlagsType{});
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
