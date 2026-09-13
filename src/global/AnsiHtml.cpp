// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "AnsiHtml.h"

#include "Consts.h"
#include "TextUtils.h"
#include "utils.h"

#include <cassert>
#include <functional>

#include <QFont>
#include <QRegularExpression>
#include <QTextDocument>
#include <QTextFrame>
#include <QUrl>

namespace { // anonymous

void foreach_char(const QChar qchar,
                  const QStringView text,
                  const std::function<void()> &callback_qchar,
                  const std::function<void(const QStringView nonQchar)> &callback_between)
{
    qsizetype pos = 0;
    const qsizetype end = text.length();
    while (pos != end) {
        const qsizetype qcharIndex = text.indexOf(qchar, pos);
        if (qcharIndex < 0) {
            callback_between(text.mid(pos));
            break;
        }

        callback_between(text.mid(pos, qcharIndex - pos));
        callback_qchar();
        pos = qcharIndex + 1;
    }
}

void foreach_backspace(const QStringView text,
                       const std::function<void()> &callback_backspace,
                       const std::function<void(const QStringView nonBackspace)> &callback_between)
{
    foreach_char(char_consts::C_BACKSPACE, text, callback_backspace, callback_between);
}

} // namespace

namespace mmqt {

void setAnsiDefaultFormat(QTextCharFormat &format, const AnsiHtmlDefaults &defaults)
{
    format.setBackground(defaults.defaultBg);
    format.setForeground(defaults.defaultFg);
    format.setFontWeight(QFont::Normal);
    format.setFontUnderline(false);
    format.setFontItalic(false);
    format.setFontStrikeOut(false);
}

RawAnsi updateAnsiFormat(QTextCharFormat &format,
                         const AnsiHtmlDefaults &defaults,
                         const RawAnsi &before,
                         RawAnsi updated)
{
    if (before == updated) {
        return updated;
    }

    if (updated == RawAnsi{}) {
        setAnsiDefaultFormat(format, defaults);
        return updated;
    }

    const auto diff = before.getFlags() ^ updated.getFlags();

    for (const AnsiStyleFlagEnum flag : diff) {
        switch (flag) {
        case AnsiStyleFlagEnum::Italic:
            format.setFontItalic(updated.hasItalic());
            break;
        case AnsiStyleFlagEnum::Underline: {
            // QTextCharFormat doesn't support other underline styles.
            format.setFontUnderline(updated.hasUnderline());
            using ULS = QTextCharFormat::UnderlineStyle;
            const auto style = std::invoke([&updated]() -> QTextCharFormat::UnderlineStyle {
                switch (updated.getUnderlineStyle()) {
                case AnsiUnderlineStyleEnum::Dotted:
                    return ULS::DotLine;
                case AnsiUnderlineStyleEnum::Curly:
                    return ULS::WaveUnderline;
                case AnsiUnderlineStyleEnum::Dashed:
                    return ULS::DashUnderline;
                case AnsiUnderlineStyleEnum::None:
                case AnsiUnderlineStyleEnum::Normal:
                case AnsiUnderlineStyleEnum::Double: // not supported by Qt
                default:
                    return ULS::SingleUnderline;
                }
            });
            format.setUnderlineStyle(style);
            break;
        }
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
        case AnsiStyleFlagEnum::Blink:   // ignored
        case AnsiStyleFlagEnum::Reverse: // handled below
        case AnsiStyleFlagEnum::Conceal: // handled below
            break;
        }
    }

    const bool conceal = updated.hasConceal();
    const bool reverse = updated.hasReverse();
    const bool intense = updated.hasBold();

    auto bg = mmqt::decodeColor(updated.bg, defaults.defaultBg, false);
    auto fg = mmqt::decodeColor(updated.fg, defaults.defaultFg, intense);
    auto ul = mmqt::decodeColor(updated.ul, defaults.getDefaultUl(), intense);

    if (reverse) {
        // was swap(fg, bg) before we supported underline color
        mmqt::reverseInPlace(fg);
        mmqt::reverseInPlace(bg);
        mmqt::reverseInPlace(ul);
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

AnsiTextToDocument::AnsiTextToDocument(QTextDocument &document,
                                       AnsiHtmlDefaults def,
                                       const bool input_linkify)
    : cursor{document.rootFrame()->firstCursorPosition()}
    , format{cursor.charFormat()}
    , defaults{std::move(def)}
    , linkify{input_linkify}
{}

void AnsiTextToDocument::init()
{
    QTextDocument &document = deref(cursor.document());
    QTextFrameFormat frameFormat = document.rootFrame()->frameFormat();
    frameFormat.setBackground(defaults.defaultBg);
    frameFormat.setForeground(defaults.defaultFg);
    document.rootFrame()->setFrameFormat(frameFormat);

    format = cursor.charFormat();
    setAnsiDefaultFormat(format, defaults);
    cursor.setCharFormat(format);
}

void AnsiTextToDocument::displayText(const QStringView input_str)
{
    // ANSI codes are formatted as the following:
    // escape + [ + n1 (+ n2) + m
    static const QRegularExpression ansi_regex{R"regex(\x1B[^A-Za-z\x1B]*[A-Za-z]?)regex"};
    static const QRegularExpression url_regex{
        R"regex(https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*))regex"};

    // REVISIT: should we even bother supporting backspaces?
    //
    // QTextEdit is not a terminal, so it doesn't really have the concept of a mutable text buffer
    // where you can backspace and then later overwrite the previous letter with something else.
    //
    // (Yes, technically we could abuse the cursor this to achieve a similar result, but the
    // cursor is used in multiple functions, so that might be too fragile.)
    //
    // Current solution: simulate the backspace operation by adding a backspace character to the
    // buffer, and then later remove the backspace and previous character.
    //
    // MUME could send a telnet event that signals that the player is waiting,
    // and then we could display the waiting state some other way.
    //
    // Or we could just ignore the backspaces and display "\|/-" instead of the animation.
    //
    // It might also be worth considering using an emoji or an icon instead of C_BACKSPACE.
    static const volatile bool allow_backspaces = true;

    // note: debug_backspaces effectively implies allow_backspaces == false,
    // because writes "(Backspace)" instead of a backspace character, and then it leaves both
    // "(Backspace)" and the letter that would have been removed by the backspace operation.
    static const volatile bool debug_backspaces = false;

    auto try_remove_backspace = [this]() {
        if (!allow_backspaces) {
            return;
        }

        const auto block = cursor.block();
        if (!block.isValid() || cursor.block().length() == 0) {
            return;
        }

        const auto text = block.text();
        if (text.isNull() || text.isEmpty() || text.back() != char_consts::C_BACKSPACE) {
            return;
        }

        cursor.deletePreviousChar();
        if (block.length() > 0) {
            cursor.deletePreviousChar();
        }
    };

    auto add_raw = [this, &try_remove_backspace](const QStringView text,
                                                 const QTextCharFormat &withFmt) {
        try_remove_backspace();
        cursor.insertText(text.toString(), withFmt);
    };

    auto try_add_backspace = [this, &add_raw]() {
        if (debug_backspaces) {
            add_raw(u"(BACKSPACE)", {});
            return;
        }

        if (!allow_backspaces || cursor.position() < 1) {
            return;
        }

        const auto block = cursor.block();
        if (!block.isValid() && block.length() < 1) {
            return;
        }

        const auto text = block.text();
        if (text.isNull() || text.isEmpty()) {
            return;
        }

        add_raw(mmqt::QS_BACKSPACE, {});
    };

    auto add_formatted = [this, &add_raw, &try_remove_backspace](const QStringView text) {
        if (!linkify) {
            add_raw(text, format);
            return;
        }

        mmqt::foreach_regex(
            url_regex,
            text,
            [this, &try_remove_backspace](const QStringView url) {
                const auto s = url.toString();
                // TODO: override the document's CSS for URLs
                const auto link
                    = QString(
                          R"(<a href="%1" style="color: cyan; background-color: #003333; font-weight: normal;" target="_blank">%2</a>)")
                          .arg(QString::fromUtf8(QUrl::fromUserInput(s).toEncoded()),
                               s.toHtmlEscaped());

                try_remove_backspace();
                cursor.insertHtml(link);
            },
            [this, &add_raw](const QStringView non_url) { add_raw(non_url, format); });
    };

    // Display text using a cursor
    mmqt::foreach_regex(
        ansi_regex,
        input_str,
        [this, &add_raw](const QStringView ansiStr) {
            assert(!ansiStr.isEmpty() && ansiStr.front() == char_consts::C_ESC);
            if (mmqt::isAnsiColor(ansiStr)) {
                if (auto optNewColor = mmqt::parseAnsiColor(currentAnsi, ansiStr)) {
                    currentAnsi = updateAnsiFormat(format, defaults, currentAnsi, *optNewColor);
                }
            } else if (mmqt::isAnsiEraseLine(ansiStr)) {
                cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
                cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
            } else {
                add_raw(u"<ESC>", {});
                if (ansiStr.length() > 1) {
                    add_raw(ansiStr.mid(1), format);
                }
            }
        },
        [&try_add_backspace, &add_formatted](const QStringView textStr) {
            foreach_backspace(textStr, try_add_backspace, add_formatted);
        });
}

void AnsiTextToDocument::limitScrollback(const int lineLimit)
{
    QTextDocument &document = deref(cursor.document());
    const int lineCount = document.lineCount();
    if (lineCount > lineLimit) {
        const int trimLines = lineCount - lineLimit;
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, trimLines);
        cursor.removeSelectedText();
        cursor.movePosition(QTextCursor::End);
    }
}

} // namespace mmqt
