// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "inputwidget.h"

#include <QLinkedList>
#include <QMessageLogContext>
#include <QRegularExpression>
#include <QSize>
#include <QString>
#include <QtGui>
#include <QtWidgets>

#include "../configuration/configuration.h"

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

    // Word History
    m_lineIterator = new InputHistoryIterator(m_lineHistory);
    m_tabIterator = new TabCompletionIterator(m_tabCompletionDictionary);
    m_newInput = true;
}

QSize InputWidget::sizeHint() const
{
    return minimumSize();
}

InputWidget::~InputWidget()
{
    delete m_lineIterator;
    delete m_tabIterator;
}

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
            if (wordHistory(currentKey)) {
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
                if (wordHistory(currentKey)) {
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
    m_newInput = true;
    base::keyPressEvent(event);
}

void InputWidget::keypadMovement(const int key)
{
    switch (key) {
    case Qt::Key_Up:
        emit sendUserInput("north");
        break;
    case Qt::Key_Down:
        emit sendUserInput("south");
        break;
    case Qt::Key_Left:
        emit sendUserInput("west");
        break;
    case Qt::Key_Right:
        emit sendUserInput("east");
        break;
    case Qt::Key_PageUp:
        emit sendUserInput("up");
        break;
    case Qt::Key_PageDown:
        emit sendUserInput("down");
        break;
    case Qt::Key_Clear: // Numpad 5
        emit sendUserInput("exits");
        break;
    case Qt::Key_Home:
        emit sendUserInput("open exit");
        break;
    case Qt::Key_End:
        emit sendUserInput("close exit");
        break;
    case Qt::Key_Insert:
        emit sendUserInput("flee");
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

bool InputWidget::wordHistory(const int key)
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
    emit sendUserInput(input);
    addLineHistory(input);
    addTabHistory(input);
    m_lineIterator->toBack();
}

void InputWidget::addLineHistory(const InputHistoryEntry &string)
{
    if (!string.isEmpty() && (m_lineHistory.empty() || m_lineHistory.last() != string)) {
        // Add to line history if it is a new entry
        m_lineHistory << string;
    }

    // Trim line history
    if (m_lineHistory.size() > getConfig().integratedClient.linesOfInputHistory) {
        m_lineHistory.removeFirst();
    }
}

void InputWidget::addTabHistory(const WordHistoryEntry &string)
{
    QStringList list = string.split(s_whitespaceRx, QString::SkipEmptyParts);
    for (const QString &word : list) {
        if (word.length() > MIN_WORD_LENGTH) {
            // Adding this word to the dictionary
            m_tabCompletionDictionary << word;

            // Trim dictionary
            if (m_tabCompletionDictionary.size()
                > getConfig().integratedClient.tabCompletionDictionarySize) {
                m_tabCompletionDictionary.removeFirst();
            }
        }
    }
}

void InputWidget::forwardHistory()
{
    if (!m_lineIterator->hasNext()) {
        emit showMessage("Reached beginning of input history", 1000);
        clear();
        return;
    }

    selectAll();
    if (m_newInput) {
        addLineHistory(toPlainText());
        m_newInput = false;
    }

    QString next = m_lineIterator->next();
    // Ensure we always get "new" input
    if (next == toPlainText() && m_lineIterator->hasNext()) {
        next = m_lineIterator->next();
    }

    insertPlainText(next);
}

void InputWidget::backwardHistory()
{
    if (!m_lineIterator->hasPrevious()) {
        emit showMessage("Reached end of input history", 1000);
        return;
    }

    selectAll();
    if (m_newInput) {
        addLineHistory(toPlainText());
        m_newInput = false;
    }

    QString previous = m_lineIterator->previous();
    // Ensure we always get "new" input
    if (previous == toPlainText() && m_lineIterator->hasPrevious()) {
        previous = m_lineIterator->previous();
    }

    insertPlainText(previous);
}

void InputWidget::tabComplete()
{
    if (m_tabCompletionDictionary.isEmpty())
        return;

    QTextCursor current = textCursor();
    current.select(QTextCursor::WordUnderCursor);
    if (!m_tabbing) {
        m_tabFragment = current.selectedText();
        m_tabIterator->toBack();
        m_tabbing = true;
    }

    // If we reach the end then loop back to the beginning and clear the selected text again
    if (!m_tabIterator->hasPrevious()) {
        textCursor().removeSelectedText();
        m_tabIterator->toBack();
        return;
    }

    // Iterate through all previous words
    while (m_tabIterator->hasPrevious()) {
        // TODO(nschimme): Utilize a trie and store the search results?
        if (m_tabIterator->peekPrevious().startsWith(m_tabFragment)) {
            // Found a previous word to complete to
            current.insertText(m_tabIterator->previous());
            if (current.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor)) {
                current.movePosition(QTextCursor::Right,
                                     QTextCursor::KeepAnchor,
                                     m_tabFragment.size());
                setTextCursor(current);
            }
            return;
        }
        // Try the next word
        m_tabIterator->previous();
    }
}
