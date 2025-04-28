// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "inputwidget.h"

#include "../configuration/configuration.h"
#include "../global/Color.h"

#include <QFont>
#include <QLinkedList>
#include <QMessageLogContext>
#include <QRegularExpression>
#include <QSize>
#include <QString>
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
    const auto currentKey = event->key();
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
                                 current.selectedText().length());
            setTextCursor(current);
        }
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

#define X_CASE(_Name) \
    case Qt::Key_##_Name: { \
        event->accept(); \
        functionKeyPressed(#_Name); \
        break; \
    }
            X_CASE(F1);
            X_CASE(F2);
            X_CASE(F3);
            X_CASE(F4);
            X_CASE(F5);
            X_CASE(F6);
            X_CASE(F7);
            X_CASE(F8);
            X_CASE(F9);
            X_CASE(F10);
            X_CASE(F11);
            X_CASE(F12);

#undef X_CASE

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

void InputWidget::functionKeyPressed(const QString &keyName)
{
    sendUserInput(keyName);
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
    QStringList list = string.split(g_whitespaceRx, Qt::SkipEmptyParts);
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
        auto length = word.length() - m_tabFragment.length();
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
    m_paletteManager.tryUpdateFromFocusEvent(*this, deref(event).type());
    return QPlainTextEdit::event(event);
}
