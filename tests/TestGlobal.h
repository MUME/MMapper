#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../src/global/macros.h"

#include <QObject>

class NODISCARD_QOBJECT TestGlobal final : public QObject
{
    Q_OBJECT

public:
    TestGlobal();
    ~TestGlobal() final;

private Q_SLOTS:
    static void ansi256ColorTest();
    static void ansiTextUtilsTest();
    static void ansiToRgbTest();
    static void charUtilsTest();
    static void colorTest();
    static void entitiesTest();
    static void hideQDebugTest();
    static void stringViewTest();
    static void toLowerLatin1Test();
    static void to_numberTest();
    static void unquoteTest();
    static void weakHandleTest();
};
