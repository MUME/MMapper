#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <QObject>

class TestParser : public QObject
{
    Q_OBJECT
public:
    TestParser();
    ~TestParser() override;

private:
private Q_SLOTS:
    // ParserUtils
    void removeAnsiMarksTest();
    void latinToAsciiTest();
    void createParseEventTest();
};
