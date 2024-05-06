// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "inputwidget.h"

#include "../configuration/configuration.h"

#include <QLinkedList>
#include <QMessageLogContext>
#include <QRegularExpression>
#include <QSize>
#include <QString>
#include <QtGui>
#include <QtWidgets>

static constexpr const int MIN_WORD_LENGTH = 3;

static const QRegularExpression s_whitespaceRx(R"(\W+)");

InputWidget::InputWidget(QWidget *const parent)
    : QPlainTextEdit(parent)
{
    // Size Policy
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    // Minimum Size
    QFontMetrics fm(currentCharFormat().font());
    setMinimumSize(
        QSize(fm.averageCharWidth(),
              fm.lineSpacing()
                  + (static_cast<int>(document()->documentMargin()) + QFrame::frameWidth()) * 2
                  + contentsMargins().top() + contentsMargins().bottom()));
    setSizeIncrement(fm.averageCharWidth(), fm.lineSpacing());

    // Line Wrapping
    setLineWrapMode(QPlainTextEdit::NoWrap);
}

QSize InputWidget::sizeHint() const
{
    return minimumSize();
}

InputWidget::~InputWidget() = default;

void InputWidget::keyPressEvent(QKeyEvent *const event)
{
    const auto currentKey = event->key();
    const auto currentModifiers = event->modifiers();

    if (currentKey != Qt::Key_Tab) {
        m_tabbing = false;
    }

    // REVISIT: if (useConsoleEscapeKeys) ...
    if (currentModifiers == Qt::ControlModifier) {
        switch (currentKey) {
        case Qt::Key_H: // ^H = backspace
            // REVISIT: can this be translated to backspace key?
            break;
        case Qt::Key_U: // ^U = delete line (clear the input)
            base::clear();
            return;
        case Qt::Key_W: // ^W = delete word
            // REVISIT: can this be translated to ctrl+shift+leftarrow + backspace?
            break;
        }

    } else if (currentModifiers == Qt::NoModifier) {
        switch (currentKey) {
        /** Submit the current text */
        case Qt::Key_Return:
        case Qt::Key_Enter:
            gotInput();
            event->accept();
            return;

            /** Key bindings for word history and tab completion */
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Tab:
            if (tryHistory(currentKey)) {
                event->accept();
                return;
            }
            break;
        }

    } else if (currentModifiers == Qt::KeypadModifier) {
        if constexpr (CURRENT_PLATFORM == PlatformEnum::Mac) {
            // NOTE: MacOS does not differentiate between arrow keys and the keypad keys
            // and as such we disable keypad movement functionality in favor of history
            switch (currentKey) {
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_Tab:
                if (tryHistory(currentKey)) {
                    event->accept();
                    return;
                }
                break;
            }
        } else {
            switch (currentKey) {
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_Left:
            case Qt::Key_Right:
            case Qt::Key_PageUp:
            case Qt::Key_PageDown:
            case Qt::Key_Clear: // Numpad 5
            case Qt::Key_Home:
            case Qt::Key_End:
                keypadMovement(currentKey);
                event->accept();
                return;
            }
        }
    }

    // All other input
    base::keyPressEvent(event);
}

void InputWidget::keypadMovement(const int key)
{
    switch (key) {
    case Qt::Key_Up:
        sendUserInput("north");
        break;
    case Qt::Key_Down:
        sendUserInput("south");
        break;
    case Qt::Key_Left:
        sendUserInput("west");
        break;
    case Qt::Key_Right:
        sendUserInput("east");
        break;
    case Qt::Key_PageUp:
        sendUserInput("up");
        break;
    case Qt::Key_PageDown:
        sendUserInput("down");
        break;
    case Qt::Key_Clear: // Numpad 5
        sendUserInput("exits");
        break;
    case Qt::Key_Home:
        sendUserInput("open exit");
        break;
    case Qt::Key_End:
        sendUserInput("close exit");
        break;
    case Qt::Key_Insert:
        sendUserInput("flee");
        break;
    case Qt::Key_Delete:
    case Qt::Key_Plus:
    case Qt::Key_Minus:
    case Qt::Key_Slash:
    case Qt::Key_Asterisk:
    default:
        qDebug() << "! Unknown keypad movement" << key;
    }
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

void InputWidget::gotInput()
{
    QString input = toPlainText();
    if (getConfig().integratedClient.clearInputOnEnter) {
        clear();
    } else {
        selectAll();
    }
    sendUserInput(input);
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
    QStringList list = string.split(s_whitespaceRx, Qt::SkipEmptyParts);
    for (const QString &word : list) {
        if (word.length() > MIN_WORD_LENGTH) {
            // Adding this word to the dictionary
            push_back(word);

            // Trim dictionary
            if (static_cast<int>(size())
                > getConfig().integratedClient.tabCompletionDictionarySize) {
                pop_front();
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
        emit sig_showMessage("Reached beginning of input history", 1000);
        return;
    }

    if (m_inputHistory.atEnd())
        m_inputHistory.backward();

    if (!m_inputHistory.atFront()) {
        m_inputHistory.backward();
        insertPlainText(m_inputHistory.value());
    }
}

void InputWidget::backwardHistory()
{
    if (m_inputHistory.atEnd()) {
        emit sig_showMessage("Reached end of input history", 1000);
        return;
    }

    clear();
    insertPlainText(m_inputHistory.value());
    if (!m_inputHistory.atEnd())
        m_inputHistory.forward();
}

void InputWidget::tabComplete()
{
    if (m_tabHistory.empty())
        return;

    QTextCursor current = textCursor();
    current.select(QTextCursor::WordUnderCursor);
    if (!m_tabbing) {
        m_tabFragment = current.selectedText();
        m_tabHistory.reset();
        m_tabbing = true;
    }

    // If we reach the end then loop back to the beginning and clear the selected text again
    if (m_tabHistory.atEnd()) {
        textCursor().removeSelectedText();
        m_tabHistory.reset();
        return;
    }

    // Iterate through all previous words
    while (!m_tabHistory.atEnd()) {
        // TODO(nschimme): Utilize a trie and search?
        const auto &word = m_tabHistory.value();
        if (!word.startsWith(m_tabFragment)) {
            // Try the next word
            m_tabHistory.forward();
            continue;
        }

        // Found a previous word to complete to
        current.insertText(word);
        if (current.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor)) {
            current.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, m_tabFragment.size());
            setTextCursor(current);
        }
        m_tabHistory.forward();
        break;
    }
}
