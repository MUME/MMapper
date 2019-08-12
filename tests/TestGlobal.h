#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <QObject>

class TestGlobal : public QObject
{
    Q_OBJECT
public:
    TestGlobal();
    ~TestGlobal();

private Q_SLOTS:
    void ansi256ColorTest();
    void ansiToRgbTest();
};
