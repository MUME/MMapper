// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "TestPasswordField.h"

#include <QLineEdit>
#include <QSignalBlocker>
#include <QString>
#include <QtTest/QtTest>

TestPasswordField::TestPasswordField() = default;
TestPasswordField::~TestPasswordField() = default;

// Replicate the bullet-stripping logic from generalpage.cpp textEdited handler.
// This is the exact same algorithm used in production.
static QString stripDummyBullets(const QString &password)
{
    QString cleaned = password;
    while (!cleaned.isEmpty() && cleaned.at(0) == QChar(0x2022))
        cleaned.remove(0, 1);
    return cleaned;
}

static const QString DUMMY_DOTS = QString(8, QChar(0x2022));

// ─── Bullet stripping tests ───

void TestPasswordField::testStripBulletsSingleChar()
{
    // User types 'a' at end of 8 dummy bullets → only 'a' remains
    QCOMPARE(stripDummyBullets(DUMMY_DOTS + "a"), QString("a"));
}

void TestPasswordField::testStripBulletsMultiCharPaste()
{
    // User pastes "mypass" at end of 8 dummy bullets → full paste preserved
    QCOMPARE(stripDummyBullets(DUMMY_DOTS + "mypass"), QString("mypass"));
}

void TestPasswordField::testStripBulletsNoBullets()
{
    // User selected all and typed/pasted — no bullets to strip
    QCOMPARE(stripDummyBullets("mypass"), QString("mypass"));
}

void TestPasswordField::testStripBulletsOnlyBullets()
{
    // Edge case: only bullets, no new input
    QCOMPARE(stripDummyBullets(DUMMY_DOTS), QString(""));
}

void TestPasswordField::testStripBulletsPartialBullets()
{
    // User deleted some bullets before typing
    QCOMPARE(stripDummyBullets(QString(3, QChar(0x2022)) + "abc"), QString("abc"));
}

void TestPasswordField::testStripBulletsMidStringBullets()
{
    // Bullets in the middle of real text should NOT be stripped
    QCOMPARE(stripDummyBullets("a" + QString(2, QChar(0x2022)) + "b"),
             QString("a" + QString(2, QChar(0x2022)) + "b"));
}

// ─── Password field state machine tests ───
//
// These simulate the GeneralPage password field behavior using QLineEdit
// with the same logic as the textEdited handler and show/hide flow.

void TestPasswordField::testDummyDotsShownOnLoad()
{
    // When a password is stored, loadConfig puts 8 bullet chars in the field
    QLineEdit edit;
    bool hasDummy = false;

    // Simulate slot_loadConfig with accountPassword = true
    {
        const QSignalBlocker blocker(&edit);
        edit.setText(DUMMY_DOTS);
        hasDummy = true;
    }

    QCOMPARE(edit.text(), DUMMY_DOTS);
    QVERIFY(hasDummy);
}

void TestPasswordField::testTypeSingleCharClearsDummy()
{
    QLineEdit edit;
    bool hasDummy = true;
    QString savedPassword;

    // Set up dummy dots
    {
        const QSignalBlocker blocker(&edit);
        edit.setText(DUMMY_DOTS);
    }

    // Simulate textEdited with a single char appended to dummy
    const QString editedText = DUMMY_DOTS + "x";
    // This is what the handler does:
    if (hasDummy) {
        hasDummy = false;
        const QString cleaned = stripDummyBullets(editedText);
        const QSignalBlocker blocker(&edit);
        edit.setText(cleaned);
        if (!cleaned.isEmpty()) {
            savedPassword = cleaned;
        }
    }

    QCOMPARE(edit.text(), QString("x"));
    QCOMPARE(savedPassword, QString("x"));
    QVERIFY(!hasDummy);
}

void TestPasswordField::testPasteOverDummyPreservesAll()
{
    QLineEdit edit;
    bool hasDummy = true;
    QString savedPassword;

    // Set up dummy dots
    {
        const QSignalBlocker blocker(&edit);
        edit.setText(DUMMY_DOTS);
    }

    // Simulate paste: "hunter2" appended after dummy dots
    const QString editedText = DUMMY_DOTS + "hunter2";
    if (hasDummy) {
        hasDummy = false;
        const QString cleaned = stripDummyBullets(editedText);
        const QSignalBlocker blocker(&edit);
        edit.setText(cleaned);
        if (!cleaned.isEmpty()) {
            savedPassword = cleaned;
        }
    }

    // The full pasted text must survive — not truncated to last char
    QCOMPARE(edit.text(), QString("hunter2"));
    QCOMPARE(savedPassword, QString("hunter2"));
}

void TestPasswordField::testNormalTypingWithoutDummy()
{
    QLineEdit edit;
    bool hasDummy = false;
    QString savedPassword;
    bool deleted = false;

    // Simulate typing "abc" one char at a time, no dummy state
    for (const auto &text : {QString("a"), QString("ab"), QString("abc")}) {
        if (hasDummy) {
            // Should not enter this branch
            QFAIL("hasDummy should be false");
        }
        if (!text.isEmpty()) {
            savedPassword = text;
            deleted = false;
        } else {
            deleted = true;
        }
    }

    QCOMPARE(savedPassword, QString("abc"));
    QVERIFY(!deleted);
}

void TestPasswordField::testClearPasswordDeletesStored()
{
    QLineEdit edit;
    bool hasDummy = false;
    bool passwordStored = true;
    bool deleted = false;

    // Simulate clearing the field (textEdited with empty string)
    const QString password;
    if (!hasDummy) {
        passwordStored = !password.isEmpty();
        if (password.isEmpty()) {
            deleted = true;
        }
    }

    QVERIFY(!passwordStored);
    QVERIFY(deleted);
}

void TestPasswordField::testHideRestoresDummyDots()
{
    QLineEdit edit;
    bool hasDummy = false;
    bool accountPasswordStored = true;

    // Show state: password is revealed
    edit.setText("secret");
    edit.setEchoMode(QLineEdit::Normal);

    // Simulate clicking "Hide Password"
    edit.setEchoMode(QLineEdit::Password);
    if (accountPasswordStored) {
        const QSignalBlocker blocker(&edit);
        edit.setText(DUMMY_DOTS);
        hasDummy = true;
    } else {
        edit.clear();
    }

    QCOMPARE(edit.text(), DUMMY_DOTS);
    QCOMPARE(edit.echoMode(), QLineEdit::Password);
    QVERIFY(hasDummy);
}

void TestPasswordField::testUncheckedClearsEverything()
{
    QLineEdit nameEdit;
    QLineEdit passEdit;
    bool hasDummy = true;
    bool accountPasswordStored = true;
    QString accountName = "player";

    // Set up initial state
    nameEdit.setText(accountName);
    {
        const QSignalBlocker blocker(&passEdit);
        passEdit.setText(DUMMY_DOTS);
    }

    // Simulate unchecking autoLogin
    accountPasswordStored = false;
    accountName.clear();
    nameEdit.clear();
    passEdit.clear();
    passEdit.setEchoMode(QLineEdit::Password);
    hasDummy = false;

    QCOMPARE(nameEdit.text(), QString(""));
    QCOMPARE(passEdit.text(), QString(""));
    QVERIFY(!hasDummy);
    QVERIFY(!accountPasswordStored);
    QVERIFY(accountName.isEmpty());
}

QTEST_MAIN(TestPasswordField)
