/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "inputwidget.h"
#include "configuration/configuration.h"

#include <QDebug>
#include <QKeyEvent>

#include <QTextCharFormat>
#include <QFont>
#include <QFontMetrics>
#include <QTextCursor>

// Smart Splitter
#include <QSizePolicy>

// Word History
#include <QStringList>

#define WORD_HISTORY_SIZE 200

const QRegExp InputWidget::s_whitespaceRx("\\W+");

InputWidget::InputWidget(QWidget *parent)
    : QPlainTextEdit(parent)
{
    // Size Policy
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    // Minimum Size
    QFontMetrics fm(currentCharFormat().font());
    int textHeight = fm.lineSpacing();
    int documentMargin = document()->documentMargin() * 2;
    int frameWidth = QFrame::frameWidth() * 2;
    setMinimumSize(0, textHeight + documentMargin + frameWidth + contentsMargins().bottom() +
                   contentsMargins().top());
    setSizeIncrement(fm.averageCharWidth(), textHeight);

    // Line Wrapping
    setLineWrapMode(QPlainTextEdit::NoWrap);

    // Word History
    m_lineIterator = new QMutableStringListIterator(m_lineHistory); // TODO: Replace with Vector
    m_tabIterator = new QMutableStringListIterator(m_wordHistory); // TODO: Replace with Map
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

void InputWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() !=  Qt::Key_Tab) m_tabState = NORMAL;

    // Check for key bindings
    switch (event->key()) {
    /** Enter could mean to submit the current text or add a newline */
    case Qt::Key_Return:
    case Qt::Key_Enter:
        switch (event->modifiers()) {
        case Qt::NoModifier:    /** Submit text */
            gotInput();
            event->accept();
            break;

        case Qt::ShiftModifier: /** Add newline (i.e. ignore) */
        default:                /** Otherwise ignore  */
            QPlainTextEdit::keyPressEvent(event);

        };
        break;

    /** Key bindings for word history, tab completion, etc) */
    // NOTE: MacOS does not differentiate between arrow keys and the keypad keys
    // and as such we disable keypad movement functionality in favor of history
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Tab:
        switch (event->modifiers()) {
        case Qt::KeypadModifier:
#ifndef Q_OS_MAC
            keypadMovement(event->key());
            event->accept();
            break;
#endif
        case Qt::NoModifier:
            wordHistory(event->key());
            event->accept();
            break;
        default:
            QPlainTextEdit::keyPressEvent(event);
        };
        break;

    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
    case Qt::Key_Clear:  // Numpad 5
    case Qt::Key_Home:
    case Qt::Key_End:
#ifndef Q_OS_MAC
        switch (event->modifiers()) {
        case Qt::KeypadModifier:
            keypadMovement(event->key());
            event->accept();
            break;
        };
#endif
    // TODO, Implement these following keys
    case Qt::Key_Delete:
    case Qt::Key_Plus:
    case Qt::Key_Minus:
    case Qt::Key_Slash:
    case Qt::Key_Asterisk:
    case Qt::Key_Insert:

    /** All other keys */
    default:
        QPlainTextEdit::keyPressEvent(event);
    };
}

void InputWidget::keypadMovement(int key)
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
    };
}


void InputWidget::wordHistory(int key)
{
    QTextCursor cursor = textCursor();
    switch (key) {
    case Qt::Key_Up:
        if (!cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor)) {
            // At the top of the document
            backwardHistory();

        }
        break;
    case Qt::Key_Down:
        if (!cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor)) {
            // At the end of the document
            forwardHistory();

        }
        break;
    case Qt::Key_Tab:
        tabWord();
        break;
    };
}

void InputWidget::gotInput()
{
    QString input = toPlainText();
    if (Config().m_clientClearInputOnEnter) {
        clear();
    } else {
        selectAll();
    }
    emit sendUserInput(input);
    addLineHistory(input);
    addTabHistory(input);
    m_lineIterator->toBack();
}

void InputWidget::addLineHistory(const QString &string)
{
    if (!string.isEmpty()) {
        qDebug() << "* adding line history:" << string;
        m_lineHistory << string;
    }

    if (m_lineHistory.size() > 100) m_lineHistory.removeFirst();
}


void InputWidget::addTabHistory(const QString &string)
{
    QStringList list = string.split(s_whitespaceRx, QString::SkipEmptyParts);
    addTabHistory(list);
}


void InputWidget::addTabHistory(const QStringList &list)
{
    foreach (QString word, list)
        if (word.length() > 3) {
            qDebug() << "* adding word history:" << word;
            m_wordHistory << word;
        }

    while (m_wordHistory.size() > WORD_HISTORY_SIZE) m_wordHistory.removeFirst();
}


void InputWidget::forwardHistory()
{
    if (!m_lineIterator->hasNext()) {
        qDebug() << "* no newer word history to go to";
        clear();
        return ;

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
        qDebug() << "* no older word history to go to";
        return ;

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


void InputWidget::showCommandHistory()
{
    QString message("#history:\r\n");
    int i = 1;
    foreach (QString s, m_lineHistory)
        message.append(QString::number(i++) + " " + s + "\r\n");

    qDebug() << "* showing history" << message;

    emit displayMessage(message);

}


void InputWidget::tabWord()
{
    QTextCursor current = textCursor();
    current.select(QTextCursor::WordUnderCursor);
    switch (m_tabState) {
    case NORMAL:
        qDebug() << "* in NORMAL state";
        m_tabFragment = current.selectedText();
        m_tabIterator->toBack();
        m_tabState = TABBING;
    case TABBING:
        qDebug() << "* in TABBING state";
        if (!m_tabIterator->hasPrevious() && !m_wordHistory.isEmpty()) {
            qDebug() << "* Resetting tab iterator";
            m_tabIterator->toBack();
        }
        while (m_tabIterator->hasPrevious()) {
            if (m_tabIterator->peekPrevious().startsWith(m_tabFragment)) {
                current.insertText(m_tabIterator->previous());
                if (current.movePosition(QTextCursor::StartOfWord,
                                         QTextCursor::KeepAnchor)) {
                    current.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,
                                         m_tabFragment.size());
                    setTextCursor(current);
                    qDebug() << "*it worked";
                }
                return ;
            } else m_tabIterator->previous();
        }
        break;
    };
}
