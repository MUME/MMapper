// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "remoteeditwidget.h"

#include "../client/displaywidget.h"
#include "../configuration/configuration.h"
#include "../global/AnsiTextUtils.h"
#include "../global/CharUtils.h"
#include "../global/Consts.h"
#include "../global/LineUtils.h"
#include "../global/TabUtils.h"
#include "../global/TextUtils.h"
#include "../global/entities.h"
#include "../global/utils.h"
#include "../global/window_utils.h"
#include "../viewers/AnsiViewWindow.h"

#include <cassert>
#include <cctype>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMessageLogContext>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QSize>
#include <QStatusBar>
#include <QString>
#include <QTextDocument>
#include <QtGui>
#include <QtWidgets>

using namespace char_consts;

static constexpr QColor darkOrangeQColor{0xFF, 0x8C, 0x00};
static constexpr const bool USE_TOOLTIPS = false;
// REVISIT: Figure out how to tweak logic to accept actual maximum length of 80
static constexpr const int MAX_LENGTH = 79;
namespace mmqt {
NODISCARD static int measureTabAndAnsiAware(const QString &s)
{
    int col = 0;
    for (auto token : mmqt::AnsiTokenizer{s}) {
        using Type = decltype(token.type);
        switch (token.type) {
        case Type::Ansi:
            break;
        case Type::Newline:
            /* unexpected */
            col = 0;
            break;
        case Type::Space:
        case Type::Control:
        case Type::Word:
            col = mmqt::measureExpandedTabsOneLine(token.getQStringView(), col);
            break;
        }
    }
    return col;
}
} // namespace mmqt

class QWidget;

/// Groups everything in the scope as a single undo action.
class NODISCARD RaiiGroupUndoActions final
{
private:
    QTextCursor m_cursor;

public:
    explicit RaiiGroupUndoActions(const QTextCursor &cursor)
        : m_cursor{cursor}
    {
        m_cursor.beginEditBlock();
    }
    ~RaiiGroupUndoActions() { m_cursor.endEditBlock(); }
};

class NODISCARD LineHighlighter final : public QSyntaxHighlighter
{
private:
    const int m_maxLength;

public:
    explicit LineHighlighter(int maxLength, QTextDocument *parent);
    ~LineHighlighter() final;

    void highlightBlock(const QString &line) override
    {
        highlightTabs(line);
        highlightOverflow(line);
        highlightTrailingSpace(line);
        highlightAnsi(line);
        highlightEntities(line);
        highlightEncodingErrors(line);
    }

    NODISCARD static QTextCharFormat getBackgroundFormat(const Qt::GlobalColor color)
    {
        QTextCharFormat fmt;
        fmt.setBackground(QBrush{color});
        return fmt;
    }
    NODISCARD static QTextCharFormat getBackgroundFormat(const QColor &color)
    {
        QTextCharFormat fmt;
        fmt.setBackground(QBrush{color});
        return fmt;
    }

    void highlightTabs(const QString &line)
    {
        if (line.indexOf(C_TAB) < 0) {
            return;
        }

        const QTextCharFormat fmt = getBackgroundFormat(Qt::yellow);
        // REVISIT: Should this be in TabUtils?
        mmqt::foreachCharPos(line, C_TAB, [this, &fmt](const auto at) {
            setFormat(static_cast<int>(at), 1, fmt);
        });
    }

    void highlightOverflow(const QString &line)
    {
        const int breakPos = (mmqt::measureTabAndAnsiAware(line) <= m_maxLength) ? -1 : m_maxLength;
        if (breakPos < 0) {
            return;
        }

        const auto getFmt = []() -> QTextCharFormat {
            QTextCharFormat fmt;
            fmt.setFontUnderline(true);
            fmt.setUnderlineStyle(QTextCharFormat::UnderlineStyle::WaveUnderline);
            fmt.setUnderlineColor(Qt::red);
            return fmt;
        };

        const auto length = line.length() - breakPos;
        setFormat(breakPos, length, getFmt());
    }

    void highlightTrailingSpace(const QString &line)
    {
        const int breakPos = mmqt::findTrailingWhitespace(line);
        if (breakPos < 0) {
            return;
        }

        const auto length = line.length() - breakPos;
        setFormat(breakPos, length, getBackgroundFormat(Qt::red));
    }

    void highlightAnsi(const QString &line)
    {
        const auto red = getBackgroundFormat(Qt::red);
        const auto cyan = getBackgroundFormat(Qt::cyan);

        mmqt::foreachAnsi(line, [this, &red, &cyan](const int start, const QStringView sv) {
            setFormat(start, static_cast<int>(sv.length()), mmqt::isValidAnsiColor(sv) ? cyan : red);
        });
    }

    void highlightEntities(const QString &line)
    {
        const auto red = getBackgroundFormat(Qt::red);
        const auto darkOrange = getBackgroundFormat(darkOrangeQColor);
        const auto yellow = getBackgroundFormat(Qt::yellow);

        struct NODISCARD MyEntityCallback final : entities::EntityCallback
        {
        private:
            LineHighlighter &m_self;
            const QTextCharFormat &m_red;
            const QTextCharFormat &m_darkOrange;
            const QTextCharFormat &m_yellow;

        public:
            explicit MyEntityCallback(LineHighlighter &self,
                                      const QTextCharFormat &red,
                                      const QTextCharFormat &darkOrange,
                                      const QTextCharFormat &yellow)
                : m_self{self}
                , m_red{red}
                , m_darkOrange{darkOrange}
                , m_yellow{yellow}
            {}

        private:
            void virt_decodedEntity(int start, int len, OptQChar decoded) final
            {
                const auto get_fmt = [this, &decoded]() -> const QTextCharFormat & {
                    if (!decoded) {
                        return m_red;
                    }
                    const auto val = decoded->unicode();
                    return (val < 256) ? m_yellow : m_darkOrange;
                };

                m_self.setFormat(start, len, get_fmt());
            }
        } callback{*this, red, darkOrange, yellow};
        entities::foreachEntity(QStringView{line}, callback);
    }

    void highlightEncodingErrors(const QString &line)
    {
        const auto red = getBackgroundFormat(Qt::red);
        const auto darkOrange = getBackgroundFormat(darkOrangeQColor);
        const auto yellow = getBackgroundFormat(Qt::yellow);
        const auto cyan = getBackgroundFormat(Qt::cyan);

        using OptFmt = std::optional<QTextCharFormat>;
#define DECL_FMT(name, color, tooltip) \
    OptFmt opt_##name{}; \
    const auto get_##name##_fmt = [&opt_##name, &color]() -> const QTextCharFormat & { \
        auto &opt = opt_##name; \
        if (!opt) { \
            opt = color; \
            if (USE_TOOLTIPS) { \
                opt.value().setToolTip(tooltip); \
            } \
        } \
        return opt.value(); \
    }
        DECL_FMT(unicode, red, "Unicode");
        DECL_FMT(nbsp, cyan, "NBSP");
        DECL_FMT(utf8, yellow, "UTF-8");
        DECL_FMT(unprintable, darkOrange, "(unprintable)");

#undef DECL_FMT

        int pos = 0;
        QChar last;
        bool hasLast = false;
        for (const auto &qc : line) {
            if (qc.unicode() > 0xFF) {
                // no valid representation
                // TODO: add a means of transliterating things like special quotes
                // Does Qt offer this feature somewhere?
                // If not, consider iconv -t LATIN1//TRANSLIT

                setFormat(pos, 1, get_unicode_fmt());
            } else {
                const char c = qc.toLatin1();
                switch (c) {
                case C_NBSP:
                    setFormat(pos, 1, get_nbsp_fmt());
                    break;
                case C_ESC:
                case C_TAB: // REVISIT: move tab handler to here?
                    /* handled elsewhere */
                    break;
                default: {
                    const auto uc = static_cast<uint8_t>(c);
                    if (hasLast
                        && (isClamped<int>(uc, 0x80, 0xBF) && (last == 0xC2 || last == 0xC3))) {
                        // Sometimes these are UTF-8 encoded Latin1 values,
                        // but they could also be intended, so they're not errors.
                        // TODO: add a feature to fix these on a case-by-case basis?
                        setFormat(pos - 1, 2, get_utf8_fmt());
                    } else if (std::iscntrl(uc) || (!std::isprint(uc) && !std::isspace(uc))) {
                        setFormat(pos, 1, get_unprintable_fmt());
                    }
                }
                }
            }

            last = qc;
            hasLast = true;
            ++pos;
        }
    }
};

LineHighlighter::LineHighlighter(const int maxLength, QTextDocument *const parent)
    : QSyntaxHighlighter(parent)
    , m_maxLength{maxLength}
{}

LineHighlighter::~LineHighlighter() = default;

RemoteTextEdit::RemoteTextEdit(const QString &initialText, QWidget *const parent)
    : base(parent)
{
    base::setPlainText(initialText);
}
RemoteTextEdit::~RemoteTextEdit() = default;

/* Qt virtual */
void RemoteTextEdit::keyPressEvent(QKeyEvent *const event)
{
    switch (event->key()) {
    case Qt::Key_Tab:
        return handleEventTab(event);
    case Qt::Key_Backtab:
        return handleEventBacktab(event);
    default:
        /* calls parent */
        return base::keyPressEvent(event);
    }
}

/* Qt virtual */
bool RemoteTextEdit::event(QEvent *const event)
{
    if ((USE_TOOLTIPS)) {
        if (event->type() == QEvent::ToolTip) {
            handle_toolTip(event);
            return true;
        }
    }

    /* calls parent */
    return base::event(event);
}

void RemoteTextEdit::handle_toolTip(QEvent *const event) const
{
    auto *const helpEvent = checked_dynamic_downcast<QHelpEvent *>(event);
    QTextCursor cursor = cursorForPosition(helpEvent->pos());
    if ((false)) {
        cursor.select(QTextCursor::WordUnderCursor);
    } else {
        // Where you actually think you're pointing.
        cursor.movePosition(QTextCursor::PreviousCharacter);
    }

    const auto pos = cursor.positionInBlock();
    const QTextBlock &block = cursor.block();
    auto text = block.text();
    if (text.isEmpty() || pos >= text.length()) {
        QToolTip::hideText();
        return;
    }

    const auto fmt = cursor.charFormat();
    auto tooltip = fmt.toolTip();
    if (tooltip.isEmpty()) {
        tooltip = "tooltip:";
    }

    const auto c = text.at(pos);
    char unicode_buf[8];
    std::snprintf(unicode_buf, sizeof(unicode_buf), "U+%04X", static_cast<uint>(c.unicode()));

    // REVISIT: Why doesn't this get the tooltip we applied in the LineHighlighter?
    // Apparently it's a reported bug that Qt refuses to acknowledge.
    // TODO: custom tooltips for error reporting and URLs
    QToolTip::showText(helpEvent->globalPos(), QString("%1 (%2)").arg(tooltip).arg(unicode_buf));
}

NODISCARD static bool lineHasTabs(const QTextCursor &line)
{
    if (line.isNull()) {
        return false;
    }
    const auto &block = line.block();
    if (!block.isValid()) {
        return false;
    }
    return block.text().indexOf(C_TAB) >= 0;
}

static void expandTabs(QTextCursor line)
{
    const QTextBlock &block = line.block();
    const QString &s = block.text();

    mmqt::TextBuffer prefix;
    prefix.appendExpandedTabs(QStringView{s}, 0);

    line.setPosition(block.position());
    line.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    line.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    line.insertText(prefix.getQString());
}

static void tryRemoveLeadingSpaces(QTextCursor line, const int max_spaces)
{
    const auto &block = line.block();
    if (!block.isValid()) {
        return;
    }

    const auto &text = block.text();
    if (text.isEmpty()) {
        return;
    }

    const int to_remove = [&text, max_spaces]() -> int {
        const int len = std::min(max_spaces, text.length());
        int n = 0;
        while (n < len && text.at(n) == C_SPACE) {
            ++n;
        }
        return n;
    }();

    if (to_remove == 0) {
        return;
    }

    line.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    line.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, to_remove);
    line.removeSelectedText();
}

struct NODISCARD LineRange final
{
    NODISCARD static auto beg(QTextCursor cur)
    {
        auto from = cur.document()->findBlock(cur.selectionStart());
        auto begCursor = QTextCursor(from);
        return begCursor;
    }

    NODISCARD static auto end(QTextCursor cur)
    {
        const auto &to = cur.document()->findBlock(cur.selectionEnd());
        auto endCursor = QTextCursor(to);
        if (!endCursor.isNull()) {
            endCursor.movePosition(QTextCursor::NextBlock);
        }
        return endCursor;
    }
};

/// Caller is responsible for not invalidating the end cursor.
///
/// callback(QTextCursor);
/// -or-
/// callback(const QTextCursor&);
template<typename Callback>
static void foreach_partly_selected_block(QTextCursor cur, Callback &&callback)
{
    auto beg = LineRange::beg(cur);
    auto end = LineRange::end(cur);
    for (auto it = beg; !it.isNull() && it <= end;) {
        if (it.block().isValid()) {
            callback(static_cast<const QTextCursor &>(it));
        }
        if (!it.movePosition(QTextCursor::NextBlock)) {
            break;
        }
    }
}

enum class NODISCARD CallbackResultEnum { KEEP_GOING, STOP };

// Caller is responsible for not invalidating the end cursor.
///
/// CallbackResultEnum callback(QTextCursor);
/// -or-
/// CallbackResultEnum callback(const QTextCursor&);
template<typename Callback>
static void foreach_partly_selected_block_until(QTextCursor cur, Callback &&callback)
{
    static_assert(
        std::is_same_v<CallbackResultEnum, decltype(callback(std::declval<const QTextCursor &>()))>);

    auto beg = LineRange::beg(cur);
    auto end = LineRange::end(cur);
    for (auto it = beg; !it.isNull() && it < end;) {
        if (it.block().isValid()) {
            if (callback(static_cast<const QTextCursor &>(it)) == CallbackResultEnum::STOP) {
                return;
            }
        }
        if (!it.movePosition(QTextCursor::NextBlock)) {
            break;
        }
    }
}

template<typename Callback>
bool exists_partly_selected_block(QTextCursor cur, Callback &&callback)
{
    static_assert(std::is_same_v<bool, decltype(callback(std::declval<const QTextCursor &>()))>);

    bool result = false;
    foreach_partly_selected_block_until(cur,
                                        [&result, &callback](QTextCursor it) -> CallbackResultEnum {
                                            if (callback(it)) {
                                                result = true;
                                                return CallbackResultEnum::STOP;
                                            }
                                            return CallbackResultEnum::KEEP_GOING;
                                        });
    return result;
}

static void insertPrefix(QTextCursor line, const QString &prefix)
{
    line.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    line.insertText(prefix);
}

/* perform a 2-space block indentation of all partly-selected lines */
void RemoteTextEdit::prefixPartialSelection(const QString &prefix)
{
    auto cur = textCursor();
    RaiiGroupUndoActions raii(cur);
    foreach_partly_selected_block(cur, [&prefix](const QTextCursor &line) -> void {
        if (lineHasTabs(line)) {
            expandTabs(line);
        }
        insertPrefix(line, prefix);
    });
}

void RemoteTextEdit::handleEventTab(QKeyEvent *const event)
{
    event->accept();
    auto cur = textCursor();
    if (cur.hasSelection()) {
        return prefixPartialSelection(string_consts::S_TWO_SPACES);
    }

    /* otherwise, insert a real tab, expand it, and restore the cursor */
    RaiiGroupUndoActions raii(cur);

    cur.insertText(string_consts::S_TAB);

    const int col_before = cur.positionInBlock();
    const auto &block = cur.block();
    const QString &s = block.text().left(col_before);
    const int col_after = mmqt::measureExpandedTabsOneLine(s, 0);
    expandTabs(cur);

    cur.setPosition(block.position() + col_after);
    setTextCursor(cur);
}

/* 2-space block de-indentation of all partly-selected lines */
void RemoteTextEdit::handleEventBacktab(QKeyEvent *const event)
{
    event->accept();
    auto cur = textCursor();
    RaiiGroupUndoActions raii(cur);
    foreach_partly_selected_block(cur, [](const QTextCursor &line) -> void {
        if (lineHasTabs(line)) {
            expandTabs(line);
        }
        tryRemoveLeadingSpaces(line, 2);
    });
}

void RemoteTextEdit::joinLines()
{
    auto cur = textCursor();
    RaiiGroupUndoActions raii(cur);

    auto doc = document();
    const auto from = doc->findBlock(cur.selectionStart());
    auto to = doc->findBlock(cur.selectionEnd());

    /* Feature: join the next line if only one line is selected */
    if (from == to) {
        if (cur.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor)) {
            to = doc->findBlock(cur.selectionEnd());
        }
    }

    mmqt::TextBuffer buffer;
    foreach_partly_selected_block(cur, [&buffer](const QTextCursor line) -> void {
        const QTextBlock block = line.block();
        const QString text = block.text();
        if (text.isEmpty()) {
            return;
        }

        if (!buffer.isEmpty()) {
            buffer.append(C_SPACE);
        }
        buffer.appendExpandedTabs(QStringView{text});
    });

    /* adjust the selection to cover select everything in the blocks */
    const int a = from.position();
    const int b = to.position() + to.length() - 1;
    cur.setPosition(a);
    cur.setPosition(b, QTextCursor::KeepAnchor);
    cur.insertText(buffer.getQString());
}

void RemoteTextEdit::quoteLines()
{
    prefixPartialSelection("> ");
}

void RemoteTextEdit::justifyLines(const int maxLen)
{
    auto cur = textCursor();
    RaiiGroupUndoActions raii(cur);

    auto doc = document();
    const auto from = doc->findBlock(cur.selectionStart());
    const auto to = doc->findBlock(cur.selectionEnd());
    const auto toBlockNumber = to.blockNumber();
    if (!from.isValid()) {
        return;
    }

    const int a = from.position();
    const int b = to.position() + to.length() - 1;

    mmqt::TextBuffer buffer;
    for (auto it = from; it.isValid() && it.blockNumber() <= toBlockNumber; it = it.next()) {
        const auto &text = it.text();
        if (text.isEmpty()) {
            continue;
        }

        if (!buffer.isEmpty()) {
            buffer.append(C_SPACE);
        }
        buffer.appendJustified(QStringView{text}, maxLen);
    }

    cur.setPosition(a);
    cur.setPosition(b, QTextCursor::KeepAnchor);
    cur.insertText(buffer.getQString());
}

void RemoteTextEdit::replaceAll(const QString &str)
{
    if ((false)) {
        /* this works */
        auto cur = this->textCursor();
        cur.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        cur.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        cur.insertText(str);
        cur.clearSelection();
    } else {
        /* but this is simpler */
        selectAll();
        insertPlainText(str);
    }
}

void RemoteTextEdit::showWhitespace(const bool enabled)
{
    auto *const doc = document();
    QTextOption option = doc->defaultTextOption();
    if (enabled) {
        option.setFlags(option.flags() | QTextOption::ShowTabsAndSpaces);
    } else {
        option.setFlags(option.flags() & ~QTextOption::ShowTabsAndSpaces);
    }
    option.setFlags(option.flags() | QTextOption::AddSpaceForLineAndParagraphSeparators);
    doc->setDefaultTextOption(option);
}

bool RemoteTextEdit::isShowingWhitespace() const
{
    auto *const doc = document();
    const QTextOption option = doc->defaultTextOption();
    return (option.flags() & QTextOption::ShowTabsAndSpaces) != 0;
}

void RemoteTextEdit::toggleWhitespace()
{
    showWhitespace(!isShowingWhitespace());
}

RemoteEditWidget::RemoteEditWidget(const bool editSession,
                                   QString title,
                                   QString body,
                                   QWidget *const parent)
    : QMainWindow(parent)
    , m_editSession(editSession)
    , m_title(std::move(title))
    , m_body(std::move(body))
{
    setWindowFlags(windowFlags() | Qt::WindowType::Widget);
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    mmqt::setWindowTitle2(*this,
                          QString("MMapper %1").arg(m_editSession ? "Editor" : "Viewer"),
                          m_title);

    // REVISIT: can this be called as an initializer?
    // Probably not. In fact this may be too early, since it accesses contentsMargins(),
    // which assumes this object is fully constructed and initialized.
    //
    // REVISIT: does this have to be QScopedPointer, or can it be std::unique_ptr?
    m_textEdit.reset(createTextEdit());

    // REVISIT: Restore geometry from config?
    setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                                    Qt::AlignCenter,
                                    size(),
                                    qApp->primaryScreen()->availableGeometry()));

    show();
    raise();
    activateWindow();
    m_textEdit->setFocus(); // REVISIT: can this be done in the creation function?
}

auto RemoteEditWidget::createTextEdit() -> Editor *
{
    QFont font;
    font.fromString(getConfig().integratedClient.font);
    const QFontMetrics fm(font);
    const int x = fm.averageCharWidth() * (80 + 1);
    const int y = fm.lineSpacing() * (24 + 1);

    const auto pTextEdit = new Editor(m_body, this);
    pTextEdit->setFont(font);
    pTextEdit->setReadOnly(!m_editSession);
    pTextEdit->setMinimumSize(QSize(x + contentsMargins().left() + contentsMargins().right(),
                                    y + contentsMargins().top() + contentsMargins().bottom()));
    pTextEdit->setLineWrapMode(Editor::LineWrapMode::NoWrap);
    pTextEdit->setSizeIncrement(fm.averageCharWidth(), fm.lineSpacing());
    pTextEdit->setTabStopDistance(fm.horizontalAdvance(" ") * 8); // A tab is 8 spaces wide
    pTextEdit->showWhitespace(false);

    auto *const doc = pTextEdit->document();
    new LineHighlighter(MAX_LENGTH, doc);

    setCentralWidget(pTextEdit);
    addStatusBar(pTextEdit);
    addFileMenu(menuBar(), pTextEdit);
    return pTextEdit;
}

void RemoteEditWidget::addFileMenu(QMenuBar *const menuBar, const Editor *const pTextEdit)
{
    QMenu *const fileMenu = menuBar->addMenu(tr("&File"));
    if (m_editSession) {
        addSave(fileMenu);
    }
    addExit(fileMenu);
    addEditAndViewMenus(menuBar, pTextEdit);
}

struct NODISCARD EditViewCommand final
{
    using mem_fn_ptr_type = void (RemoteEditWidget::*)();
    const mem_fn_ptr_type mem_fn_ptr;
    const EditViewCmdEnum cmd_type;
    const char *const action;
    const char *const status;
    const char *const shortcut;

    constexpr explicit EditViewCommand(const mem_fn_ptr_type _mem_fn_ptr,
                                       const EditViewCmdEnum _cmd_type,
                                       const char *const _action,
                                       const char *const _status,
                                       const char *const _shortcut)
        : mem_fn_ptr{_mem_fn_ptr}
        , cmd_type{_cmd_type}
        , action{_action}
        , status{_status}
        , shortcut{_shortcut}
    {}
};

struct NODISCARD EditCommand2 final
{
    using mem_fn_ptr_type = void (RemoteTextEdit::*)();
    mem_fn_ptr_type mem_fn_ptr = nullptr;
    const char *theme = nullptr;
    const char *icon = nullptr;
    const char *action = nullptr;
    const char *status = nullptr;
    const char *shortcut = nullptr;
    EditCmd2Enum cmd_type = EditCmd2Enum::SPACER;

    constexpr EditCommand2() = default;
    constexpr explicit EditCommand2(const mem_fn_ptr_type _mem_fn_ptr,
                                    const char *const _theme,
                                    const char *const _icon,
                                    const char *const _action,
                                    const char *const _status,
                                    const char *const _shortcut,
                                    const EditCmd2Enum _cmd_type)
        : mem_fn_ptr{_mem_fn_ptr}
        , theme{_theme}
        , icon{_icon}
        , action{_action}
        , status{_status}
        , shortcut{_shortcut}
        , cmd_type{_cmd_type}
    {}
};

void RemoteEditWidget::addEditAndViewMenus(QMenuBar *const menuBar, const Editor *const pTextEdit)
{
    QMenu *const editMenu = menuBar->addMenu("&Edit");
    QMenu *const viewMenu = menuBar->addMenu("&View");

    const std::vector<EditCommand2> cmds{

        EditCommand2{&Editor::undo,
                     "edit-undo",
                     ":/icons/undo.png",
                     "&Undo",
                     "Undo previous typing or commands",
                     "Ctrl+Z",
                     EditCmd2Enum::EDIT_ONLY},
        EditCommand2{&Editor::redo,
                     "edit-redo",
                     ":/icons/redo.png",
                     "&Redo",
                     // REVISIT: Ctrl+Y vs Ctrl+Shift+Z may depend on platform conventions;
                     // it's Ctrl+Shift+Z by default on Ubuntu MATE,
                     // but it might be Ctrl+Y by default on other desktops?
                     "Redo previous typing or commands",
                     "Ctrl+Shift+Z",
                     EditCmd2Enum::EDIT_ONLY},
        EditCommand2{},
        EditCommand2{&Editor::selectAll,
                     "edit-select-all",
                     ":/icons/redo.png",
                     "&Select All",
                     "Select the entire document",
                     "Ctrl+A",
                     EditCmd2Enum::EDIT_OR_VIEW},
        EditCommand2{},
        EditCommand2{&Editor::cut,
                     "edit-cut",
                     ":/icons/cut.png",
                     "Cu&t",
                     "Cut the current selection's contents to the clipboard",
                     "Ctrl+X",
                     EditCmd2Enum::EDIT_ONLY},
        EditCommand2{&Editor::copy,
                     "edit-copy",
                     ":/icons/copy.png",
                     "&Copy",
                     "Copy the current selection's contents to the clipboard",
                     "Ctrl+C",
                     EditCmd2Enum::EDIT_OR_VIEW},
        EditCommand2{&Editor::paste,
                     "edit-paste",
                     ":/icons/paste.png",
                     "&Paste",
                     "Paste the clipboard's contents into the current selection",
                     "Ctrl+V",
                     EditCmd2Enum::EDIT_ONLY},

        EditCommand2{}};

    for (const EditCommand2 &cmd : cmds) {
        switch (cmd.cmd_type) {
        case EditCmd2Enum::SPACER:
            editMenu->addSeparator();
            break;
        case EditCmd2Enum::EDIT_ONLY:
            if (m_editSession) {
                addToMenu(editMenu, cmd, pTextEdit);
            }
            break;
        case EditCmd2Enum::EDIT_OR_VIEW:
            addToMenu(editMenu, cmd, pTextEdit);
            break;
        }
    }

    // Note: "&Colors" looks like it conflicts with "&Copy",
    // but you can alt-E-C-C to visit copy first then colors.
    QMenu *const alignmentMenu = m_editSession ? editMenu->addMenu("&Alignment") : nullptr;
    QMenu *const colorsMenu = m_editSession ? editMenu->addMenu("&Colors") : nullptr;
    QMenu *const whitespaceMenu = m_editSession ? editMenu->addMenu("&Whitespace") : nullptr;

    const auto getMenu = [alignmentMenu, colorsMenu, editMenu, viewMenu, whitespaceMenu](
                             const EditViewCmdEnum cmd) -> QMenu * {
        switch (cmd) {
        case EditViewCmdEnum::VIEW_OPTION:
            return viewMenu;
        case EditViewCmdEnum::EDIT_ALIGNMENT:
            return alignmentMenu;
        case EditViewCmdEnum::EDIT_COLORS:
            return colorsMenu;
        case EditViewCmdEnum::EDIT_WHITESPACE:
            return whitespaceMenu;
        default:
            return editMenu;
        }
    };
#define X_ADD_MENU(a, b, c, d, e) \
    if (auto menu = getMenu(b)) { \
        addToMenu(menu, EditViewCommand{&RemoteEditWidget::slot_##a, (b), (c), (d), (e)}); \
    }
    XFOREACH_REMOTE_EDIT_MENU_ITEM(X_ADD_MENU)
#undef X_ADD_MENU
}

void RemoteEditWidget::addSave(QMenu *const fileMenu)
{
    // REVISIT: Add option to require confirmation, especially if the user didn't
    // change anything.
    QAction *const saveAction = new QAction(QIcon::fromTheme("document-save",
                                                             QIcon(":/icons/save.png")),
                                            tr("&Submit"),
                                            this);
    saveAction->setShortcut(tr("Ctrl+S"));
    saveAction->setStatusTip(tr("Submit changes to MUME"));
    fileMenu->addAction(saveAction);
    connect(saveAction, &QAction::triggered, this, &RemoteEditWidget::slot_finishEdit);
}

void RemoteEditWidget::addExit(QMenu *const fileMenu)
{
    // FIXME: This should require confirmation if the user has modified the document,
    // and possibly even if ANY change has been made (even an undone change).
    if ((false)) {
        auto *const doc = m_textEdit->document();
        MAYBE_UNUSED const bool isModified = doc->isModified();
        MAYBE_UNUSED const bool canUndo = doc->isUndoAvailable();
    }
    QAction *const quitAction = new QAction(QIcon::fromTheme("window-close",
                                                             QIcon(":/icons/exit.png")),
                                            tr("E&xit"),
                                            this);
    quitAction->setShortcut(tr("Ctrl+Q"));
    quitAction->setStatusTip(tr("Cancel and do not submit changes to MUME"));
    fileMenu->addAction(quitAction);
    connect(quitAction, &QAction::triggered, this, &RemoteEditWidget::slot_cancelEdit);
}

void RemoteEditWidget::addToMenu(QMenu *const menu, const EditViewCommand &cmd)
{
    if (menu == nullptr) {
        assert(false);
        return;
    }

    QAction *const act = new QAction(tr(cmd.action), this);
    if (cmd.shortcut != nullptr) {
        act->setShortcut(tr(cmd.shortcut));
    }
    act->setStatusTip(tr(cmd.status));
    menu->addAction(act);
    connect(act, &QAction::triggered, this, cmd.mem_fn_ptr);
}

void RemoteEditWidget::addToMenu(QMenu *const menu,
                                 const EditCommand2 &cmd,
                                 const Editor *const pTextEdit)
{
    if (menu == nullptr) {
        assert(false);
        return;
    }

    QAction *const act = new QAction(QIcon::fromTheme(cmd.theme, QIcon(cmd.icon)),
                                     tr(cmd.action),
                                     this);
    if (cmd.shortcut != nullptr) {
        act->setShortcut(tr(cmd.shortcut));
    }
    act->setStatusTip(tr(cmd.status));
    menu->addAction(act);

    connect(act, &QAction::triggered, pTextEdit, cmd.mem_fn_ptr);
}

void RemoteEditWidget::addStatusBar(const Editor *const pTextEdit)
{
    QStatusBar *const status = statusBar();
    status->showMessage(tr("Ready"));

    connect(pTextEdit,
            &QPlainTextEdit::cursorPositionChanged,
            this,
            &RemoteEditWidget::slot_updateStatusBar);
    connect(pTextEdit,
            &QPlainTextEdit::selectionChanged,
            this,
            &RemoteEditWidget::slot_updateStatusBar);
    connect(pTextEdit, &QPlainTextEdit::textChanged, this, &RemoteEditWidget::slot_updateStatusBar);
}

struct NODISCARD CursorColumnInfo final
{
    int actual = 0;
    int tab_aware = 0;
    int tab_and_ansi_aware = 0;
};

NODISCARD static CursorColumnInfo getCursorColumn(QTextCursor &cursor)
{
    const int pos = cursor.positionInBlock();
    const auto &block = cursor.block();
    const auto &text = block.text().left(pos);

    CursorColumnInfo cci;
    cci.actual = pos;
    cci.tab_aware = mmqt::measureExpandedTabsOneLine(text, 0);
    cci.tab_and_ansi_aware = mmqt::measureTabAndAnsiAware(text);
    return cci;
}

struct NODISCARD CursorAnsiInfo final
{
    mmqt::TextBuffer buffer;
    // RawAnsi ansi;

    explicit operator bool() const { return !buffer.isEmpty(); }
};

NODISCARD static CursorAnsiInfo getCursorAnsi(QTextCursor cursor)
{
    const int pos = cursor.positionInBlock();

    CursorAnsiInfo result;
    const auto &line = cursor.block().text();
    mmqt::foreachAnsi(line, [pos, &result](int start, const QStringView sv) {
        if (result || pos < start || pos >= start + sv.length()) {
            return;
        }

        if (!mmqt::isValidAnsiColor(sv)) {
            result.buffer.append("*invalid*");
            // result.ansi = {};
            return;
        }

        auto &buffer = result.buffer;

        if (const auto optAnsi = mmqt::parseAnsiColor(RawAnsi{}, sv)) {
            const auto &ansi = *optAnsi;
            // result.ansi = ansi;
            if (ansi == RawAnsi{}) {
                buffer.append("reset");
            } else {
                std::ostringstream oss;
                to_stream(oss, ansi);
                buffer.append(mmqt::toQStringUtf8(oss.str()));
            }
        } else {
            result.buffer.append("*error");
            // result.ansi = {};
        }
    });
    return result;
}

NODISCARD static bool linesHaveTabs(const QTextCursor &cur)
{
    return exists_partly_selected_block(cur, [](const QTextCursor &it) -> bool {
        return lineHasTabs(it);
    });
}

NODISCARD static bool linesHaveTrailingSpace(const QTextCursor &cur)
{
    return exists_partly_selected_block(cur, [](const QTextCursor &it) -> bool {
        const auto &block = it.block();
        const auto &line = block.text();
        return mmqt::findTrailingWhitespace(line) >= 0;
    });
}

NODISCARD static bool hasLongLines(const QTextCursor &cur)
{
    return exists_partly_selected_block(cur, [](const QTextCursor &line) -> bool {
        const auto &block = line.block();
        const auto &s = block.text();
        return mmqt::measureTabAndAnsiAware(s) > 80;
    });
}

void RemoteEditWidget::slot_updateStatusBar()
{
    auto cur = m_textEdit->textCursor();

    const auto cci = getCursorColumn(cur);
    const int row = cur.blockNumber() + 1;
    QString status = QString("Line %1, Column %2").arg(row).arg(cci.tab_and_ansi_aware + 1);
    if (cci.tab_aware != cci.tab_and_ansi_aware) {
        status.append(QString(" (non-ansi-aware: %1)").arg(cci.tab_aware + 1));
    }

    if (cur.hasSelection()) {
        const auto plural = [](auto n) { return (n == 1) ? "" : "s"; };

        const QString selection = cur.selection().toPlainText();
        const int selectionLength = selection.length();
        const int selectionLines = selection.count(C_NEWLINE)
                                   + (selection.endsWith(C_NEWLINE) ? 0 : 1);

        status.append(QString(", Selection: %1 char%2 on %3 line%4")
                          .arg(selectionLength)
                          .arg(plural(selectionLength))
                          .arg(selectionLines)
                          .arg(plural(selectionLines)));
    } else if (auto ansi = getCursorAnsi(cur)) {
        status.append(QString(", AnsiCode: {%1}").arg(ansi.buffer.getQString()));
    }

    const auto report_errors = [&cur, &status]() {
        const bool hasSelection = cur.hasSelection();
        if (!hasSelection) {
            cur.select(QTextCursor::Document);
        }

        bool first = true;
        const auto err = [&first, &hasSelection, &status](auto &&x) {
            if (first) {
                status.append(hasSelection ? ", Selected-Lines-Err: " : ", Document-Err: ");
                first = false;
            } else {
                status.append(", ");
            }
            status.append(std::forward<decltype(x)>(x));
        };

        if (linesHaveTabs(cur)) {
            err("Tabs");
        }

        if (linesHaveTrailingSpace(cur)) {
            err("Trailing-Spaces");
        }

        if (hasLongLines(cur)) {
            err("Long-lines");
        }

        if (first) {
            status.append(hasSelection ? ", Selected-Lines: Ok" : ", Document: Ok");
        }
    };
    report_errors();
    statusBar()->showMessage(status);
}

void RemoteEditWidget::slot_justifyText()
{
    const QString &old = m_textEdit->toPlainText();
    mmqt::TextBuffer text;
    text.reserve(2 * old.length()); // Just a wild guess in case there's a lot of wrapping.
    mmqt::foreachLine(old,
                      [&text, maxLen = MAX_LENGTH](const QStringView line, bool /*hasNewline*/) {
                          text.appendJustified(line, maxLen);
                          text.append(C_NEWLINE);
                      });
    m_textEdit->replaceAll(text.getQString());
}

// TODO: Set the tabstops for the edit control to 8 spaces, so the text won't jump when you expand tabs.
void RemoteEditWidget::slot_expandTabs()
{
    const QTextCursor &cur = m_textEdit->textCursor();
    RaiiGroupUndoActions raii{cur};

    foreach_partly_selected_block(cur, [](QTextCursor line) -> void {
        if (lineHasTabs(line)) {
            ::expandTabs(line);
        }
    });
}

void RemoteEditWidget::slot_removeTrailingWhitespace()
{
    const QTextCursor &cur = m_textEdit->textCursor();
    RaiiGroupUndoActions raii{cur};

    foreach_partly_selected_block(cur, [](QTextCursor line) -> void {
        static const QRegularExpression trailingWhitespace(R"([[:space:]]+$)");
        assert(trailingWhitespace.isValid());
        QString text = line.block().text();
        if (!text.contains(trailingWhitespace)) {
            return;
        }

        text.remove(trailingWhitespace);
        line.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        line.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        line.insertText(text);
    });
}

void RemoteEditWidget::slot_removeDuplicateSpaces()
{
    const QTextCursor &cur = m_textEdit->textCursor();
    RaiiGroupUndoActions raii{cur};

    foreach_partly_selected_block(cur, [](QTextCursor line) -> void {
        static const QRegularExpression spaces(R"((\t|[[:space:]]{2,}))");
        assert(spaces.isValid());
        QString text = line.block().text();
        if (!text.contains(spaces)) {
            return;
        }

        text.replace(spaces, string_consts::S_SPACE);
        line.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        line.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        line.insertText(text);
    });
}

void RemoteEditWidget::slot_normalizeAnsi()
{
    const QString &old = m_textEdit->toPlainText();
    if (!mmqt::containsAnsi(old)) {
        return;
    }

    mmqt::TextBuffer output = mmqt::normalizeAnsi(ANSI_COLOR_SUPPORT_HI, old);
    if (!output.hasTrailingNewline()) {
        output.append(C_NEWLINE);
    }

    m_textEdit->replaceAll(output.getQString());
}

void RemoteEditWidget::slot_previewAnsi()
{
    QString s = m_textEdit->toPlainText();
    if (!s.endsWith(C_NEWLINE)) {
        s += C_NEWLINE;
    }

    m_preview = makeAnsiViewWindow("MMapper Editor Preview", m_title, mmqt::toStdStringUtf8(s));
}

void RemoteEditWidget::slot_insertAnsiReset()
{
    const auto reset = AnsiString::get_reset_string();
    m_textEdit->insertPlainText(reset.c_str());
}

void RemoteEditWidget::slot_joinLines()
{
    m_textEdit->joinLines();
}

void RemoteEditWidget::slot_quoteLines()
{
    m_textEdit->quoteLines();
}

void RemoteEditWidget::slot_toggleWhitespace()
{
    m_textEdit->toggleWhitespace();
}

void RemoteEditWidget::slot_justifyLines()
{
    m_textEdit->justifyLines(MAX_LENGTH);
}

RemoteEditWidget::~RemoteEditWidget()
{
    qInfo() << "Destroyed RemoteEditWidget" << m_title;
}

QSize RemoteEditWidget::minimumSizeHint() const
{
    return QSize{100, 100};
}

QSize RemoteEditWidget::sizeHint() const
{
    return QSize{640, 480};
}

void RemoteEditWidget::closeEvent(QCloseEvent *event)
{
    if (m_editSession) {
        if (m_submitted || slot_maybeCancel()) {
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        slot_cancelEdit();
        event->accept();
    }
}

bool RemoteEditWidget::slot_maybeCancel()
{
    if (slot_contentsChanged()) {
        const int ret = QMessageBox::warning(this,
                                             m_title,
                                             tr("You have edited the document.\n"
                                                "Do you want to cancel your changes?"),
                                             QMessageBox::Yes,
                                             QMessageBox::No | QMessageBox::Escape
                                                 | QMessageBox::Default);
        if (ret == QMessageBox::No) {
            return false;
        }
    }

    slot_cancelEdit();
    return true;
}

bool RemoteEditWidget::slot_contentsChanged() const
{
    const QString text = m_textEdit->toPlainText();
    return QString::compare(text, m_body, Qt::CaseSensitive) != 0;
}

void RemoteEditWidget::slot_cancelEdit()
{
    m_submitted = true;
    emit sig_cancel();
    close();
}

void RemoteEditWidget::slot_finishEdit()
{
    m_submitted = true;
    emit sig_save(m_textEdit->toPlainText());
    close();
}

void RemoteEditWidget::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if (visible) {
        slot_updateStatusBar();
    }
}
