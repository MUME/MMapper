// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "displaywidget.h"

#include "../configuration/configuration.h"
#include "../global/AnsiTextUtils.h"

#include <QMessageLogContext>
#include <QRegularExpression>
#include <QScrollBar>
#include <QString>
#include <QTextCursor>
#include <QToolTip>
#include <QtGui>

static const constexpr int SCROLLBAR_BUFFER = 1;
static const constexpr int TAB_WIDTH_SPACES = 8;

FontDefaults::FontDefaults()
{
    const auto &settings = getConfig().integratedClient;

    // Default Colors
    defaultBg = settings.backgroundColor;
    defaultFg = settings.foregroundColor;

    // Default Font
    serverOutputFont.fromString(settings.font);
}

void AnsiTextHelper::init()
{
    QTextFrameFormat frameFormat = textEdit.document()->rootFrame()->frameFormat();
    frameFormat.setBackground(defaults.defaultBg);
    frameFormat.setForeground(defaults.defaultFg);
    textEdit.document()->rootFrame()->setFrameFormat(frameFormat);

    format = cursor.charFormat();
    setDefaultFormat(format, defaults);
    cursor.setCharFormat(format);
}

DisplayWidget::DisplayWidget(QWidget *const parent)
    : QTextEdit(parent)
    , m_ansiTextHelper{static_cast<QTextEdit &>(*this)}
{
    setReadOnly(true);
    setOverwriteMode(true);
    setUndoRedoEnabled(false);
    setDocumentTitle("MMapper Mud Client");
    setTextInteractionFlags(Qt::TextSelectableByMouse);
    setTabChangesFocus(false);

    // REVISIT: Is this necessary to do in both places?
    document()->setUndoRedoEnabled(false);
    m_ansiTextHelper.init();

    {
        const auto &settings = getConfig().integratedClient;

        // Add an extra character for the scrollbars
        QFontMetrics fm{getDefaultFont()};
        int y = fm.lineSpacing() * (settings.rows + SCROLLBAR_BUFFER);
        setLineWrapMode(QTextEdit::FixedColumnWidth);
        setLineWrapColumnOrWidth(settings.columns);
        setWordWrapMode(QTextOption::WordWrap);
        setSizeIncrement(fm.averageCharWidth(), fm.lineSpacing());
        setTabStopDistance(fm.horizontalAdvance(" ") * TAB_WIDTH_SPACES);

        QScrollBar *const scrollbar = verticalScrollBar();
        scrollbar->setSingleStep(fm.lineSpacing());
        scrollbar->setPageStep(y);
    }

    connect(this, &DisplayWidget::copyAvailable, this, [this](const bool available) {
        m_canCopy = available;
    });
}

DisplayWidget::~DisplayWidget() = default;

QSize DisplayWidget::sizeHint() const
{
    const auto &settings = getConfig().integratedClient;
    const auto &margins = contentsMargins();
    QFontMetrics fm{getDefaultFont()};
    const int x = fm.averageCharWidth() * (settings.columns + SCROLLBAR_BUFFER) + margins.left()
                  + margins.right();
    const int y = fm.lineSpacing() * (settings.rows + SCROLLBAR_BUFFER) + margins.top()
                  + margins.bottom();
    return QSize(x, y);
}

void DisplayWidget::resizeEvent(QResizeEvent *const event)
{
    const auto &margins = contentsMargins();
    QFontMetrics fm{getDefaultFont()};
    int x = (size().width() - margins.left() - margins.right()) / fm.averageCharWidth();
    int y = (size().height() - margins.top() - margins.bottom()) / fm.lineSpacing();
    // We subtract an extra character for the scrollbars
    x -= SCROLLBAR_BUFFER;
    y -= SCROLLBAR_BUFFER;
    setLineWrapColumnOrWidth(x);
    verticalScrollBar()->setPageStep(y);

    // Inform user of new dimensions
    QString message = QString("Dimensions: %1x%2").arg(x).arg(y);
    QFontMetrics fmToolTip(QToolTip::font());
    QPoint messageDiff(fmToolTip.horizontalAdvance(message) / 2, fmToolTip.lineSpacing() / 2);
    QToolTip::showText(mapToGlobal(rect().center() - messageDiff), message, this, rect(), 1000);

    const auto &settings = getConfig().integratedClient;
    if (settings.autoResizeTerminal) {
        emit sig_windowSizeChanged(x, y);
    }

    QTextEdit::resizeEvent(event);
}

void setDefaultFormat(QTextCharFormat &format, const FontDefaults &defaults)
{
    format.setFont(defaults.serverOutputFont);
    format.setBackground(defaults.defaultBg);
    format.setForeground(defaults.defaultFg);
    format.setFontWeight(QFont::Normal);
    format.setFontUnderline(false);
    format.setFontItalic(false);
    format.setFontStrikeOut(false);
}

void AnsiTextHelper::displayText(const QString &str)
{
    // Split ansi from this text
    QStringList textList, ansiList;
    int textIndex = 0, ansiIndex = 0;
    // ANSI codes are formatted as the following:
    // escape + [ + n1 (+ n2) + m
    static const QRegularExpression ansiRx(R"(\x1b\[((?:\d+;?)*)m)");
    QRegularExpressionMatchIterator it = ansiRx.globalMatch(str);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        ansiIndex = match.capturedStart(0);
        textList << str.mid(textIndex, ansiIndex - textIndex);
        ansiList << match.captured(0);
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
            if (backspace) {
                cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, 1);
                backspace = false;
            }
            int backspaceIndex = textStr.indexOf(char_consts::C_BACKSPACE);
            if (backspaceIndex == -1) {
                // No backspace
                cursor.insertText(textStr, format);

            } else {
                backspace = true;
                cursor.insertText(textStr.mid(0, backspaceIndex), format);
                cursor.insertText(textStr.mid(backspaceIndex + 1), format);
            }
        }

        // Change format according to ansi codes
        if (ansiIterator.hasNext()) {
            auto ansiStr = ansiIterator.next();
            if (auto optNewColor = mmqt::parseAnsiColor(currentAnsi, ansiStr)) {
                currentAnsi = updateFormat(format, defaults, currentAnsi, *optNewColor);
            }
        }
    }
}

void AnsiTextHelper::limitScrollback(int lineLimit)
{
    const int lineCount = textEdit.document()->lineCount();
    if (lineCount > lineLimit) {
        const int trimLines = lineCount - lineLimit;
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, trimLines);
        cursor.removeSelectedText();
        cursor.movePosition(QTextCursor::End);
    }
}

void DisplayWidget::slot_displayText(const QString &str)
{
    const int lineLimit = getConfig().integratedClient.linesOfScrollback;

    auto &vscroll = deref(verticalScrollBar());
    constexpr int ALMOST_ALL_THE_WAY = 4;
    const bool wasAtBottom = (vscroll.sliderPosition() >= vscroll.maximum() - ALMOST_ALL_THE_WAY);

    m_ansiTextHelper.displayText(str);

    // REVISIT: include option to limit scrollback in the displayText function,
    // so it can choose to remove lines from the top when the overflow occurs,
    // to improve performance for massive inserts.
    m_ansiTextHelper.limitScrollback(lineLimit);

    // Detecting the keyboard Scroll Lock status would be preferable, but we'll have to live with
    // this because Qt is apparently the only windowing system in existence that doesn't provide
    // a way to query CapsLock/NumLock/ScrollLock ?!?
    //
    // REVISIT: should this be user-configurable?
    if (wasAtBottom) {
        vscroll.setSliderPosition(vscroll.maximum());
    }
}

NODISCARD static QColor decodeColor(const AnsiColorRGB var)
{
    return QColor::fromRgb(var.r, var.g, var.b);
}

NODISCARD static QColor decodeColor(AnsiColor256 var, const bool intense)
{
    if (var.color < 8 && intense) {
        var.color += 8;
    }

    return mmqt::ansi256toRgb(var.color);
}

NODISCARD static QColor decodeColor(const AnsiColorVariant var,
                                    const QColor defaultColor,
                                    const bool intense)
{
    if (var.hasRGB()) {
        return decodeColor(var.getRGB());
    }

    if (var.has256()) {
        return decodeColor(var.get256(), intense);
    }

    return defaultColor;
}

static void reverseInPlace(QColor &color)
{
    color.setRed(255 - color.red());
    color.setGreen(255 - color.green());
    color.setBlue(255 - color.blue());
}

RawAnsi updateFormat(QTextCharFormat &format,
                     const FontDefaults &defaults,
                     const RawAnsi &before,
                     RawAnsi updated)
{
    if (!updated.ul.hasDefaultColor()) {
        // Ignore underline color.
        updated.ul = AnsiColorVariant{};
    }

    if (before == updated) {
        return updated;
    }

    if (updated == RawAnsi{}) {
        setDefaultFormat(format, defaults);
        return updated;
    }

    // auto removed = before.flags & ~updated.flags;
    // auto added = updated.flags & ~before.flags;
    auto diff = before.getFlags() ^ updated.getFlags();

    for (const auto flag : diff) {
        switch (flag) {
        case AnsiStyleFlagEnum::Italic:
            format.setFontItalic(updated.hasItalic());
            break;
        case AnsiStyleFlagEnum::Underline:
            format.setFontUnderline(updated.hasUnderline());
            break;
        case AnsiStyleFlagEnum::Strikeout:
            format.setFontStrikeOut(updated.hasStrikeout());
            break;
        case AnsiStyleFlagEnum::Bold:
        case AnsiStyleFlagEnum::Faint:
            if (updated.hasBold()) {
                format.setFontWeight(QFont::Bold);
            } else if (updated.hasFaint()) {
                format.setFontWeight(QFont::Light);
            } else {
                format.setFontWeight(QFont::Normal);
            }
            break;
        case AnsiStyleFlagEnum::Blink:
            break;
        case AnsiStyleFlagEnum::Reverse:
        case AnsiStyleFlagEnum::Conceal:
            break;
        }
    }

    const bool conceal = updated.hasConceal();
    const bool reverse = updated.hasReverse();
    const bool intense = updated.hasBold();

    auto bg = decodeColor(updated.bg, defaults.defaultBg, false);
    auto fg = decodeColor(updated.fg, defaults.defaultFg, intense);
    auto ul = decodeColor(updated.ul, defaults.getDefaultUl(), intense);

    if (reverse) {
        // was swap(fg, bg) before we supported underline color
        ::reverseInPlace(fg);
        ::reverseInPlace(bg);
        ::reverseInPlace(ul);
    }

    // Create a config setting and use it here if you really want to miss text that others will see!
    bool userExplicitlyOptedInForConceal = false;
    if (conceal && userExplicitlyOptedInForConceal) {
        ul = fg = bg;
    }

    format.setBackground(bg);
    format.setForeground(fg);
    format.setUnderlineColor(ul);
    return updated;
}
