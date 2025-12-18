#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"
#include "../global/macros.h"
#include "PaletteManager.h"

#include <iterator>
#include <list>

#include <QEvent>
#include <QObject>
#include <QPlainTextEdit>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QtCore>

class QKeyEvent;
class QObject;
class QWidget;

// Key classification system for unified key handling
enum class KeyType {
    FunctionKey,      // F1-F12
    NumpadKey,        // NUMPAD0-9, NUMPAD_SLASH, etc.
    NavigationKey,    // HOME, END, INSERT
    ArrowKey,         // UP, DOWN (for history), LEFT, RIGHT (for hotkeys)
    MiscKey,          // ACCENT, number row, HYPHEN, EQUAL
    TerminalShortcut, // Ctrl+U, Ctrl+W, Ctrl+H
    BasicKey,         // Enter, Tab (no modifiers)
    PageKey,          // PageUp, PageDown (for scrolling display)
    Other             // Not handled by us
};

struct NODISCARD KeyClassification
{
    KeyType type = KeyType::Other;
    QString keyName;
    Qt::KeyboardModifiers realModifiers = Qt::NoModifier;
    bool shouldHandle = false;
};

class NODISCARD InputHistory final : private std::list<QString>
{
private:
    std::list<QString>::iterator m_iterator;

public:
    InputHistory() { m_iterator = begin(); }

public:
    void addInputLine(const QString &);

public:
    void forward() { std::advance(m_iterator, 1); }
    void backward() { std::advance(m_iterator, -1); }

public:
    NODISCARD const QString &value() const { return *m_iterator; }

public:
    NODISCARD bool atFront() const { return m_iterator == begin(); }
    NODISCARD bool atEnd() const { return m_iterator == end(); }
};

class NODISCARD TabHistory final : private std::list<QString>
{
    using base = std::list<QString>;

private:
    std::list<QString>::iterator m_iterator;

public:
    TabHistory() { m_iterator = begin(); }

public:
    void addInputLine(const QString &);

public:
    void forward() { std::advance(m_iterator, 1); }
    void reset() { m_iterator = begin(); }

public:
    NODISCARD const QString &value() const { return *m_iterator; }

public:
    NODISCARD bool empty() { return base::empty(); }
    NODISCARD bool atEnd() const { return m_iterator == end(); }
};

struct NODISCARD InputWidgetOutputs
{
public:
    explicit InputWidgetOutputs() = default;
    virtual ~InputWidgetOutputs();
    DELETE_CTORS_AND_ASSIGN_OPS(InputWidgetOutputs);

public:
    void sendUserInput(const QString &msg) { virt_sendUserInput(msg); }
    void displayMessage(const QString &msg) { virt_displayMessage(msg); }
    void showMessage(const QString &msg, const int timeout) { virt_showMessage(msg, timeout); }
    void gotPasswordInput(const QString &password) { virt_gotPasswordInput(password); }
    void scrollDisplay(bool pageUp) { virt_scrollDisplay(pageUp); }

private:
    virtual void virt_sendUserInput(const QString &msg) = 0;
    virtual void virt_displayMessage(const QString &msg) = 0;
    virtual void virt_showMessage(const QString &msg, int timeout) = 0;
    virtual void virt_gotPasswordInput(const QString &password) = 0;
    virtual void virt_scrollDisplay(bool pageUp) = 0;
};

class NODISCARD_QOBJECT InputWidget final : public QPlainTextEdit
{
    Q_OBJECT

private:
    using base = QPlainTextEdit;

private:
    InputWidgetOutputs &m_outputs;
    QString m_tabFragment;
    TabHistory m_tabHistory;
    InputHistory m_inputHistory;
    PaletteManager m_paletteManager;
    bool m_tabbing = false;
    bool m_handledInShortcutOverride = false; // Track if key was already handled in ShortcutOverride

public:
    explicit InputWidget(QWidget *parent, InputWidgetOutputs &);
    ~InputWidget() final;

    NODISCARD QSize sizeHint() const override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;

private:
    void gotInput();
    NODISCARD bool tryHistory(int);
    NODISCARD bool numpadKeyPressed(int key, Qt::KeyboardModifiers modifiers);
    NODISCARD bool navigationKeyPressed(int key, Qt::KeyboardModifiers modifiers);
    NODISCARD bool arrowKeyPressed(int key, Qt::KeyboardModifiers modifiers);
    NODISCARD bool miscKeyPressed(int key, Qt::KeyboardModifiers modifiers);
    void functionKeyPressed(int key, Qt::KeyboardModifiers modifiers);
    NODISCARD QString buildHotkeyString(const QString &keyName, Qt::KeyboardModifiers modifiers);
    NODISCARD bool handleTerminalShortcut(int key);
    NODISCARD bool handleBasicKey(int key);
    NODISCARD bool handlePageKey(int key, Qt::KeyboardModifiers modifiers);

private:
    void tabComplete();

private:
    void forwardHistory();
    void backwardHistory();

private:
    void sendUserInput(const QString &msg) { m_outputs.sendUserInput(msg); }
    void sendCommandWithSeparator(const QString &command);
};
