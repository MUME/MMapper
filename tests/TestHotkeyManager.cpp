// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "TestHotkeyManager.h"

#include "../src/client/Hotkey.h"
#include "../src/client/HotkeyManager.h"
#include "../src/configuration/configuration.h"

#include <QCoreApplication>
#include <QSettings>
#include <QStringList>
#include <QtTest/QtTest>

TestHotkeyManager::TestHotkeyManager()
{
    setEnteredMain();
}
TestHotkeyManager::~TestHotkeyManager() = default;

void TestHotkeyManager::checkHk(const HotkeyManager &manager,
                                const Hotkey &hk,
                                std::string_view expected)
{
    auto actual = manager.getCommand(hk).value_or("");
    if (actual != expected) {
        qDebug() << "Failure for key:" << QString::fromStdString(hk.to_string())
                 << "Expected:" << QString::fromStdString(std::string(expected))
                 << "Actual:" << QString::fromStdString(actual);
    }
    QCOMPARE(actual, std::string(expected));
}

void TestHotkeyManager::keyNormalizationTest()
{
    HotkeyManager manager;
    manager.clear();

    // Test all base keys defined in macro
#define X_TEST_KEY(id, name, qkey, pol) \
    { \
        const std::string nameStr(name); \
        std::string boundName = nameStr; \
        Qt::KeyboardModifiers qm = (pol == HotkeyPolicyEnum::Keypad) ? Qt::KeypadModifier \
                                                                     : Qt::NoModifier; \
        if (pol == HotkeyPolicyEnum::ModifierRequired \
            || pol == HotkeyPolicyEnum::ModifierNotShift) { \
            boundName = "CTRL+" + nameStr; \
            qm |= Qt::ControlModifier; \
        } \
        QVERIFY(manager.setHotkey(Hotkey{boundName}, "cmd_" + nameStr)); \
        checkHk(manager, Hotkey{boundName}, "cmd_" + nameStr); \
        Hotkey hk(qkey, qm); \
        checkHk(manager, hk, "cmd_" + nameStr); \
    }

    XFOREACH_HOTKEY_BASE_KEYS(X_TEST_KEY)
#undef X_TEST_KEY

    // Test that modifiers are normalized to canonical order: SHIFT+CTRL+ALT+META

    // Set a hotkey with non-canonical modifier order
    QVERIFY(manager.setHotkey(Hotkey{"ALT+CTRL+F1"}, "test1"));

    // Should be retrievable with canonical order
    checkHk(manager, Hotkey{"CTRL+ALT+F1"}, "test1");

    // Should also be retrievable with the original order (due to normalization)
    checkHk(manager, Hotkey{"ALT+CTRL+F1"}, "test1");

    // Test all modifier combinations normalize correctly
    QVERIFY(manager.setHotkey(Hotkey{"META+ALT+SHIFT+CTRL+F2"}, "test2"));
    checkHk(manager, Hotkey{"SHIFT+CTRL+ALT+META+F2"}, "test2");

    // Test that case is normalized to uppercase
    QVERIFY(manager.setHotkey(Hotkey{"ctrl+f3"}, "test3"));
    checkHk(manager, Hotkey{"CTRL+F3"}, "test3");
}

void TestHotkeyManager::importExportRoundTripTest()
{
    HotkeyManager manager;
    manager.clear();

    // Verify all hotkeys can be set and retrieved
    QVERIFY(manager.setHotkey(Hotkey{"F1"}, "look"));
    QVERIFY(manager.setHotkey(Hotkey{"CTRL+F2"}, "open exit n"));
    QVERIFY(manager.setHotkey(Hotkey{"SHIFT+ALT+F3"}, "pick exit s"));
    QVERIFY(manager.setHotkey(Hotkey{"NUMPAD8"}, "n"));

    checkHk(manager, Hotkey{"F1"}, "look");
    checkHk(manager, Hotkey{"CTRL+F2"}, "open exit n");
    checkHk(manager, Hotkey{"SHIFT+ALT+F3"}, "pick exit s");
    checkHk(manager, Hotkey{"NUMPAD8"}, "n");

    // Verify total count
    QCOMPARE(static_cast<int>(manager.getAllHotkeys().size()), 4);

    // Verify serialization
    QCOMPARE(QString::fromStdString(Hotkey{"F1"}.to_string()), QString("F1"));
    QCOMPARE(QString::fromStdString(Hotkey{"CTRL+F2"}.to_string()), QString("CTRL+F2"));
    QCOMPARE(QString::fromStdString(Hotkey{"SHIFT+ALT+F3"}.to_string()), QString("SHIFT+ALT+F3"));
    QCOMPARE(QString::fromStdString(Hotkey{"NUMPAD8"}.to_string()), QString("NUMPAD8"));
}

void TestHotkeyManager::importEdgeCasesTest()
{
    HotkeyManager manager;
    manager.clear();

    // Test command with multiple spaces (should preserve spaces in command)
    std::ignore = manager.setHotkey(Hotkey{"F1"}, "cast 'cure light'");
    QCOMPARE(manager.getCommand(Hotkey{"F1"}).value_or(""), std::string("cast 'cure light'"));

    // Test leading/trailing whitespace handling in key name
    QVERIFY(manager.setHotkey(Hotkey{"  F4  "}, "command with spaces"));
    QCOMPARE(manager.getCommand(Hotkey{"F4"}).value_or(""), std::string("command with spaces"));
}

void TestHotkeyManager::resetToDefaultsTest()
{
    HotkeyManager manager;
    manager.clear();

    // Set custom hotkeys
    std::ignore = manager.setHotkey(Hotkey{"F1"}, "custom");
    std::ignore = manager.setHotkey(Hotkey{"F2"}, "another");
    checkHk(manager, Hotkey{"F1"}, "custom");
    QCOMPARE(static_cast<int>(manager.getAllHotkeys().size()), 2);

    // Reset to defaults
    manager.resetToDefaults();

    // Verify defaults are restored
    checkHk(manager, Hotkey{"NUMPAD8"}, "north");
    checkHk(manager, Hotkey{"NUMPAD4"}, "west");
    checkHk(manager, Hotkey{"CTRL+NUMPAD8"}, "open exit north");
    checkHk(manager, Hotkey{"ALT+NUMPAD8"}, "close exit north");
    checkHk(manager, Hotkey{"SHIFT+NUMPAD8"}, "pick exit north");

    // F1 is in defaults
    checkHk(manager, Hotkey{"F1"}, "F1");

    // Verify defaults are non-empty (don't assert exact count to avoid brittleness)
    QVERIFY(!manager.getAllHotkeys().empty());
}

void TestHotkeyManager::exportCountTest()
{
    HotkeyManager manager;
    manager.clear();

    std::ignore = manager.setHotkey(Hotkey{"F1"}, "cmd1");
    std::ignore = manager.setHotkey(Hotkey{"F2"}, "cmd2");

    auto hotkeys = manager.getAllHotkeys();
    QCOMPARE(static_cast<int>(hotkeys.size()), 2);
}

void TestHotkeyManager::setHotkeyTest()
{
    HotkeyManager manager;
    manager.clear();

    // Test setting a new hotkey directly
    QVERIFY(manager.setHotkey(Hotkey{"F1"}, "look"));
    checkHk(manager, Hotkey{"F1"}, "look");
    QCOMPARE(static_cast<int>(manager.getAllHotkeys().size()), 1);

    // Test setting another hotkey
    QVERIFY(manager.setHotkey(Hotkey{"F2"}, "flee"));
    checkHk(manager, Hotkey{"F2"}, "flee");
    QCOMPARE(static_cast<int>(manager.getAllHotkeys().size()), 2);

    // Test updating an existing hotkey (should replace, not add)
    QVERIFY(manager.setHotkey(Hotkey{"F1"}, "inventory"));
    checkHk(manager, Hotkey{"F1"}, "inventory");
    QCOMPARE(static_cast<int>(manager.getAllHotkeys().size()), 2); // Still 2, not 3

    // Test setting hotkey with modifiers
    QVERIFY(manager.setHotkey(Hotkey{"CTRL+F3"}, "open exit n"));
    checkHk(manager, Hotkey{"CTRL+F3"}, "open exit n");
    QCOMPARE(static_cast<int>(manager.getAllHotkeys().size()), 3);
}

void TestHotkeyManager::removeHotkeyTest()
{
    HotkeyManager manager;
    manager.clear();

    std::ignore = manager.setHotkey(Hotkey{"F1"}, "look");
    std::ignore = manager.setHotkey(Hotkey{"F2"}, "flee");
    std::ignore = manager.setHotkey(Hotkey{"CTRL+F3"}, "open exit n");
    QCOMPARE(static_cast<int>(manager.getAllHotkeys().size()), 3);

    // Test removing a hotkey
    std::ignore = manager.removeHotkey(Hotkey{"F1"});
    checkHk(manager, Hotkey{"F1"}, ""); // Should be empty now
    QCOMPARE(static_cast<int>(manager.getAllHotkeys().size()), 2);

    // Test removing hotkey with modifiers
    std::ignore = manager.removeHotkey(Hotkey{"CTRL+F3"});
    checkHk(manager, Hotkey{"CTRL+F3"}, "");
    QCOMPARE(static_cast<int>(manager.getAllHotkeys().size()), 1);
}

void TestHotkeyManager::invalidKeyValidationTest()
{
    HotkeyManager manager;
    manager.clear();

    // Test that invalid base keys are rejected
    QVERIFY(!manager.setHotkey(Hotkey{"F13"}, "invalid"));
    QCOMPARE(manager.getCommand(Hotkey{"F13"}).value_or(""), std::string("")); // Should not be set
    QCOMPARE(static_cast<int>(manager.getAllHotkeys().size()), 0);

    // Test typo in key name
    QVERIFY(!manager.setHotkey(Hotkey{"NUMPDA8"}, "typo")); // Typo: NUMPDA instead of NUMPAD
}

void TestHotkeyManager::duplicateKeyBehaviorTest()
{
    HotkeyManager manager;
    manager.clear();

    // Test that setHotkey replaces existing entry
    std::ignore = manager.setHotkey(Hotkey{"F1"}, "original");
    QCOMPARE(manager.getCommand(Hotkey{"F1"}).value_or(""), std::string("original"));
    QCOMPARE(static_cast<int>(manager.getAllHotkeys().size()), 1);

    QVERIFY(manager.setHotkey(Hotkey{"F1"}, "replaced"));
    QCOMPARE(manager.getCommand(Hotkey{"F1"}).value_or(""), std::string("replaced"));
    QCOMPARE(static_cast<int>(manager.getAllHotkeys().size()), 1); // Still 1, not 2
}

void TestHotkeyManager::directLookupTest()
{
    HotkeyManager manager;
    manager.clear();

    // Set hotkeys for testing
    std::ignore = manager.setHotkey(Hotkey{"F1"}, "look");
    std::ignore = manager.setHotkey(Hotkey{"CTRL+F2"}, "flee");
    std::ignore = manager.setHotkey(Hotkey{"NUMPAD8"}, "n");
    std::ignore = manager.setHotkey(Hotkey{"CTRL+NUMPAD5"}, "s");
    std::ignore = manager.setHotkey(Hotkey{"SHIFT+ALT+UP"}, "north");

    // Test direct lookup for function keys (isNumpad=false)
    checkHk(manager, Hotkey{Qt::Key_F1, Qt::NoModifier}, "look");
    checkHk(manager, Hotkey{Qt::Key_F2, Qt::ControlModifier}, "flee");

    // Test that wrong modifiers don't match
    checkHk(manager, Hotkey{Qt::Key_F1, Qt::ControlModifier}, "");

    // Test numpad keys (isNumpad=true) - Qt::Key_8 with isNumpad=true
    checkHk(manager, Hotkey{Qt::Key_8, Qt::KeypadModifier}, "n");
    checkHk(manager, Hotkey{Qt::Key_5, Qt::ControlModifier | Qt::KeypadModifier}, "s");

    // Test that numpad keys don't match non-numpad lookups
    checkHk(manager, Hotkey{Qt::Key_8, Qt::NoModifier}, "");

    // Test arrow keys (isNumpad=false)
    checkHk(manager, Hotkey{Qt::Key_Up, (Qt::ShiftModifier | Qt::AltModifier)}, "north");

    // Test NUMPAD8 (NumLock ON) which comes as Qt::Key_8 + Keypad
    std::ignore = manager.setHotkey(Hotkey{"NUMPAD8"}, "north");
    checkHk(manager, Hotkey{Qt::Key_8, Qt::KeypadModifier}, "north");

    // Pure keypad-style numeric key with KeypadModifier should be treated as numpad;
    // unless its a Mac
    {
        Hotkey numpad4Hotkey(Qt::Key_4, Qt::KeypadModifier);
        std::ignore = manager.setHotkey(numpad4Hotkey, "west");
        QVERIFY(numpad4Hotkey.isKeypad());
        QCOMPARE(numpad4Hotkey.base(), HotkeyEnum::NUMPAD4);
        checkHk(manager, numpad4Hotkey, "west");

        Hotkey leftHotkey(Qt::Key_Left, Qt::KeypadModifier);
        if constexpr (CURRENT_PLATFORM == PlatformEnum::Mac) {
            QCOMPARE(leftHotkey.base(), HotkeyEnum::LEFT);

        } else {
            QVERIFY(leftHotkey.isKeypad());
            QCOMPARE(leftHotkey.base(), HotkeyEnum::NUMPAD4);
            checkHk(manager, leftHotkey, "west");
        }
    }
}

void TestHotkeyManager::hotkeyParsingAndToStringTest()
{
    struct Case
    {
        std::string input;
        std::string canonical;
        bool valid;
    };

    const Case cases[] = {
        // Simple modifier + key, lower-case input -> canonical form
        {"ctrl+f1", "CTRL+F1", true},

        // Extra whitespace and mixed case; modifier order should be canonicalized
        {"  CTRL   +   shift  +  f1  ", "SHIFT+CTRL+F1", true},

        // Different modifier order in input, expect canonical order in output
        {"alt+ctrl+f1", "CTRL+ALT+F1", true},

        // Single key without modifiers
        {"f5", "F5", true},

        // Policy violations (Recognized, but invalid)
        {"1", "1", false},
        {"UP", "UP", false},
        {"SHIFT+1", "SHIFT+1", false},

        // Valid with required modifiers
        {"CTRL+1", "CTRL+1", true},
        {"SHIFT+UP", "SHIFT+UP", true},

        // Duplicate modifiers should be tolerated and normalized
        {"Ctrl+ctrl+SHIFT+f1", "SHIFT+CTRL+F1", true},

        // Unknown base key
        {"CTRL+INVALIDKEY", "", false},

        // Unknown modifier token
        {"Super+F1", "", false},

        // Empty string is invalid
        {"", "", false},
    };

    for (const auto &c : cases) {
        Hotkey hk(c.input);

        QCOMPARE(hk.isValid(), c.valid);

        if (c.valid) {
            // Canonical string form
            QCOMPARE(hk.to_string(), c.canonical);

            // Round-trip: constructing from canonical form must preserve it
            Hotkey roundTrip(hk.to_string());
            QVERIFY(roundTrip.isValid());
            QCOMPARE(roundTrip.to_string(), c.canonical);
        } else if (!c.canonical.empty()) {
            // Recognized but invalid
            QVERIFY(hk.isRecognized());
            QCOMPARE(hk.to_string(), c.canonical);
        } else {
            // Unrecognized cases should return INVALID enum
            QCOMPARE(hk.base(), HotkeyEnum::INVALID);
        }
    }
}

QTEST_MAIN(TestHotkeyManager)
