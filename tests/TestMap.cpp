// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "TestMap.h"

#include "../src/global/HideQDebug.h"
#include "../src/map/Diff.h"
#include "../src/map/Map.h"
#include "../src/map/TinyRoomIdSet.h"
#include "../src/map/sanitizer.h"

#include <QDebug>
#include <QtTest/QtTest>

TestMap::TestMap() = default;

TestMap::~TestMap() = default;

void TestMap::diffTest()
{
    Map::enableExtraSanityChecks(true);
    mmqt::HideQDebug forThisTest;
    test::testMapDiff();
}

void TestMap::mapTest()
{
    Map::enableExtraSanityChecks(true);
    mmqt::HideQDebug forThisTest;
    test::testMap();
}

void TestMap::sanitizerTest()
{
    Map::enableExtraSanityChecks(true);
    mmqt::HideQDebug forThisTest;
    test::testSanitizer();
}

void TestMap::tinyRoomIdSetTest()
{
    Map::enableExtraSanityChecks(true);
    mmqt::HideQDebug forThisTest;
    test::testTinyRoomIdSet();
}

void TestMap::roomIdSetTest()
{
    Map::enableExtraSanityChecks(true);
    mmqt::HideQDebug forThisTest;
    test::testRoomIdSet();
}

QTEST_MAIN(TestMap)
