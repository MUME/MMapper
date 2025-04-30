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
#include <QStyle>
#include <QTextCursor>
#include <QToolTip>
#include <QtGui>

namespace { // anonymous

const constexpr int TAB_WIDTH_SPACES = 8;
const volatile bool ignore_non_default_underline_colors = false;

void foreach_backspace(const QStringView text,
                       const std::function<void()> &callback_backspace,
                       const std::function<void(const QStringView nonBackspace)> &callback_between)
{
    qsizetype pos = 0;
    const qsizetype end = text.length();
    while (pos != end) {
        const qsizetype backspaceIndex = text.indexOf(char_consts::C_BACKSPACE, pos);
        if (backspaceIndex < 0) {
            callback_between(text.mid(pos));
            break;
        }

        callback_between(text.mid(pos, backspaceIndex - pos));
        callback_backspace();
        pos = backspaceIndex + 1;
    }
}

} // namespace

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

DisplayWidgetOutputs::~DisplayWidgetOutputs() = default;
DisplayWidget::DisplayWidget(QWidget *const parent)
    : QTextBrowser(parent)
    , m_ansiTextHelper{static_cast<QTextEdit &>(*this)}
{
    setReadOnly(true);
    setOverwriteMode(true);
    setUndoRedoEnabled(false);
    setDocumentTitle("MMapper Mud Client");
    setTextInteractionFlags(Qt::TextBrowserInteraction);
    setOpenExternalLinks(true);
    setTabChangesFocus(false);

    // REVISIT: Is this necessary to do in both places?
    document()->setUndoRedoEnabled(false);
    m_ansiTextHelper.init();

    // Set word wrap mode and other settings
    QFontMetrics fm{getDefaultFont()};
    setLineWrapMode(QTextEdit::FixedColumnWidth);
    setWordWrapMode(QTextOption::WordWrap);
    setSizeIncrement(fm.averageCharWidth(), fm.lineSpacing());
    setTabStopDistance(fm.horizontalAdvance(" ") * TAB_WIDTH_SPACES);

    // Scrollbar settings
    QScrollBar *const scrollbar = verticalScrollBar();
    scrollbar->setSingleStep(fm.lineSpacing());
    scrollbar->setPageStep(sizeHint().height());
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(this, &DisplayWidget::copyAvailable, this, [this](const bool available) {
        m_canCopy = available;
        if (available) {
            this->setFocus();
        }
    });

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        bool atBottom = (value == verticalScrollBar()->maximum());
        getOutput().showPreview(!atBottom);
    });
}

DisplayWidget::~DisplayWidget() = default;

QSize DisplayWidget::sizeHint() const
{
    const auto &settings = getConfig().integratedClient;
    const auto &margins = contentsMargins();
    const int frame = frameWidth() * 2;
    QFontMetrics fm{getDefaultFont()};
    const int scrollbarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent);

    int x = (settings.columns * fm.averageCharWidth()) + margins.left() + margins.right()
            + scrollbarExtent + frame;
    int y = (settings.rows * fm.lineSpacing()) + margins.top() + margins.bottom() + scrollbarExtent
            + frame;

    return QSize(x, y);
}

void DisplayWidget::resizeEvent(QResizeEvent *const event)
{
    const auto &margins = contentsMargins();
    QFontMetrics fm{getDefaultFont()};
    const int scrollbarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    const int frame = frameWidth() * 2;

    int widthAvailable = size().width() - margins.left() - margins.right() - scrollbarExtent
                         - frame;
    int heightAvailable = size().height() - margins.top() - margins.bottom() - scrollbarExtent
                          - frame;

    int x = widthAvailable / fm.averageCharWidth();
    int y = heightAvailable / fm.lineSpacing();

    // REVISIT: Right now only Mac is correct and we need to replace these row hacks
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Linux) {
        y -= 6;
    } else if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows) {
        y -= 2;
    }

    // Set the line wrap width/height
    setLineWrapColumnOrWidth(x);
    verticalScrollBar()->setPageStep(y);

    // Inform user of new dimensions
    QString message = QString("Dimensions: %1x%2").arg(x).arg(y);
    QFontMetrics fmToolTip(QToolTip::font());
    QPoint messageDiff(fmToolTip.horizontalAdvance(message) / 2, fmToolTip.lineSpacing() / 2);
    QToolTip::showText(mapToGlobal(rect().center() - messageDiff), message, this, rect(), 1000);

    const auto &settings = getConfig().integratedClient;
    if (settings.autoResizeTerminal) {
        getOutput().windowSizeChanged(x, y);
    }

    QTextEdit::resizeEvent(event);

    bool atBottom = (verticalScrollBar()->sliderPosition() == verticalScrollBar()->maximum());
    getOutput().showPreview(!atBottom);
}

void DisplayWidget::keyPressEvent(QKeyEvent *event)
{
    bool isModifier = event->modifiers() != Qt::NoModifier;
    auto isNavigationKey = [&event]() {
        switch (event->key()) {
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            return true;
        default:
            return false;
        }
    };
    auto isAllowedKeySequence = [&event]() {
        return event->matches(QKeySequence::Copy) || event->matches(QKeySequence::Paste)
               || event->matches(QKeySequence::Cut) || event->matches(QKeySequence::SelectAll);
    };

    if (isModifier || isNavigationKey() || isAllowedKeySequence()) {
        QTextBrowser::keyPressEvent(event);
    } else {
        getOutput().returnFocusToInput();
        event->accept();
    }
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

void AnsiTextHelper::displayText(const QString &input_str)
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

        auto isTwiddler = [](const QChar c) -> bool {
            using namespace char_consts;
            switch (c.unicode()) {
            case C_BACKSLASH:
            case C_MINUS_SIGN:
            case C_SLASH:
            case C_SPACE:
            case C_VERTICAL_BAR:
                return true;
            default:
                return false;
            }
        };

        const auto block = cursor.block();
        if (!block.isValid() && block.length() < 1) {
            return;
        }

        const auto text = block.text();
        if (text.isNull() || text.isEmpty()) {
            return;
        }

        if (!isTwiddler(text.back())) {
            const auto max_shown = 20;
            const auto shown = (text.length() < max_shown)
                                   ? text
                                   : "..." + text.mid(text.length() - max_shown);
            qWarning() << "refusing to backspace over non-twiddler:" << shown;
        }

        add_raw(mmqt::QS_BACKSPACE, {});
    };

    auto add_formatted = [this, &add_raw, &try_remove_backspace](const QStringView text) {
        mmqt::foreach_regex(
            url_regex,
            text,
            [this, &try_remove_backspace](const QStringView url) {
                const auto s = url.toString();
                // TODO: override the document's CSS for URLs
                const auto link
                    = QString(
                          R"(<a href="%1" style="color: cyan; background-color: #003333; font-weight: normal;">%2</a>)")
                          .arg(QString::fromUtf8(QUrl::fromUserInput(s).toEncoded()),
                               s.toHtmlEscaped());

                try_remove_backspace();
                cursor.insertHtml(link);
            },
            [this, &add_raw](const QStringView &non_url) { add_raw(non_url, format); });
    };

    // Display text using a cursor
    mmqt::foreach_regex(
        ansi_regex,
        input_str,
        [this, &add_raw](const QStringView ansiStr) {
            assert(!ansiStr.isEmpty() && ansiStr.front() == char_consts::C_ESC);
            if (mmqt::isAnsiColor(ansiStr)) {
                if (auto optNewColor = mmqt::parseAnsiColor(currentAnsi, ansiStr)) {
                    currentAnsi = updateFormat(format, defaults, currentAnsi, *optNewColor);
                }
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
    if (ignore_non_default_underline_colors && !updated.ul.hasDefaultColor()) {
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
    const auto diff = before.getFlags() ^ updated.getFlags();

    for (const auto flag : diff) {
        switch (flag) {
        case AnsiStyleFlagEnum::Italic:
            format.setFontItalic(updated.hasItalic());
            break;
        case AnsiStyleFlagEnum::Underline: {
            // QTextCharFormat doesn't support other underline styles.
            format.setFontUnderline(updated.hasUnderline());
            using ULS = QTextCharFormat::UnderlineStyle;
            const auto style = [&updated]() -> QTextCharFormat::UnderlineStyle {
                switch (updated.getUnderlineStyle()) {
                case AnsiUnderlineStyle::Dotted:
                    return ULS::DotLine;
                case AnsiUnderlineStyle::Curly:
                    return ULS::WaveUnderline;
                case AnsiUnderlineStyle::Dashed:
                    return ULS::DashUnderline;
                case AnsiUnderlineStyle::None:
                case AnsiUnderlineStyle::Normal:
                case AnsiUnderlineStyle::Double: // not supported by Qt
                default:
                    return ULS::SingleUnderline;
                }
            }();
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

void setAnsiText(QTextEdit *const pEdit, const std::string_view text)
{
    QTextEdit &edit = deref(pEdit);

    edit.clear();
    edit.setReadOnly(true);
    edit.setOverwriteMode(true);
    edit.setUndoRedoEnabled(false);
    edit.setTextInteractionFlags(Qt::TextSelectableByMouse);

    // REVISIT: Is this necessary to do in both places?
    deref(edit.document()).setUndoRedoEnabled(false);

    AnsiTextHelper helper{edit};
    helper.init();
    helper.displayText(mmqt::toQStringUtf8(text));

    // FIXME: why don't the scroll bars work?
    if (QScrollBar *const vs = edit.verticalScrollBar()) {
        vs->setEnabled(true);
    }
}
