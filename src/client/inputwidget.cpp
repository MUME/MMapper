// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "inputwidget.h"

#include "../configuration/configuration.h"
#include "../global/Color.h"
#include "ClientWidget.h"

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

    const auto currentKey = event->keyCombination().key();
    const auto currentModifiers = event->modifiers();

    if (m_tabbing) {
        if (currentKey != Qt::Key_Tab) {
            m_tabbing = false;
        }

        // If Backspace or Escape is pressed, reject the completion
        QTextCursor current = textCursor();
        if (currentKey == Qt::Key_Backspace || currentKey == Qt::Key_Escape) {
            current.removeSelectedText();
            event->accept();
            return;
        }

        // For any other key press, accept the completion
        if (currentKey != Qt::Key_Tab) {
            current.movePosition(QTextCursor::Right,
                                 QTextCursor::MoveAnchor,
                                 static_cast<int>(current.selectedText().length()));
            setTextCursor(current);
        }
    }

    if (handleCommandInput(currentKey, currentModifiers)) {
        event->accept();
        return;
    }

    const Hotkey hk(currentKey, currentModifiers);
    if (!hk.isValid()) {
        if (currentKey == Qt::Key_Up || currentKey == Qt::Key_Down) {
            if (tryHistory(currentKey)) {
                event->accept();
                return;
            }
        } else if (currentKey == Qt::Key_PageUp || currentKey == Qt::Key_PageDown) {
            m_outputs.scrollDisplay(currentKey == Qt::Key_PageUp);
            event->accept();
            return;
        } else if (handleBasicKey(currentKey)) {
            event->accept();
            return;
        }

        base::keyPressEvent(event);
        return;
    }

    if (currentModifiers == Qt::NoModifier && handleBasicKey(currentKey)) {
        event->accept();
        return;
    }

    // All other input
    base::keyPressEvent(event);
}

bool InputWidget::handleCommandInput(Qt::Key key, Qt::KeyboardModifiers mods)
{
    // Terminal Shortcuts (Ctrl+U, Ctrl+W, Ctrl+H)
    if ((mods & Qt::ControlModifier) && handleTerminalShortcut(key)) {
        return true;
    }

    const Hotkey hk(static_cast<Qt::Key>(key), mods);
    if (!hk.isValid()) {
        return false;
    }

    // Try Hotkeys
    if (auto command = m_outputs.getHotkey(hk)) {
        sendCommandWithSeparator(*command);
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

bool InputWidget::tryHistory(const int key)
{
    /** Key bindings for word history and tab completion */
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
        auto *const keyEvent = static_cast<QKeyEvent *>(event);
        if (handleCommandInput(keyEvent->keyCombination().key(), keyEvent->modifiers())) {
            m_handledInShortcutOverride = true;
            event->accept();
            return true;
        }
    }

    m_paletteManager.tryUpdateFromFocusEvent(*this, deref(event).type());
    return QPlainTextEdit::event(event);
}
