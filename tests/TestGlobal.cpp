// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "TestGlobal.h"

#include <QDebug>
#include <QtTest/QtTest>

#include "../src/global/Color.h"

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

QTEST_MAIN(TestGlobal)
