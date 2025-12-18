// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "inputwidget.h"

#include "../configuration/HotkeyManager.h"
#include "../configuration/configuration.h"
#include "../global/Color.h"

#include <QFont>
#include <QMessageLogContext>
#include <QRegularExpression>
#include <QSize>
#include <QString>
#include <QTextStream>
#include <QtGui>
#include <QtWidgets>

static constexpr const int MIN_WORD_LENGTH = 3;

static const QRegularExpression g_whitespaceRx(R"(\s+)");

// Lookup tables for key name mapping (reduces cyclomatic complexity)
static const QHash<int, QString> &getNumpadKeyMap()
{
    static const QHash<int, QString> map{{Qt::Key_0, "NUMPAD0"},
                                         {Qt::Key_1, "NUMPAD1"},
                                         {Qt::Key_2, "NUMPAD2"},
                                         {Qt::Key_3, "NUMPAD3"},
                                         {Qt::Key_4, "NUMPAD4"},
                                         {Qt::Key_5, "NUMPAD5"},
                                         {Qt::Key_6, "NUMPAD6"},
                                         {Qt::Key_7, "NUMPAD7"},
                                         {Qt::Key_8, "NUMPAD8"},
                                         {Qt::Key_9, "NUMPAD9"},
                                         {Qt::Key_Slash, "NUMPAD_SLASH"},
                                         {Qt::Key_Asterisk, "NUMPAD_ASTERISK"},
                                         {Qt::Key_Minus, "NUMPAD_MINUS"},
                                         {Qt::Key_Plus, "NUMPAD_PLUS"},
                                         {Qt::Key_Period, "NUMPAD_PERIOD"}};
    return map;
}

static QString getNumpadKeyName(int key)
{
    return getNumpadKeyMap().value(key);
}

static const QHash<int, QString> &getNavigationKeyMap()
{
    static const QHash<int, QString> map{{Qt::Key_Home, "HOME"},
                                         {Qt::Key_End, "END"},
                                         {Qt::Key_Insert, "INSERT"},
                                         {Qt::Key_Help, "INSERT"}}; // macOS maps Insert to Help
    return map;
}

static QString getNavigationKeyName(int key)
{
    return getNavigationKeyMap().value(key);
}

static const QHash<int, QString> &getMiscKeyMap()
{
    static const QHash<int, QString> map{{Qt::Key_QuoteLeft, "ACCENT"},
                                         {Qt::Key_1, "1"},
                                         {Qt::Key_2, "2"},
                                         {Qt::Key_3, "3"},
                                         {Qt::Key_4, "4"},
                                         {Qt::Key_5, "5"},
                                         {Qt::Key_6, "6"},
                                         {Qt::Key_7, "7"},
                                         {Qt::Key_8, "8"},
                                         {Qt::Key_9, "9"},
                                         {Qt::Key_0, "0"},
                                         {Qt::Key_Minus, "HYPHEN"},
                                         {Qt::Key_Equal, "EQUAL"}};
    return map;
}

static QString getMiscKeyName(int key)
{
    return getMiscKeyMap().value(key);
}

static KeyClassification classifyKey(int key, Qt::KeyboardModifiers mods)
{
    KeyClassification result;
    result.realModifiers = mods & ~Qt::KeypadModifier;

    // Function keys F1-F12 (always handled)
    if (key >= Qt::Key_F1 && key <= Qt::Key_F12) {
        result.type = KeyType::FunctionKey;
        result.keyName = QString("F%1").arg(key - Qt::Key_F1 + 1);
        result.shouldHandle = true;
        return result;
    }

    // Numpad keys (only with KeypadModifier)
    if (mods & Qt::KeypadModifier) {
        QString name = getNumpadKeyName(key);
        if (!name.isEmpty()) {
            result.type = KeyType::NumpadKey;
            result.keyName = name;
            result.shouldHandle = true;
            return result;
        }
    }

    // Navigation keys (HOME, END, INSERT - from any source)
    {
        QString name = getNavigationKeyName(key);
        if (!name.isEmpty()) {
            result.type = KeyType::NavigationKey;
            result.keyName = name;
            result.shouldHandle = true;
            return result;
        }
    }

    // Arrow keys (UP, DOWN, LEFT, RIGHT)
    if (key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_Left || key == Qt::Key_Right) {
        result.type = KeyType::ArrowKey;
        switch (key) {
        case Qt::Key_Up:
            result.keyName = "UP";
            break;
        case Qt::Key_Down:
            result.keyName = "DOWN";
            break;
        case Qt::Key_Left:
            result.keyName = "LEFT";
            break;
        case Qt::Key_Right:
            result.keyName = "RIGHT";
            break;
        }
        result.shouldHandle = true;
        return result;
    }

    // Misc keys (only when NOT from numpad)
    if (!(mods & Qt::KeypadModifier)) {
        QString name = getMiscKeyName(key);
        if (!name.isEmpty()) {
            result.type = KeyType::MiscKey;
            result.keyName = name;
            result.shouldHandle = true;
            return result;
        }
    }

    // Terminal shortcuts (Ctrl+U, Ctrl+W, Ctrl+H or Cmd+U, Cmd+W, Cmd+H)
    if ((key == Qt::Key_U || key == Qt::Key_W || key == Qt::Key_H)
        && (result.realModifiers == Qt::ControlModifier
            || result.realModifiers == Qt::MetaModifier)) {
        result.type = KeyType::TerminalShortcut;
        result.shouldHandle = true;
        return result;
    }

    // Basic keys (Tab, Enter - only without modifiers)
    if ((key == Qt::Key_Tab || key == Qt::Key_Return || key == Qt::Key_Enter)
        && result.realModifiers == Qt::NoModifier) {
        result.type = KeyType::BasicKey;
        result.shouldHandle = true;
        return result;
    }

    // Page keys (PageUp, PageDown - for scrolling display)
    if (key == Qt::Key_PageUp || key == Qt::Key_PageDown) {
        result.type = KeyType::PageKey;
        result.keyName = (key == Qt::Key_PageUp) ? "PAGEUP" : "PAGEDOWN";
        result.shouldHandle = true;
        return result;
    }

    return result;
}

InputWidgetOutputs::~InputWidgetOutputs() = default;

InputWidget::InputWidget(QWidget *const parent, InputWidgetOutputs &outputs)
    : QPlainTextEdit(parent)
    , m_outputs{outputs}
{
    // Size Policy
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    // Terminal Font
    QFont font;
    font.fromString(getConfig().integratedClient.font);
    setFont(font);

    // Minimum Size
    QFontMetrics fm(font);
    setMinimumSize(
        QSize(fm.averageCharWidth(),
              fm.lineSpacing()
                  + (static_cast<int>(document()->documentMargin()) + QFrame::frameWidth()) * 2
                  + contentsMargins().top() + contentsMargins().bottom()));
    setSizeIncrement(fm.averageCharWidth(), fm.lineSpacing());

    // Line Wrapping
    setLineWrapMode(QPlainTextEdit::NoWrap);

    // Remember native palletes
    m_paletteManager.init(*this, std::nullopt, Qt::lightGray);
}

QSize InputWidget::sizeHint() const
{
    return minimumSize();
}

InputWidget::~InputWidget() = default;

void InputWidget::keyPressEvent(QKeyEvent *const event)
{
    // Check if this key was already handled in ShortcutOverride
    if (m_handledInShortcutOverride) {
        m_handledInShortcutOverride = false; // Reset for next key
        event->accept();
        return;
    }

    const auto key = event->key();
    const auto mods = event->modifiers();

    // Handle tabbing state (unchanged)
    if (m_tabbing) {
        if (key != Qt::Key_Tab) {
            m_tabbing = false;
        }

        // If Backspace or Escape is pressed, reject the completion
        QTextCursor current = textCursor();
        if (key == Qt::Key_Backspace || key == Qt::Key_Escape) {
            current.removeSelectedText();
            event->accept();
            return;
        }

        // For any other key press, accept the completion
        if (key != Qt::Key_Tab) {
            current.movePosition(QTextCursor::Right,
                                 QTextCursor::MoveAnchor,
                                 static_cast<int>(current.selectedText().length()));
            setTextCursor(current);
        }
    }

    // Classify the key ONCE
    auto classification = classifyKey(key, mods);

    if (classification.shouldHandle) {
        switch (classification.type) {
        case KeyType::FunctionKey:
            functionKeyPressed(key, classification.realModifiers);
            event->accept();
            return;

        case KeyType::NumpadKey:
            if (numpadKeyPressed(key, classification.realModifiers)) {
                event->accept();
                return;
            }
            break;

        case KeyType::NavigationKey:
            if (navigationKeyPressed(key, classification.realModifiers)) {
                event->accept();
                return;
            }
            break;

        case KeyType::ArrowKey:
            if (arrowKeyPressed(key, classification.realModifiers)) {
                event->accept();
                return;
            }
            break;

        case KeyType::MiscKey:
            if (miscKeyPressed(key, classification.realModifiers)) {
                event->accept();
                return;
            }
            break;

        case KeyType::TerminalShortcut:
            if (handleTerminalShortcut(key)) {
                event->accept();
                return;
            }
            break;

        case KeyType::BasicKey:
            if (handleBasicKey(key)) {
                event->accept();
                return;
            }
            break;

        case KeyType::PageKey:
            if (handlePageKey(key, classification.realModifiers)) {
                event->accept();
                return;
            }
            break;

        case KeyType::Other:
            break;
        }
    }

    // All other input
    base::keyPressEvent(event);
}

void InputWidget::functionKeyPressed(int key, Qt::KeyboardModifiers modifiers)
{
    // Check if there's a configured hotkey for this key combination
    // Function keys are never numpad keys
    const QString command = getConfig().hotkeyManager.getCommand(key, modifiers, false);

    if (!command.isEmpty()) {
        sendCommandWithSeparator(command);
    } else {
        // No hotkey configured, send the key name as-is (e.g., "CTRL+F1")
        QString keyName = QString("F%1").arg(key - Qt::Key_F1 + 1);
        QString fullKeyString = buildHotkeyString(keyName, modifiers);
        sendCommandWithSeparator(fullKeyString);
    }
}

bool InputWidget::numpadKeyPressed(int key, Qt::KeyboardModifiers modifiers)
{
    // Check if there's a configured hotkey for this numpad key (isNumpad=true)
    const QString command = getConfig().hotkeyManager.getCommand(key, modifiers, true);

    if (!command.isEmpty()) {
        sendCommandWithSeparator(command);
        return true;
    }
    return false;
}

bool InputWidget::navigationKeyPressed(int key, Qt::KeyboardModifiers modifiers)
{
    // Check if there's a configured hotkey for this navigation key (isNumpad=false)
    const QString command = getConfig().hotkeyManager.getCommand(key, modifiers, false);

    if (!command.isEmpty()) {
        sendCommandWithSeparator(command);
        return true;
    } else {
        return false;
    }
}

QString InputWidget::buildHotkeyString(const QString &keyName, Qt::KeyboardModifiers modifiers)
{
    QStringList parts;

    if (modifiers & Qt::ControlModifier) {
        parts << "CTRL";
    }
    if (modifiers & Qt::ShiftModifier) {
        parts << "SHIFT";
    }
    if (modifiers & Qt::AltModifier) {
        parts << "ALT";
    }
    if (modifiers & Qt::MetaModifier) {
        parts << "META";
    }

    parts << keyName;
    return parts.join("+");
}

bool InputWidget::arrowKeyPressed(const int key, Qt::KeyboardModifiers modifiers)
{
    // UP/DOWN with no modifiers cycle through command history
    if (modifiers == Qt::NoModifier) {
        if (key == Qt::Key_Up) {
            backwardHistory();
            return true;
        } else if (key == Qt::Key_Down) {
            forwardHistory();
            return true;
        }
    }

    // Arrow keys with modifiers check for hotkeys (isNumpad=false)
    const QString command = getConfig().hotkeyManager.getCommand(key, modifiers, false);

    if (!command.isEmpty()) {
        sendCommandWithSeparator(command);
        return true;
    }

    // Let default behavior handle bare arrow keys (cursor movement)
    return false;
}

bool InputWidget::miscKeyPressed(int key, Qt::KeyboardModifiers modifiers)
{
    // Check if there's a configured hotkey for this misc key (isNumpad=false)
    const QString command = getConfig().hotkeyManager.getCommand(key, modifiers, false);

    if (!command.isEmpty()) {
        sendCommandWithSeparator(command);
        return true;
    }
    return false;
}

bool InputWidget::handleTerminalShortcut(int key)
{
    switch (key) {
    case Qt::Key_H: // ^H = backspace
        textCursor().deletePreviousChar();
        return true;

    case Qt::Key_U: // ^U = delete line (clear the input)
        base::clear();
        return true;

    case Qt::Key_W: // ^W = delete word (whitespace-delimited)
    {
        QTextCursor cursor = textCursor();
        // If at start, nothing to delete
        if (cursor.atStart()) {
            return true;
        }
        // First, skip any trailing whitespace before the word
        while (!cursor.atStart() && document()->characterAt(cursor.position() - 1).isSpace()) {
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        }
        // Then, select the word (non-whitespace characters)
        while (!cursor.atStart() && !document()->characterAt(cursor.position() - 1).isSpace()) {
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        }
        cursor.removeSelectedText();
        setTextCursor(cursor);
        return true;
    }
    }
    return false;
}

bool InputWidget::handleBasicKey(int key)
{
    switch (key) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        gotInput();
        return true;

    case Qt::Key_Tab:
        return tryHistory(key);
    }
    return false;
}

bool InputWidget::handlePageKey(int key, Qt::KeyboardModifiers modifiers)
{
    // PageUp/PageDown without modifiers scroll the display widget
    if (modifiers == Qt::NoModifier) {
        bool isPageUp = (key == Qt::Key_PageUp);
        m_outputs.scrollDisplay(isPageUp);
        return true;
    }

    // With modifiers, check for hotkeys (isNumpad=false for page keys)
    const QString command = getConfig().hotkeyManager.getCommand(key, modifiers, false);
    if (!command.isEmpty()) {
        sendCommandWithSeparator(command);
        return true;
    }

    return false;
}

bool InputWidget::tryHistory(const int key)
{
    QTextCursor cursor = textCursor();
    switch (key) {
    case Qt::Key_Up:
        if (!cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor)) {
            // At the top of the document
            backwardHistory();
            return true;
        }
        break;
    case Qt::Key_Down:
        if (!cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor)) {
            // At the end of the document
            forwardHistory();
            return true;
        }
        break;
    case Qt::Key_Tab:
        tabComplete();
        return true;
    }
    return false;
}

void InputWidget::sendCommandWithSeparator(const QString &command)
{
    const auto &settings = getConfig().integratedClient;

    // Handle command separator (e.g., "l;;look" sends "l" then "look")
    if (settings.useCommandSeparator && !settings.commandSeparator.isEmpty()) {
        const QString &sep = settings.commandSeparator;
        const QString escaped = QRegularExpression::escape(sep);
        const QRegularExpression regex(QString("(?<!\\\\)%1").arg(escaped));
        const QStringList commands = command.split(regex);
        for (QString cmd : commands) {
            cmd.replace("\\" + sep, sep);
            sendUserInput(cmd);
        }
    } else {
        sendUserInput(command);
    }
}

void InputWidget::gotInput()
{
    const auto &settings = getConfig().integratedClient;

    QString input = toPlainText();
    if (settings.clearInputOnEnter) {
        clear();
    } else {
        selectAll();
    }

    // Send input (with command separator handling if enabled)
    sendCommandWithSeparator(input);

    m_inputHistory.addInputLine(input);
    m_tabHistory.addInputLine(input);
}

void InputHistory::addInputLine(const QString &string)
{
    if (!string.isEmpty() && (empty() || back() != string)) {
        // Add to line history if it is a new entry
        push_front(string);
    }

    // Trim line history
    if (static_cast<int>(size()) > getConfig().integratedClient.linesOfInputHistory) {
        pop_back();
    }

    // Reset the iterator
    m_iterator = begin();
}

void TabHistory::addInputLine(const QString &string)
{
    QStringList list = string.split(g_whitespaceRx, Qt::SplitBehaviorFlags::SkipEmptyParts);
    for (const QString &word : list) {
        if (word.length() > MIN_WORD_LENGTH) {
            // Adding this word to the dictionary
            push_front(word);

            // Trim dictionary
            if (static_cast<int>(size())
                > getConfig().integratedClient.tabCompletionDictionarySize) {
                pop_back();
            }
        }
    }

    // Reset the iterator
    m_iterator = begin();
}

void InputWidget::forwardHistory()
{
    clear();
    if (m_inputHistory.atFront()) {
        m_outputs.showMessage("Reached beginning of input history", 1000);
        return;
    }

    if (m_inputHistory.atEnd()) {
        m_inputHistory.backward();
    }

    if (!m_inputHistory.atFront()) {
        m_inputHistory.backward();
        insertPlainText(m_inputHistory.value());
    }
}

void InputWidget::backwardHistory()
{
    if (m_inputHistory.atEnd()) {
        m_outputs.showMessage("Reached end of input history", 1000);
        return;
    }

    clear();
    insertPlainText(m_inputHistory.value());
    if (!m_inputHistory.atEnd()) {
        m_inputHistory.forward();
    }
}

void InputWidget::tabComplete()
{
    // Select all characters up until the previous whitespace
    QTextCursor current = textCursor();
    if (m_tabHistory.empty() || current.atStart()
        || document()->characterAt(current.selectionStart() - 1).isSpace()) {
        return;
    }
    do {
        current.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
    } while (!current.atStart() && !document()->characterAt(current.selectionStart() - 1).isSpace());
    if (current.selectedText().isEmpty()) {
        return;
    }
    if (!m_tabbing) {
        m_tabFragment = current.selectedText();
        m_tabHistory.reset();
        m_tabbing = true;
    }

    // Iterate through all previous words
    while (!m_tabHistory.atEnd()) {
        const auto &word = m_tabHistory.value();
        if (!word.startsWith(m_tabFragment)) {
            // Try the next word
            m_tabHistory.forward();
            continue;
        }

        // Found a previous word to complete to
        current.removeSelectedText();
        current.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
        current.insertText(word);
        auto length = static_cast<int>(word.length() - m_tabFragment.length());
        current.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, length);
        current.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, length);
        setTextCursor(current);

        m_tabHistory.forward();
        break;
    }

    // If we reach the end then loop back to the beginning and clear the selected text again
    if (m_tabHistory.atEnd()) {
        textCursor().removeSelectedText();
        m_tabHistory.reset();
    }
}

bool InputWidget::event(QEvent *const event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        auto classification = classifyKey(keyEvent->key(), keyEvent->modifiers());

        if (classification.shouldHandle) {
            // Handle directly if there are real modifiers (some don't generate KeyPress)
            if (classification.realModifiers != Qt::NoModifier) {
                bool handled = false;

                switch (classification.type) {
                case KeyType::FunctionKey:
                    functionKeyPressed(keyEvent->key(), classification.realModifiers);
                    handled = true;
                    break;
                case KeyType::NumpadKey:
                    handled = numpadKeyPressed(keyEvent->key(), classification.realModifiers);
                    break;
                case KeyType::NavigationKey:
                    handled = navigationKeyPressed(keyEvent->key(), classification.realModifiers);
                    break;
                case KeyType::ArrowKey:
                    handled = arrowKeyPressed(keyEvent->key(), classification.realModifiers);
                    break;
                case KeyType::MiscKey:
                    handled = miscKeyPressed(keyEvent->key(), classification.realModifiers);
                    break;
                case KeyType::PageKey:
                    handled = handlePageKey(keyEvent->key(), classification.realModifiers);
                    break;
                case KeyType::TerminalShortcut:
                case KeyType::BasicKey:
                case KeyType::Other:
                    break;
                }

                if (handled) {
                    m_handledInShortcutOverride = true;
                    event->accept();
                    return true;
                }
            }

            // Accept so KeyPress comes through
            event->accept();
            return true;
        }
    }

    m_paletteManager.tryUpdateFromFocusEvent(*this, deref(event).type());
    return QPlainTextEdit::event(event);
}
