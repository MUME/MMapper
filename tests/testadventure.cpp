// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "testadventure.h"
#include "../src/adventure/lineparsers.h"

#include <QtTest/QtTest>

TestAdventure::TestAdventure() = default;

TestAdventure::~TestAdventure() = default;

void TestAdventure::testAccomplishedTaskParser() {}

void TestAdventure::testAchievementParser() {}

void TestAdventure::testDiedParser() {}

void TestAdventure::testGainedLevelParser() {}

void TestAdventure::testHintParser() {}

void TestAdventure::testKillAndXPParser()
{
    std::vector<QString> lines = {"foo", "bar", "hoochie"};
    std::vector<bool> parseResults{};

    KillAndXPParser parser{};

    //std::for_each(lines.begin(), lines.end(), [parser, parseResults](QString l){parseResults.insert(parser.parse(l))})
}

QTEST_MAIN(TestAdventure)
