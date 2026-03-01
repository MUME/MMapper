#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../src/global/macros.h"

#include <QObject>

class NODISCARD_QOBJECT TestPasswordField final : public QObject
{
    Q_OBJECT

public:
    TestPasswordField();
    ~TestPasswordField() final;

private Q_SLOTS:
    // Bullet stripping — the logic that replaces password.right(1)
    void testStripBulletsSingleChar();
    void testStripBulletsMultiCharPaste();
    void testStripBulletsNoBullets();
    void testStripBulletsOnlyBullets();
    void testStripBulletsPartialBullets();
    void testStripBulletsMidStringBullets();

    // Password field state machine
    void testDummyDotsShownOnLoad();
    void testTypeSingleCharClearsDummy();
    void testPasteOverDummyPreservesAll();
    void testNormalTypingWithoutDummy();
    void testClearPasswordDeletesStored();
    void testHideRestoresDummyDots();
    void testUncheckedClearsEverything();
};
