#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../src/global/macros.h"

#include <QObject>

class NODISCARD_QOBJECT TestHotkeyManager final : public QObject
{
    Q_OBJECT

public:
    TestHotkeyManager();
    ~TestHotkeyManager() final;

private Q_SLOTS:
    void keyNormalizationTest();
    void importExportRoundTripTest();
    void importEdgeCasesTest();
    void resetToDefaultsTest();
    void exportSortOrderTest();
    void setHotkeyTest();
    void removeHotkeyTest();
    void hasHotkeyTest();
    void invalidKeyValidationTest();
    void duplicateKeyBehaviorTest();
    void commentPreservationTest();
    void settingsPersistenceTest();
};
