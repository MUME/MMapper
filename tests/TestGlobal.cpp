// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "TestGlobal.h"

#include <QDebug>
#include <QtTest/QtTest>

#include "../src/global/AnsiColor.h"
#include "../src/global/StringView.h"
#include "../src/global/TextUtils.h"
#include "../src/global/string_view_utils.h"
#include "../src/global/unquote.h"

TestGlobal::TestGlobal() = default;

TestGlobal::~TestGlobal() = default;

void TestGlobal::ansi256ColorTest()
{
    int blackAnsi = rgbToAnsi256(0, 0, 0);
    QCOMPARE(blackAnsi, 16);
    QColor blackRgb = ansi256toRgb(blackAnsi);
    QCOMPARE(blackRgb, QColor(Qt::black));

    int redAnsi = rgbToAnsi256(255, 0, 0);
    QCOMPARE(redAnsi, 196);
    QColor redRgb = ansi256toRgb(redAnsi);
    QCOMPARE(redRgb, QColor(Qt::red));

    int greenAnsi = rgbToAnsi256(0, 255, 0);
    QCOMPARE(greenAnsi, 46);
    QColor greenRgb = ansi256toRgb(greenAnsi);
    QCOMPARE(greenRgb, QColor(Qt::green));

    int blueAnsi = rgbToAnsi256(0, 0, 255);
    QCOMPARE(blueAnsi, 21);
    QColor blueRgb = ansi256toRgb(blueAnsi);
    QCOMPARE(blueRgb, QColor(Qt::blue));

    int cyanAnsi = rgbToAnsi256(0, 255, 255);
    QCOMPARE(cyanAnsi, 51);
    QColor cyanRgb = ansi256toRgb(cyanAnsi);
    QCOMPARE(cyanRgb, QColor(Qt::cyan));

    int whiteAnsi = rgbToAnsi256(255, 255, 255);
    QCOMPARE(whiteAnsi, 231);
    QColor whiteRgb = ansi256toRgb(whiteAnsi);
    QCOMPARE(whiteRgb, QColor(Qt::white));

    int grayAnsi = rgbToAnsi256(128, 128, 128);
    QCOMPARE(grayAnsi, 244);
    QColor grayRgb = ansi256toRgb(grayAnsi);
    QCOMPARE(grayRgb, QColor(Qt::darkGray));
}

void TestGlobal::ansiToRgbTest()
{
    int cyanAnsi = 153;
    QColor cyanRgb = ansi256toRgb(cyanAnsi);
    QCOMPARE(cyanRgb, QColor("#99ccff"));

    int blackAnsi = static_cast<int>(AnsiColorTableEnum::black);
    QColor blackRgb = ansi256toRgb(blackAnsi);
    QCOMPARE(blackRgb, QColor("#2e3436"));

    int highBlackAnsi = static_cast<int>(AnsiColorTableEnum::BLACK) + 8 - 60;
    QColor highBlackRgb = ansi256toRgb(highBlackAnsi);
    QCOMPARE(highBlackRgb, QColor("#555753"));
}

void TestGlobal::stringViewTest()
{
    // REVISIT: Test is meaningless during release builds
    test::testStringView();
}

void TestGlobal::unquoteTest()
{
    // REVISIT: Test is meaningless during release builds
    test::test_unquote();
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
    QCOMPARE(toLowerLatin1('-'), '-');
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

QTEST_MAIN(TestGlobal)
