#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../src/expandoracommon/property.h"
#include "../src/map/parseevent.h"
#include "../src/map/room.h"

#include <QObject>

Q_DECLARE_METATYPE(SharedRoom)
Q_DECLARE_METATYPE(SharedParseEvent)
Q_DECLARE_METATYPE(ComparisonResultEnum)

class TestExpandoraCommon final : public QObject
{
    Q_OBJECT

public:
    TestExpandoraCommon();
    ~TestExpandoraCommon() final;

private:
private Q_SLOTS:
    void skippablePropertyTest();
    void stringPropertyTest();
    void roomCompareTest_data();
    void roomCompareTest();
};
