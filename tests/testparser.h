#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <QObject>

class TestParser final : public QObject
{
    Q_OBJECT

public:
    TestParser();
    ~TestParser() final;

private Q_SLOTS:
    // ParserUtils
    void removeAnsiMarksTest();
    void latinToAsciiTest();
    void createParseEventTest();
};
