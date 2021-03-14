#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <QObject>

#include "../src/expandoracommon/parseevent.h"
#include "../src/expandoracommon/property.h"
#include "../src/expandoracommon/room.h"

Q_DECLARE_METATYPE(SharedRoom)
Q_DECLARE_METATYPE(SharedParseEvent)
Q_DECLARE_METATYPE(ComparisonResultEnum)

class TestExpandoraCommon final : public QObject
{
    Q_OBJECT
public:
    TestExpandoraCommon();
    ~TestExpandoraCommon() override;

private:
private Q_SLOTS:
    void skippablePropertyTest();
    void stringPropertyTest();
    void roomCompareTest_data();
    void roomCompareTest();
};
