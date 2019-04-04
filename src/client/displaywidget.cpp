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

#include <QMessageLogContext>
#include <QRegularExpression>
#include <QScrollBar>
#include <QString>
#include <QTextCursor>
#include <QtGui>

#include "../configuration/configuration.h"
#include "../global/Color.h"

// ANSI codes are formatted as the following:
// escape + [ + n1 (+ n2) + m
static const QRegularExpression s_ansiRx(R"(\x1b\[((?:\d+;)*\d+)m)");

DisplayWidget::DisplayWidget(QWidget *parent)
    : QTextEdit(parent)
{
    const auto &settings = getConfig().integratedClient;

    setReadOnly(true);
    setOverwriteMode(true);
    setUndoRedoEnabled(false);
    setDocumentTitle("MMapper Mud Client");
    setTextInteractionFlags(Qt::TextSelectableByMouse);
    setTabChangesFocus(false);
    setContextMenuPolicy(Qt::NoContextMenu);

    document()->setUndoRedoEnabled(false);

    // Default Colors
    m_foregroundColor = settings.foregroundColor;
    m_backgroundColor = settings.backgroundColor;

    // Default Font
    m_serverOutputFont.fromString(settings.font);

    QTextFrameFormat frameFormat = document()->rootFrame()->frameFormat();
    frameFormat.setBackground(m_backgroundColor);
    frameFormat.setForeground(m_foregroundColor);
    document()->rootFrame()->setFrameFormat(frameFormat);

    m_cursor = document()->rootFrame()->firstCursorPosition();
    m_format = m_cursor.charFormat();
    setDefaultFormat(m_format);
    m_cursor.setCharFormat(m_format);

    // Add an extra character for the scrollbars
    QFontMetrics fm(m_serverOutputFont);
    int x = fm.averageCharWidth() * (settings.columns + 1);
    int y = fm.lineSpacing() * (settings.rows + 1);
    setMinimumSize(QSize(x + contentsMargins().left() + contentsMargins().right(),
                         y + contentsMargins().top() + contentsMargins().bottom()));
    setLineWrapMode(QTextEdit::FixedColumnWidth);
    setLineWrapColumnOrWidth(settings.columns);
    setWordWrapMode(QTextOption::WordWrap);
    setSizeIncrement(fm.averageCharWidth(), fm.lineSpacing());
    setTabStopWidth(fm.width(" ") * 8); // A tab is 8 spaces wide

    QScrollBar *const scrollbar = verticalScrollBar();
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
    // We subtract an extra character for the scrollbars
    x -= 1;
    y -= 1;
    setLineWrapColumnOrWidth(x);
    verticalScrollBar()->setPageStep(y);
    emit showMessage(QString("Dimensions: %1x%2").arg(x).arg(y), 1000);
    const auto &settings = getConfig().integratedClient;
    if (settings.autoResizeTerminal) {
        emit windowSizeChanged(x, y);
    }
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
    QRegularExpressionMatchIterator it = s_ansiRx.globalMatch(str);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        ansiIndex = match.capturedStart(0);
        textList << str.mid(textIndex, ansiIndex - textIndex);
        ansiList << match.captured(1);
        textIndex = match.capturedEnd(0);
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
    const int lineLimit = getConfig().integratedClient.linesOfScrollback;
    const int lineCount = document()->lineCount();
    if (lineCount > lineLimit) {
        const int trimLines = lineCount - lineLimit;
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
    case 2:
        // dim
        format.setFontWeight(QFont::Light);
        break;
    case 3:
        // italic
        format.setFontItalic(true);
        break;
    case 4:
        // underline
        format.setFontUnderline(true);
        break;
    case 5:
        // blink slow
        format.setFontWeight(QFont::Bold);
        break;
    case 6:
        // blink fast
        format.setFontWeight(QFont::Bold);
        updateFormatBoldColor(format);
        break;
    case 7:
    case 27:
        // inverse
        {
            QBrush tempBrush = format.background();
            format.setBackground(format.foreground());
            format.setForeground(tempBrush);
            break;
        }
    case 8:
        // conceal
        format.setForeground(format.background());
        break;
    case 9:
        // strike-through
        format.setFontStrikeOut(true);
        break;
    case 21:
    case 22:
    case 25:
        // bold off
        format.setFontWeight(QFont::Normal);
        break;
    case 23:
        // italic off
        format.setFontItalic(false);
        break;
    case 24:
        // underline off
        format.setFontUnderline(false);
        break;
    case 28:
        // conceal off
        format.setForeground(m_foregroundColor);
        break;
    case 29:
        // not crossed out
        format.setFontStrikeOut(false);
        break;
    case 30:
        // black foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 31:
        // red foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 32:
        // green foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 33:
        // yellow foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 34:
        // blue foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 35:
        // magenta foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 36:
        // cyan foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 37:
        // gray foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 40:
        // black background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    case 41:
        // red background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    case 42:
        // green background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    case 43:
        // yellow background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    case 44:
        // blue background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    case 45:
        // magenta background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    case 46:
        // cyan background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    case 47:
        // gray background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    case 90:
        // high-black foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 91:
        // high-red foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 92:
        // high-green foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 93:
        // high-yellow foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 94:
        // high-blue foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 95:
        // high-magenta foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 96:
        // high-cyan foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 97:
        // high-white foreground
        format.setForeground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 30)));
        break;
    case 100:
        // high-black background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    case 101:
        // high-red background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    case 102:
        // high-green background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    case 103:
        // high-yellow background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    case 104:
        // high-blue background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    case 105:
        // high-magenta background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    case 106:
        // high-cyan background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    case 107:
        // high-white background
        format.setBackground(ansiColor(static_cast<AnsiColorTable>(ansiCode - 40)));
        break;
    default:
        qWarning() << "Unknown ansicode" << ansiCode;
        format.setBackground(Qt::gray);
    };
}

void DisplayWidget::updateFormatBoldColor(QTextCharFormat &format)
{
    for (int i = 0; i <= static_cast<int>(AnsiColorTable::white); i++) {
        if (format.foreground().color() == ansiColor(static_cast<AnsiColorTable>(i)))
            format.setForeground(ansiColor(static_cast<AnsiColorTable>(i + 60)));
    }
}
