// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "TestGroup.h"

#include "../src/group/enums.h"

#include <QDebug>
#include <QtTest/QtTest>

TestGroup::TestGroup() = default;
TestGroup::~TestGroup() = default;

void TestGroup::enumsTest()
{
    test::test_group_enums();
}

QTEST_MAIN(TestGroup)
