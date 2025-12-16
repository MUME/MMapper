// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "TestHotkeyManager.h"

#include "../src/configuration/HotkeyManager.h"

#include <QtTest/QtTest>

TestHotkeyManager::TestHotkeyManager() = default;
TestHotkeyManager::~TestHotkeyManager() = default;

void TestHotkeyManager::keyNormalizationTest()
{
    HotkeyManager manager;

    // Test that modifiers are normalized to canonical order: CTRL+SHIFT+ALT+META
    // Set a hotkey with non-canonical modifier order
    manager.setHotkey("ALT+CTRL+F1", "test1");

    // Should be retrievable with canonical order
    QCOMPARE(manager.getCommand("CTRL+ALT+F1"), QString("test1"));

    // Should also be retrievable with the original order (due to normalization)
    QCOMPARE(manager.getCommand("ALT+CTRL+F1"), QString("test1"));

    // Test all modifier combinations normalize correctly
    manager.setHotkey("META+ALT+SHIFT+CTRL+F2", "test2");
    QCOMPARE(manager.getCommand("CTRL+SHIFT+ALT+META+F2"), QString("test2"));

    // Test that case is normalized to uppercase
    manager.setHotkey("ctrl+f3", "test3");
    QCOMPARE(manager.getCommand("CTRL+F3"), QString("test3"));

    // Test CONTROL alias normalizes to CTRL
    manager.setHotkey("CONTROL+F4", "test4");
    QCOMPARE(manager.getCommand("CTRL+F4"), QString("test4"));

    // Test CMD/COMMAND aliases normalize to META
    manager.setHotkey("CMD+F5", "test5");
    QCOMPARE(manager.getCommand("META+F5"), QString("test5"));

    manager.setHotkey("COMMAND+F6", "test6");
    QCOMPARE(manager.getCommand("META+F6"), QString("test6"));

    // Test simple key without modifiers
    manager.setHotkey("f7", "test7");
    QCOMPARE(manager.getCommand("F7"), QString("test7"));

    // Test numpad keys
    manager.setHotkey("numpad8", "north");
    QCOMPARE(manager.getCommand("NUMPAD8"), QString("north"));
}

void TestHotkeyManager::importExportRoundTripTest()
{
    HotkeyManager manager;

    // Test import with a known string (this clears existing hotkeys)
    QString testConfig = "_hotkey F1 look\n"
                         "_hotkey CTRL+F2 open exit n\n"
                         "_hotkey SHIFT+ALT+F3 pick exit s\n"
                         "_hotkey NUMPAD8 n\n"
                         "_hotkey CTRL+SHIFT+NUMPAD_PLUS test command\n";

    int importedCount = manager.importFromCliFormat(testConfig);

    // Verify the import count
    QCOMPARE(importedCount, 5);

    // Verify all hotkeys were imported correctly
    QCOMPARE(manager.getCommand("F1"), QString("look"));
    QCOMPARE(manager.getCommand("CTRL+F2"), QString("open exit n"));
    QCOMPARE(manager.getCommand("SHIFT+ALT+F3"), QString("pick exit s"));
    QCOMPARE(manager.getCommand("NUMPAD8"), QString("n"));
    QCOMPARE(manager.getCommand("CTRL+SHIFT+NUMPAD_PLUS"), QString("test command"));

    // Verify total count
    QCOMPARE(manager.getAllHotkeys().size(), 5);

    // Export and verify content
    QString exported = manager.exportToCliFormat();
    QVERIFY(exported.contains("_hotkey F1 look"));
    QVERIFY(exported.contains("_hotkey CTRL+F2 open exit n"));
    QVERIFY(exported.contains("_hotkey NUMPAD8 n"));

    // Test that comments and empty lines are ignored during import
    QString contentWithComments = "# This is a comment\n"
                                  "\n"
                                  "_hotkey F10 flee\n"
                                  "# Another comment\n"
                                  "_hotkey F11 rest\n";

    int count = manager.importFromCliFormat(contentWithComments);
    QCOMPARE(count, 2);
    QCOMPARE(manager.getCommand("F10"), QString("flee"));
    QCOMPARE(manager.getCommand("F11"), QString("rest"));

    // Verify import cleared existing hotkeys
    QCOMPARE(manager.getAllHotkeys().size(), 2);
    QCOMPARE(manager.getCommand("F1"), QString()); // Should be cleared

    // Test another import clears and replaces
    manager.importFromCliFormat("_hotkey F12 stand\n");
    QCOMPARE(manager.getAllHotkeys().size(), 1);
    QCOMPARE(manager.getCommand("F10"), QString()); // Should be cleared
    QCOMPARE(manager.getCommand("F12"), QString("stand"));
}

void TestHotkeyManager::importEdgeCasesTest()
{
    HotkeyManager manager;

    // Test command with multiple spaces (should preserve spaces in command)
    manager.importFromCliFormat("_hotkey F1 cast 'cure light'");
    QCOMPARE(manager.getCommand("F1"), QString("cast 'cure light'"));

    // Test malformed lines are skipped
    // "_hotkey" alone - no key
    // "_hotkey F2" - no command
    // "_hotkey F3 valid" - valid
    manager.importFromCliFormat("_hotkey\n_hotkey F2\n_hotkey F3 valid");
    QCOMPARE(manager.getAllHotkeys().size(), 1);
    QCOMPARE(manager.getCommand("F3"), QString("valid"));

    // Test leading/trailing whitespace handling
    manager.importFromCliFormat("  _hotkey   F4   command with spaces  ");
    QCOMPARE(manager.getCommand("F4"), QString("command with spaces"));

    // Test empty input
    manager.importFromCliFormat("");
    QCOMPARE(manager.getAllHotkeys().size(), 0);

    // Test only comments and whitespace
    manager.importFromCliFormat("# comment\n\n# another comment\n   \n");
    QCOMPARE(manager.getAllHotkeys().size(), 0);
}

void TestHotkeyManager::resetToDefaultsTest()
{
    HotkeyManager manager;

    // Import custom hotkeys
    manager.importFromCliFormat("_hotkey F1 custom\n_hotkey F2 another");
    QCOMPARE(manager.getCommand("F1"), QString("custom"));
    QCOMPARE(manager.getAllHotkeys().size(), 2);

    // Reset to defaults
    manager.resetToDefaults();

    // Verify defaults are restored
    QCOMPARE(manager.getCommand("NUMPAD8"), QString("n"));
    QCOMPARE(manager.getCommand("NUMPAD4"), QString("w"));
    QCOMPARE(manager.getCommand("CTRL+NUMPAD8"), QString("open exit n"));
    QCOMPARE(manager.getCommand("ALT+NUMPAD8"), QString("close exit n"));
    QCOMPARE(manager.getCommand("SHIFT+NUMPAD8"), QString("pick exit n"));

    // F1 is not in defaults, should be empty
    QCOMPARE(manager.getCommand("F1"), QString());

    // Verify we have the expected number of defaults (30)
    QCOMPARE(manager.getAllHotkeys().size(), 30);
}

void TestHotkeyManager::exportSortOrderTest()
{
    HotkeyManager manager;

    // Import hotkeys in a specific order - order should be preserved (no auto-sorting)
    QString testConfig = "_hotkey CTRL+SHIFT+F1 two_mods\n"
                         "_hotkey F2 no_mods\n"
                         "_hotkey ALT+F3 one_mod\n"
                         "_hotkey F4 no_mods_2\n"
                         "_hotkey CTRL+F5 one_mod_2\n";

    manager.importFromCliFormat(testConfig);

    QString exported = manager.exportToCliFormat();

    // Find positions of each hotkey in the exported string
    const auto posF2 = exported.indexOf("_hotkey F2");
    const auto posF4 = exported.indexOf("_hotkey F4");
    const auto posAltF3 = exported.indexOf("_hotkey ALT+F3");
    const auto posCtrlF5 = exported.indexOf("_hotkey CTRL+F5");
    const auto posCtrlShiftF1 = exported.indexOf("_hotkey CTRL+SHIFT+F1");

    // Verify order is preserved exactly as imported (no auto-sorting)
    // Original order: CTRL+SHIFT+F1, F2, ALT+F3, F4, CTRL+F5
    QVERIFY(posCtrlShiftF1 < posF2);
    QVERIFY(posF2 < posAltF3);
    QVERIFY(posAltF3 < posF4);
    QVERIFY(posF4 < posCtrlF5);
}

QTEST_MAIN(TestHotkeyManager)
