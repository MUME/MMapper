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

#include "displaywidget.h"
#include "configuration/configuration.h"

#include <QBrush>
#include <QDebug>
#include <QScrollBar>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFrame>

// ANSI codes are formatted as the following:
// escape + [ + n1 (+ n2) + m
const QRegExp DisplayWidget::s_ansiRx(R"(\0033\[((?:\d+;)*\d+)m)");

DisplayWidget::DisplayWidget(QWidget *parent)
    : QTextEdit(parent)
{
    setReadOnly(true);
    setOverwriteMode(true);
    setUndoRedoEnabled(false);
    setDocumentTitle("MMapper Mud Client");
    setTextInteractionFlags(Qt::TextSelectableByMouse);
    setTabChangesFocus(false);
    setContextMenuPolicy(Qt::NoContextMenu);

    document()->setUndoRedoEnabled(false);

    // Default Colors
    m_foregroundColor = Config().m_clientForegroundColor;
    m_backgroundColor = Config().m_clientBackgroundColor;
    m_blackColor = QColor("#2e3436");
    m_darkGrayColor = QColor("#555753");
    m_redColor = QColor("#cc0000");
    m_brightRedColor = QColor("#ef2929");
    m_greenColor = QColor("#4e9a06");
    m_brightGreenColor = QColor("#8ae234");
    m_yellowColor = QColor("#c4a000");
    m_brightYellowColor = QColor("#fce94f");
    m_blueColor = QColor("#3465a4");
    m_brightBlueColor = QColor("#729fcf");
    m_magentaColor = QColor("#75507b");
    m_brightMagentaColor = QColor("#ad7fa8");
    m_cyanColor = QColor("#06989a");
    m_brightCyanColor = QColor("#34e2e2");
    m_grayColor = QColor("#d3d7cf");
    m_whiteColor = QColor("#eeeeec");

    // Default Font
    m_serverOutputFont = Config().m_clientFont;

    QTextFrameFormat frameFormat = document()->rootFrame()->frameFormat();
    frameFormat.setBackground(m_backgroundColor);
    frameFormat.setForeground(m_foregroundColor);
    document()->rootFrame()->setFrameFormat(frameFormat);

    m_cursor = document()->rootFrame()->firstCursorPosition();
    m_format = m_cursor.charFormat();
    setDefaultFormat(m_format);
    m_cursor.setCharFormat(m_format);

    QFontMetrics fm(m_serverOutputFont);
    int x = fm.averageCharWidth() * Config().m_clientColumns;
    int y = fm.lineSpacing() * Config().m_clientRows;
    setMinimumSize(QSize(x + contentsMargins().left() + contentsMargins().right(),
                         y + contentsMargins().top() + contentsMargins().bottom()));
    setLineWrapMode(QTextEdit::FixedColumnWidth);
    setLineWrapColumnOrWidth(Config().m_clientColumns);
    setWordWrapMode(QTextOption::WordWrap);
    setSizeIncrement(fm.averageCharWidth(), fm.lineSpacing());
    setTabStopWidth(fm.width(" ") * 8); // A tab is 8 spaces wide

    QScrollBar *scrollbar = verticalScrollBar();
    scrollbar->setSingleStep(fm.lineSpacing());
    scrollbar->setPageStep(y);
}

DisplayWidget::~DisplayWidget() = default;

void DisplayWidget::resizeEvent(QResizeEvent *event)
{
    QFontMetrics fm(m_serverOutputFont);
    int x = (size().width() - contentsMargins().left() - contentsMargins().right())
            / fm.averageCharWidth();
    int y = (size().height() - contentsMargins().top() - contentsMargins().bottom())
            / fm.lineSpacing();
    setLineWrapColumnOrWidth(x);
    verticalScrollBar()->setPageStep(y);
    emit showMessage(QString("Dimensions: %1x%2").arg(x).arg(y), 1000);
    QTextEdit::resizeEvent(event);
}

void DisplayWidget::setDefaultFormat(QTextCharFormat &format)
{
    format.setFont(m_serverOutputFont);
    format.setBackground(m_backgroundColor);
    format.setForeground(m_foregroundColor);
    format.setFontWeight(QFont::Normal);
    format.setFontUnderline(false);
    format.setFontItalic(false);
    format.setFontStrikeOut(false);
}

void DisplayWidget::displayText(const QString &str)
{
    // Split ansi from this text
    QStringList textList, ansiList;
    int textIndex = 0, ansiIndex = 0;
    while ((ansiIndex = s_ansiRx.indexIn(str, textIndex)) != -1) {
        textList << str.mid(textIndex, ansiIndex - textIndex);
        ansiList << s_ansiRx.cap(1);
        textIndex = ansiIndex + s_ansiRx.matchedLength();
    }
    if (textIndex < str.length()) {
        textList << str.mid(textIndex);
    }

    // Display text using a cursor
    QStringListIterator textIterator(textList), ansiIterator(ansiList);
    while (textIterator.hasNext()) {
        QString textStr = textIterator.next();

        if (textStr.length() != 0) {
            // Backspaces occur on the next character being drawn
            if (m_backspace) {
                m_cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, 1);
                m_backspace = false;
            }
            int backspaceIndex = textStr.indexOf('\10');
            if (backspaceIndex == -1) {
                // No backspace
                m_cursor.insertText(textStr, m_format);

            } else {
                m_backspace = true;
                m_cursor.insertText(textStr.mid(0, backspaceIndex), m_format);
                m_cursor.insertText(textStr.mid(backspaceIndex + 1), m_format);
            }
        }

        // Change format according to ansi codes
        if (ansiIterator.hasNext()) {
            // split several semicoloned ansi codes into individual codes
            QStringList subAnsi = ansiIterator.next().split(';');
            QStringListIterator ansiCodeIterator(subAnsi);
            while (ansiCodeIterator.hasNext()) {
                int ansiCode = ansiCodeIterator.next().toInt();
                updateFormat(m_format, ansiCode);
            }
        }
    }

    // Ensure we limit the scrollback history
    int lineLimit = Config().m_clientLinesOfScrollback;
    if (document()->lineCount() > lineLimit) {
        int trimLines = document()->lineCount() - lineLimit;
        m_cursor.movePosition(QTextCursor::Start);
        m_cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, trimLines);
        m_cursor.removeSelectedText();
        m_cursor.movePosition(QTextCursor::End);
    }

    verticalScrollBar()->setSliderPosition(verticalScrollBar()->maximum());
}

void DisplayWidget::updateFormat(QTextCharFormat &format, int ansiCode)
{
    switch (ansiCode) {
    case 0:
        // turn ANSI off (i.e. return to normal defaults)
        setDefaultFormat(format);
        break;
    case 1:
        // bold
        format.setFontWeight(QFont::Bold);
        updateFormatBoldColor(format);
        break;
    case 4:
        // underline
        format.setFontUnderline(true);
        break;
    case 5:
        // blink
        format.setFontItalic(true);
        break;
    case 7:
        // inverse
        {
            QBrush tempBrush = format.background();
            format.setBackground(format.foreground());
            format.setForeground(tempBrush);
            break;
        }
    case 8:
        // strike-through
        format.setFontStrikeOut(true);
        break;
    case 30:
        // black foreground
        format.setForeground(m_blackColor);
        break;
    case 31:
        // red foreground
        format.setForeground(m_redColor);
        break;
    case 32:
        // green foreground
        format.setForeground(m_greenColor);
        break;
    case 33:
        // yellow foreground
        format.setForeground(m_yellowColor);
        break;
    case 34:
        // blue foreground
        format.setForeground(m_blueColor);
        break;
    case 35:
        // magenta foreground
        format.setForeground(m_magentaColor);
        break;
    case 36:
        // cyan foreground
        format.setForeground(m_cyanColor);
        break;
    case 37:
        // gray foreground
        format.setForeground(m_grayColor);
        break;
    case 40:
        // black background
        format.setBackground(m_blackColor);
        break;
    case 41:
        // red background
        format.setBackground(m_redColor);
        break;
    case 42:
        // green background
        format.setBackground(m_greenColor);
        break;
    case 43:
        // yellow background
        format.setBackground(m_yellowColor);
        break;
    case 44:
        // blue background
        format.setBackground(m_blueColor);
        break;
    case 45:
        // magenta background
        format.setBackground(m_magentaColor);
        break;
    case 46:
        // cyan background
        format.setBackground(m_cyanColor);
        break;
    case 47:
        // gray background
        format.setBackground(m_grayColor);
        break;
    case 90:
        // high-black foreground
        format.setForeground(m_darkGrayColor);
        break;
    case 91:
        // high-red foreground
        format.setForeground(m_brightRedColor);
        break;
    case 92:
        // high-green foreground
        format.setForeground(m_brightGreenColor);
        break;
    case 93:
        // high-yellow foreground
        format.setForeground(m_brightYellowColor);
        break;
    case 94:
        // high-blue foreground
        format.setForeground(m_brightBlueColor);
        break;
    case 95:
        // high-magenta foreground
        format.setForeground(m_brightMagentaColor);
        break;
    case 96:
        // high-cyan foreground
        format.setForeground(m_brightCyanColor);
        break;
    case 97:
        // high-white foreground
        format.setForeground(m_whiteColor);
        break;
    case 100:
        // high-black background
        format.setBackground(m_darkGrayColor);
        break;
    case 101:
        // high-red background
        format.setBackground(m_brightRedColor);
        break;
    case 102:
        // high-green background
        format.setBackground(m_brightGreenColor);
        break;
    case 103:
        // high-yellow background
        format.setBackground(m_brightYellowColor);
        break;
    case 104:
        // high-blue background
        format.setBackground(m_brightBlueColor);
        break;
    case 105:
        // high-magenta background
        format.setBackground(m_brightMagentaColor);
        break;
    case 106:
        // high-cyan background
        format.setBackground(m_brightCyanColor);
        break;
    case 107:
        // high-white background
        format.setBackground(m_whiteColor);
        break;
    default:
        qWarning() << "Unknown ansicode" << ansiCode;
        format.setBackground(Qt::gray);
    };
}

void DisplayWidget::updateFormatBoldColor(QTextCharFormat &format)
{
    if (format.foreground() == m_blackColor) {
        format.setForeground(m_darkGrayColor);
    } else if (format.foreground() == m_redColor) {
        format.setForeground(m_brightRedColor);
    } else if (format.foreground() == m_greenColor) {
        format.setForeground(m_brightGreenColor);
    } else if (format.foreground() == m_yellowColor) {
        format.setForeground(m_brightYellowColor);
    } else if (format.foreground() == m_blueColor) {
        format.setForeground(m_brightBlueColor);
    } else if (format.foreground() == m_magentaColor) {
        format.setForeground(m_brightMagentaColor);
    } else if (format.foreground() == m_cyanColor) {
        format.setForeground(m_brightCyanColor);
    } else if (format.foreground() == m_grayColor) {
        format.setForeground(m_whiteColor);
    }
}
