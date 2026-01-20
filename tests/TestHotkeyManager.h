#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../src/global/macros.h"

#include <QObject>

class Hotkey;
class HotkeyManager;

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
    void exportCountTest();
    void setHotkeyTest();
    void removeHotkeyTest();
    void invalidKeyValidationTest();
    void duplicateKeyBehaviorTest();
    void directLookupTest();
    void hotkeyParsingAndToStringTest();

private:
    void checkHk(const HotkeyManager &manager, const Hotkey &hk, std::string_view expected);
};
