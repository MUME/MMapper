// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "TestGlobal.h"

#include "../src/global/AnsiTextUtils.h"
#include "../src/global/CaseUtils.h"
#include "../src/global/StringView.h"
#include "../src/global/TextUtils.h"
#include "../src/global/string_view_utils.h"
#include "../src/global/unquote.h"

#include <QDebug>
#include <QtTest/QtTest>

TestGlobal::TestGlobal() = default;

TestGlobal::~TestGlobal() = default;

void TestGlobal::ansi256ColorTest()
{
    const int blackAnsi = rgbToAnsi256(0, 0, 0);
    QCOMPARE(blackAnsi, 16);
    const QColor blackRgb = mmqt::ansi256toRgb(blackAnsi);
    QCOMPARE(blackRgb, QColor(Qt::black));

    const int redAnsi = rgbToAnsi256(255, 0, 0);
    QCOMPARE(redAnsi, 196);
    const QColor redRgb = mmqt::ansi256toRgb(redAnsi);
    QCOMPARE(redRgb, QColor(Qt::red));

    const int greenAnsi = rgbToAnsi256(0, 255, 0);
    QCOMPARE(greenAnsi, 46);
    const QColor greenRgb = mmqt::ansi256toRgb(greenAnsi);
    QCOMPARE(greenRgb, QColor(Qt::green));

    const int yellowAnsi = rgbToAnsi256(255, 255, 0);
    QCOMPARE(yellowAnsi, 226);
    const QColor yellowRgb = mmqt::ansi256toRgb(yellowAnsi);
    QCOMPARE(yellowRgb, QColor(Qt::yellow));

    const int blueAnsi = rgbToAnsi256(0, 0, 255);
    QCOMPARE(blueAnsi, 21);
    const QColor blueRgb = mmqt::ansi256toRgb(blueAnsi);
    QCOMPARE(blueRgb, QColor(Qt::blue));

    const int magentaAnsi = rgbToAnsi256(255, 0, 255);
    QCOMPARE(magentaAnsi, 201);
    const QColor magentaRgb = mmqt::ansi256toRgb(magentaAnsi);
    QCOMPARE(magentaRgb, QColor(Qt::magenta));

    const int cyanAnsi = rgbToAnsi256(0, 255, 255);
    QCOMPARE(cyanAnsi, 51);
    const QColor cyanRgb = mmqt::ansi256toRgb(cyanAnsi);
    QCOMPARE(cyanRgb, QColor(Qt::cyan));

    const int whiteAnsi = rgbToAnsi256(255, 255, 255);
    QCOMPARE(whiteAnsi, 231);
    const QColor whiteRgb = mmqt::ansi256toRgb(whiteAnsi);
    QCOMPARE(whiteRgb, QColor(Qt::white));

    const int grayAnsi = rgbToAnsi256(128, 128, 128);
    QCOMPARE(grayAnsi, 244);
    const QColor grayRgb = mmqt::ansi256toRgb(grayAnsi);
    QCOMPARE(grayRgb, QColor(Qt::darkGray));

    // these are called for the side-effect of testing their asserts.
#define X_TEST(N, lower, UPPER) \
    do { \
        const std::string_view testing = #lower; \
        MAYBE_UNUSED const auto fg = mmqt::rgbToAnsi256String(lower##Rgb, \
                                                              AnsiColor16LocationEnum::Foreground); \
        MAYBE_UNUSED const auto bg = mmqt::rgbToAnsi256String(lower##Rgb, \
                                                              AnsiColor16LocationEnum::Background); \
    } while (false);
    XFOREACH_ANSI_COLOR_0_7(X_TEST)

    // TODO: use colons instead of semicolons
    QCOMPARE(mmqt::rgbToAnsi256String(blackRgb, AnsiColor16LocationEnum::Foreground),
             QString("[38;5;16m"));
    QCOMPARE(mmqt::rgbToAnsi256String(blackRgb, AnsiColor16LocationEnum::Background),
             QString("[37;48;5;16m"));

    QCOMPARE(mmqt::rgbToAnsi256String(whiteRgb, AnsiColor16LocationEnum::Foreground),
             QString("[38;5;231m"));
    QCOMPARE(mmqt::rgbToAnsi256String(whiteRgb, AnsiColor16LocationEnum::Background),
             QString("[30;48;5;231m"));
#undef X_TEST
}

void TestGlobal::ansiToRgbTest()
{
    static_assert(153 == 16 + 36 * 3 + 6 * 4 + 5);
    // ansi_rgb6(3x4x5) is light blue with a lot of green, it's definitely not cyan.
    //
    // https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_(Select_Graphic_Rendition)_parameters
    // lists 153 as #afd7ff, which looks light blue.
    // If you're looking for cyan, try 159 (#afffff).
    const int cyanAnsi = 153;
    const QColor cyanRgb = mmqt::ansi256toRgb(cyanAnsi);
    QCOMPARE(cyanRgb, QColor("#afd7ff"));

    auto testOne = [](const int ansi256, const QColor color, const AnsiColor16Enum ansi) {
        QCOMPARE(mmqt::toQColor(ansi), color);
        QCOMPARE(mmqt::ansi256toRgb(ansi256), color);
    };

    testOne(0, "#2e3436", AnsiColor16Enum::black);
    testOne(6, "#06989a", AnsiColor16Enum::cyan);
    testOne(7, "#d3d7cf", AnsiColor16Enum::white);

    testOne(8, "#555753", AnsiColor16Enum::BLACK);
    testOne(14, "#34e2e2", AnsiColor16Enum::CYAN);
    testOne(15, "#eeeeec", AnsiColor16Enum::WHITE);
}

void TestGlobal::stringViewTest()
{
    // REVISIT: Test is meaningless during release builds
    test::testStringView();
}

void TestGlobal::toLowerLatin1Test()
{
    // Visible Latin-1 characters
    QCOMPARE(toLowerLatin1(char(192)), char(192 + 32));
    QCOMPARE(toLowerLatin1(char(221)), char(221 + 32));

    // Non-visible Latin-1 characters
    QCOMPARE(toLowerLatin1(char(191)), char(191));
    QCOMPARE(toLowerLatin1(char(222)), char(222));

    // Division
    QCOMPARE(toLowerLatin1(char(215)), char(215));

    QCOMPARE(toLowerLatin1('A'), 'a');
    QCOMPARE(toLowerLatin1('Z'), 'z');
    using char_consts::C_MINUS_SIGN;
    QCOMPARE(toLowerLatin1(C_MINUS_SIGN), C_MINUS_SIGN);
}

void TestGlobal::to_numberTest()
{
    bool ok = false;
    QCOMPARE(to_integer<uint64_t>(u"0", ok), 0);
    QCOMPARE(ok, true);
    QCOMPARE(to_integer<uint64_t>(u"1", ok), 1);
    QCOMPARE(ok, true);
    QCOMPARE(to_integer<uint64_t>(u"1234567890", ok), 1234567890);
    QCOMPARE(ok, true);
    QCOMPARE(to_integer<uint64_t>(u"12345678901234567890", ok), 12345678901234567890ull);
    QCOMPARE(ok, true);
    QCOMPARE(to_integer<uint64_t>(u"18446744073709551615", ok), 18446744073709551615ull);
    QCOMPARE(ok, true);
    QCOMPARE(to_integer<uint64_t>(u"18446744073709551616", ok), 0);
    QCOMPARE(ok, false);
    QCOMPARE(to_integer<uint64_t>(u"36893488147419103231", ok), 0);
    QCOMPARE(ok, false);
    QCOMPARE(to_integer<uint64_t>(u"92233720368547758079", ok), 0);
    QCOMPARE(ok, false);
    QCOMPARE(to_integer<uint64_t>(u"110680464442257309695", ok), 0);
    QCOMPARE(ok, false);
}

void TestGlobal::unquoteTest()
{
    // REVISIT: Test is meaningless during release builds
    test::test_unquote();
}

QTEST_MAIN(TestGlobal)
