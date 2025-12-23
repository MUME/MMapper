// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "TestHotkeyManager.h"

#include "../src/configuration/HotkeyManager.h"

#include <QCoreApplication>
#include <QSettings>
#include <QStringList>
#include <QtTest/QtTest>

TestHotkeyManager::TestHotkeyManager() = default;
TestHotkeyManager::~TestHotkeyManager() = default;

void TestHotkeyManager::initTestCase()
{
    // Save original QSettings namespace to avoid polluting real user settings
    m_originalOrganization = QCoreApplication::organizationName();
    m_originalApplication = QCoreApplication::applicationName();

    // Use test-specific namespace for isolation
    QCoreApplication::setOrganizationName(QStringLiteral("MMapperTest"));
    QCoreApplication::setApplicationName(QStringLiteral("HotkeyManagerTest"));
}

void TestHotkeyManager::cleanupTestCase()
{
    // Restore original QSettings namespace
    QCoreApplication::setOrganizationName(m_originalOrganization);
    QCoreApplication::setApplicationName(m_originalApplication);
}

void TestHotkeyManager::keyNormalizationTest()
{
    HotkeyManager manager;

    // Test that modifiers are normalized to canonical order: CTRL+SHIFT+ALT+META
    // Set a hotkey with non-canonical modifier order
    QVERIFY(manager.setHotkey("ALT+CTRL+F1", "test1"));

    // Should be retrievable with canonical order
    QCOMPARE(manager.getCommandQString("CTRL+ALT+F1"), QString("test1"));

    // Should also be retrievable with the original order (due to normalization)
    QCOMPARE(manager.getCommandQString("ALT+CTRL+F1"), QString("test1"));

    // Test all modifier combinations normalize correctly
    QVERIFY(manager.setHotkey("META+ALT+SHIFT+CTRL+F2", "test2"));
    QCOMPARE(manager.getCommandQString("CTRL+SHIFT+ALT+META+F2"), QString("test2"));

    // Test that case is normalized to uppercase
    QVERIFY(manager.setHotkey("ctrl+f3", "test3"));
    QCOMPARE(manager.getCommandQString("CTRL+F3"), QString("test3"));

    // Test CONTROL alias normalizes to CTRL
    QVERIFY(manager.setHotkey("CONTROL+F4", "test4"));
    QCOMPARE(manager.getCommandQString("CTRL+F4"), QString("test4"));

    // Test CMD/COMMAND aliases normalize to META
    QVERIFY(manager.setHotkey("CMD+F5", "test5"));
    QCOMPARE(manager.getCommandQString("META+F5"), QString("test5"));

    QVERIFY(manager.setHotkey("COMMAND+F6", "test6"));
    QCOMPARE(manager.getCommandQString("META+F6"), QString("test6"));

    // Test simple key without modifiers
    QVERIFY(manager.setHotkey("f7", "test7"));
    QCOMPARE(manager.getCommandQString("F7"), QString("test7"));

    // Test numpad keys
    QVERIFY(manager.setHotkey("numpad8", "north"));
    QCOMPARE(manager.getCommandQString("NUMPAD8"), QString("north"));
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
    QCOMPARE(manager.getCommandQString("F1"), QString("look"));
    QCOMPARE(manager.getCommandQString("CTRL+F2"), QString("open exit n"));
    QCOMPARE(manager.getCommandQString("SHIFT+ALT+F3"), QString("pick exit s"));
    QCOMPARE(manager.getCommandQString("NUMPAD8"), QString("n"));
    QCOMPARE(manager.getCommandQString("CTRL+SHIFT+NUMPAD_PLUS"), QString("test command"));

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
    QCOMPARE(manager.getCommandQString("F10"), QString("flee"));
    QCOMPARE(manager.getCommandQString("F11"), QString("rest"));

    // Verify import cleared existing hotkeys
    QCOMPARE(manager.getAllHotkeys().size(), 2);
    QCOMPARE(manager.getCommandQString("F1"), QString()); // Should be cleared

    // Test another import clears and replaces
    manager.importFromCliFormat("_hotkey F12 stand\n");
    QCOMPARE(manager.getAllHotkeys().size(), 1);
    QCOMPARE(manager.getCommandQString("F10"), QString()); // Should be cleared
    QCOMPARE(manager.getCommandQString("F12"), QString("stand"));
}

void TestHotkeyManager::importEdgeCasesTest()
{
    HotkeyManager manager;

    // Test command with multiple spaces (should preserve spaces in command)
    manager.importFromCliFormat("_hotkey F1 cast 'cure light'");
    QCOMPARE(manager.getCommandQString("F1"), QString("cast 'cure light'"));

    // Test malformed lines are skipped
    // "_hotkey" alone - no key
    // "_hotkey F2" - no command
    // "_hotkey F3 valid" - valid
    manager.importFromCliFormat("_hotkey\n_hotkey F2\n_hotkey F3 valid");
    QCOMPARE(manager.getAllHotkeys().size(), 1);
    QCOMPARE(manager.getCommandQString("F3"), QString("valid"));

    // Test leading/trailing whitespace handling
    manager.importFromCliFormat("  _hotkey   F4   command with spaces  ");
    QCOMPARE(manager.getCommandQString("F4"), QString("command with spaces"));

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
    QCOMPARE(manager.getCommandQString("F1"), QString("custom"));
    QCOMPARE(manager.getAllHotkeys().size(), 2);

    // Reset to defaults
    manager.resetToDefaults();

    // Verify defaults are restored
    QCOMPARE(manager.getCommandQString("NUMPAD8"), QString("n"));
    QCOMPARE(manager.getCommandQString("NUMPAD4"), QString("w"));
    QCOMPARE(manager.getCommandQString("CTRL+NUMPAD8"), QString("open exit n"));
    QCOMPARE(manager.getCommandQString("ALT+NUMPAD8"), QString("close exit n"));
    QCOMPARE(manager.getCommandQString("SHIFT+NUMPAD8"), QString("pick exit n"));

    // F1 is not in defaults, should be empty
    QCOMPARE(manager.getCommandQString("F1"), QString());

    // Verify defaults are non-empty (don't assert exact count to avoid brittleness)
    QVERIFY(!manager.getAllHotkeys().empty());
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

void TestHotkeyManager::setHotkeyTest()
{
    HotkeyManager manager;

    // Clear any existing hotkeys
    manager.importFromCliFormat("");
    QCOMPARE(manager.getAllHotkeys().size(), 0);

    // Test setting a new hotkey directly
    QVERIFY(manager.setHotkey("F1", "look"));
    QCOMPARE(manager.getCommandQString("F1"), QString("look"));
    QCOMPARE(manager.getAllHotkeys().size(), 1);

    // Test setting another hotkey
    QVERIFY(manager.setHotkey("F2", "flee"));
    QCOMPARE(manager.getCommandQString("F2"), QString("flee"));
    QCOMPARE(manager.getAllHotkeys().size(), 2);

    // Test updating an existing hotkey (should replace, not add)
    QVERIFY(manager.setHotkey("F1", "inventory"));
    QCOMPARE(manager.getCommandQString("F1"), QString("inventory"));
    QCOMPARE(manager.getAllHotkeys().size(), 2); // Still 2, not 3

    // Test setting hotkey with modifiers
    QVERIFY(manager.setHotkey("CTRL+F3", "open exit n"));
    QCOMPARE(manager.getCommandQString("CTRL+F3"), QString("open exit n"));
    QCOMPARE(manager.getAllHotkeys().size(), 3);

    // Test updating hotkey with modifiers
    QVERIFY(manager.setHotkey("CTRL+F3", "close exit n"));
    QCOMPARE(manager.getCommandQString("CTRL+F3"), QString("close exit n"));
    QCOMPARE(manager.getAllHotkeys().size(), 3); // Still 3

    // Test that export contains the updated values
    QString exported = manager.exportToCliFormat();
    QVERIFY(exported.contains("_hotkey F1 inventory"));
    QVERIFY(exported.contains("_hotkey F2 flee"));
    QVERIFY(exported.contains("_hotkey CTRL+F3 close exit n"));
    QVERIFY(!exported.contains("_hotkey F1 look")); // Old value should not be present
}

void TestHotkeyManager::removeHotkeyTest()
{
    HotkeyManager manager;

    // Setup: import some hotkeys
    manager.importFromCliFormat("_hotkey F1 look\n_hotkey F2 flee\n_hotkey CTRL+F3 open exit n\n");
    QCOMPARE(manager.getAllHotkeys().size(), 3);

    // Test removing a hotkey
    manager.removeHotkey("F1");
    QCOMPARE(manager.getCommandQString("F1"), QString()); // Should be empty now
    QCOMPARE(manager.getAllHotkeys().size(), 2);

    // Verify other hotkeys still exist
    QCOMPARE(manager.getCommandQString("F2"), QString("flee"));
    QCOMPARE(manager.getCommandQString("CTRL+F3"), QString("open exit n"));

    // Test removing hotkey with modifiers
    manager.removeHotkey("CTRL+F3");
    QCOMPARE(manager.getCommandQString("CTRL+F3"), QString());
    QCOMPARE(manager.getAllHotkeys().size(), 1);

    // Test removing non-existent hotkey (should not crash or change count)
    manager.removeHotkey("F10");
    QCOMPARE(manager.getAllHotkeys().size(), 1);

    // Test removing with non-canonical modifier order (should still work due to normalization)
    manager.importFromCliFormat("_hotkey ALT+CTRL+F5 test\n");
    QCOMPARE(manager.getAllHotkeys().size(), 1);
    manager.removeHotkey("CTRL+ALT+F5"); // Canonical order
    QCOMPARE(manager.getAllHotkeys().size(), 0);

    // Test that export reflects removal
    manager.importFromCliFormat("_hotkey F1 look\n_hotkey F2 flee\n");
    manager.removeHotkey("F1");
    QString exported = manager.exportToCliFormat();
    QVERIFY(!exported.contains("_hotkey F1"));
    QVERIFY(exported.contains("_hotkey F2 flee"));
}

void TestHotkeyManager::hasHotkeyTest()
{
    HotkeyManager manager;

    // Clear and setup
    manager.importFromCliFormat("_hotkey F1 look\n_hotkey CTRL+F2 flee\n");

    // Test hasHotkey returns true for existing keys
    QVERIFY(manager.hasHotkey("F1"));
    QVERIFY(manager.hasHotkey("CTRL+F2"));

    // Test hasHotkey returns false for non-existent keys
    QVERIFY(!manager.hasHotkey("F3"));
    QVERIFY(!manager.hasHotkey("CTRL+F1"));
    QVERIFY(!manager.hasHotkey("ALT+F2"));

    // Test hasHotkey works with non-canonical modifier order
    QVERIFY(manager.hasHotkey("CTRL+F2"));

    // Test case insensitivity
    QVERIFY(manager.hasHotkey("f1"));
    QVERIFY(manager.hasHotkey("ctrl+f2"));

    // Test after removal
    manager.removeHotkey("F1");
    QVERIFY(!manager.hasHotkey("F1"));
    QVERIFY(manager.hasHotkey("CTRL+F2")); // Other key still exists
}

void TestHotkeyManager::invalidKeyValidationTest()
{
    HotkeyManager manager;

    // Clear any existing hotkeys
    manager.importFromCliFormat("");
    QCOMPARE(manager.getAllHotkeys().size(), 0);

    // Test that invalid base keys are rejected
    QVERIFY(!manager.setHotkey("F13", "invalid"));
    QCOMPARE(manager.getCommandQString("F13"), QString()); // Should not be set
    QCOMPARE(manager.getAllHotkeys().size(), 0);

    // Test typo in key name
    QVERIFY(!manager.setHotkey("NUMPDA8", "typo")); // Typo: NUMPDA instead of NUMPAD
    QCOMPARE(manager.getCommandQString("NUMPDA8"), QString());
    QCOMPARE(manager.getAllHotkeys().size(), 0);

    // Test completely invalid key
    QVERIFY(!manager.setHotkey("INVALID", "test"));
    QCOMPARE(manager.getCommandQString("INVALID"), QString());
    QCOMPARE(manager.getAllHotkeys().size(), 0);

    // Test that valid keys still work
    QVERIFY(manager.setHotkey("F12", "valid"));
    QCOMPARE(manager.getCommandQString("F12"), QString("valid"));
    QCOMPARE(manager.getAllHotkeys().size(), 1);

    // Test invalid key with valid modifiers
    QVERIFY(!manager.setHotkey("CTRL+F13", "invalid"));
    QCOMPARE(manager.getCommandQString("CTRL+F13"), QString());
    QCOMPARE(manager.getAllHotkeys().size(), 1); // Still just F12

    // Test import also rejects invalid keys
    manager.importFromCliFormat("_hotkey F1 valid\n_hotkey F13 invalid\n_hotkey NUMPAD8 valid2\n");
    QCOMPARE(manager.getAllHotkeys().size(), 2); // Only F1 and NUMPAD8
    QCOMPARE(manager.getCommandQString("F1"), QString("valid"));
    QCOMPARE(manager.getCommandQString("NUMPAD8"), QString("valid2"));
    QCOMPARE(manager.getCommandQString("F13"), QString()); // Not imported

    // Test all valid key categories work
    manager.importFromCliFormat("");

    // Function keys
    QVERIFY(manager.setHotkey("F1", "test"));
    QVERIFY(manager.hasHotkey("F1"));

    // Numpad
    QVERIFY(manager.setHotkey("NUMPAD5", "test"));
    QVERIFY(manager.hasHotkey("NUMPAD5"));

    // Navigation
    QVERIFY(manager.setHotkey("HOME", "test"));
    QVERIFY(manager.hasHotkey("HOME"));

    // Arrow keys
    QVERIFY(manager.setHotkey("UP", "test"));
    QVERIFY(manager.hasHotkey("UP"));

    // Misc
    QVERIFY(manager.setHotkey("ACCENT", "test"));
    QVERIFY(manager.hasHotkey("ACCENT"));

    QVERIFY(manager.setHotkey("0", "test"));
    QVERIFY(manager.hasHotkey("0"));

    QVERIFY(manager.setHotkey("HYPHEN", "test"));
    QVERIFY(manager.hasHotkey("HYPHEN"));
}

void TestHotkeyManager::duplicateKeyBehaviorTest()
{
    HotkeyManager manager;

    // Test that duplicate keys in imported content use the last definition
    QString contentWithDuplicates = "_hotkey F1 first\n"
                                    "_hotkey F2 middle\n"
                                    "_hotkey F1 second\n";

    manager.importFromCliFormat(contentWithDuplicates);

    // getCommand should return the last definition
    QCOMPARE(manager.getCommandQString("F1"), QString("second"));
    QCOMPARE(manager.getCommandQString("F2"), QString("middle"));

    // Test that setHotkey replaces existing entry
    manager.importFromCliFormat("_hotkey F1 original\n");
    QCOMPARE(manager.getCommandQString("F1"), QString("original"));
    QCOMPARE(manager.getAllHotkeys().size(), 1);

    QVERIFY(manager.setHotkey("F1", "replaced"));
    QCOMPARE(manager.getCommandQString("F1"), QString("replaced"));
    QCOMPARE(manager.getAllHotkeys().size(), 1); // Still 1, not 2
}

void TestHotkeyManager::commentPreservationTest()
{
    HotkeyManager manager;

    // Test that comments and formatting survive import/export round trip
    const QString cliFormat = "# Leading comment\n"
                              "\n"
                              "# Section header\n"
                              "_hotkey F1 open\n"
                              "\n"
                              "# Another comment\n"
                              "_hotkey F2 close\n";

    manager.importFromCliFormat(cliFormat);
    const QString exported = manager.exportToCliFormat();

    // Verify comments are preserved in export
    QVERIFY(exported.contains("# Leading comment"));
    QVERIFY(exported.contains("# Section header"));
    QVERIFY(exported.contains("# Another comment"));

    // Verify hotkeys are still correct
    QVERIFY(exported.contains("_hotkey F1 open"));
    QVERIFY(exported.contains("_hotkey F2 close"));

    // Verify order is preserved (comments before their hotkeys)
    const auto posLeading = exported.indexOf("# Leading comment");
    const auto posSection = exported.indexOf("# Section header");
    const auto posF1 = exported.indexOf("_hotkey F1");
    const auto posAnother = exported.indexOf("# Another comment");
    const auto posF2 = exported.indexOf("_hotkey F2");

    QVERIFY(posLeading < posSection);
    QVERIFY(posSection < posF1);
    QVERIFY(posF1 < posAnother);
    QVERIFY(posAnother < posF2);
}

void TestHotkeyManager::settingsPersistenceTest()
{
    // Test that the HotkeyManager constructor loads settings and
    // that saveToSettings() can be called without error.
    // Note: Full persistence testing would require dependency injection
    // of QSettings, which is beyond the scope of this test.

    HotkeyManager manager;

    // Manager should have loaded something (either defaults or saved settings)
    // Just verify it's in a valid state
    QVERIFY(!manager.exportToCliFormat().isEmpty());

    // Import custom hotkeys
    manager.importFromCliFormat("# Persistence test\n"
                                "_hotkey F1 testcmd\n");

    QCOMPARE(manager.getCommandQString("F1"), QString("testcmd"));

    // Verify saveToSettings() doesn't crash
    manager.saveToSettings();

    // Verify state is still valid after save
    QCOMPARE(manager.getCommandQString("F1"), QString("testcmd"));
    QVERIFY(manager.exportToCliFormat().contains("# Persistence test"));
}

void TestHotkeyManager::directLookupTest()
{
    HotkeyManager manager;

    // Import hotkeys for testing
    manager.importFromCliFormat("_hotkey F1 look\n"
                                "_hotkey CTRL+F2 flee\n"
                                "_hotkey NUMPAD8 n\n"
                                "_hotkey CTRL+NUMPAD5 s\n"
                                "_hotkey SHIFT+ALT+UP north\n");

    // Test direct lookup for function keys (isNumpad=false)
    QCOMPARE(manager.getCommandQString(Qt::Key_F1, Qt::NoModifier, false), QString("look"));
    QCOMPARE(manager.getCommandQString(Qt::Key_F2, Qt::ControlModifier, false), QString("flee"));

    // Test that wrong modifiers don't match
    QCOMPARE(manager.getCommandQString(Qt::Key_F1, Qt::ControlModifier, false), QString());
    QCOMPARE(manager.getCommandQString(Qt::Key_F2, Qt::NoModifier, false), QString());

    // Test numpad keys (isNumpad=true) - Qt::Key_8 with isNumpad=true
    QCOMPARE(manager.getCommandQString(Qt::Key_8, Qt::NoModifier, true), QString("n"));
    QCOMPARE(manager.getCommandQString(Qt::Key_5, Qt::ControlModifier, true), QString("s"));

    // Test that numpad keys don't match non-numpad lookups
    QCOMPARE(manager.getCommandQString(Qt::Key_8, Qt::NoModifier, false), QString());

    // Test arrow keys (isNumpad=false)
    QCOMPARE(manager.getCommandQString(Qt::Key_Up, Qt::ShiftModifier | Qt::AltModifier, false),
             QString("north"));

    // Test that order of modifiers doesn't matter for lookup
    QCOMPARE(manager.getCommandQString(Qt::Key_Up, Qt::AltModifier | Qt::ShiftModifier, false),
             QString("north"));

    // Test non-existent hotkey
    QCOMPARE(manager.getCommandQString(Qt::Key_F12, Qt::NoModifier, false), QString());
}

QTEST_MAIN(TestHotkeyManager)
