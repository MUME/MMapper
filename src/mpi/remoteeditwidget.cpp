// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "remoteeditwidget.h"

#include <cassert>
#include <cctype>
#include <memory>
#include <optional>
#include <utility>
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

#include "../configuration/configuration.h"
#include "../global/RAII.h"
#include "../global/TextUtils.h"
#include "../global/entities.h"
#include "../global/utils.h"

static constexpr const bool USE_TOOLTIPS = false;
static constexpr const char S_TWO_SPACES[3]{C_SPACE, C_SPACE, C_NUL};
// REVISIT: Figure out how to tweak logic to accept actual maximum length of 80
static constexpr const int MAX_LENGTH = 79;

static int measureTabAndAnsiAware(const QString &s)
{
    int col = 0;
    for (auto token : AnsiTokenizer{s}) {
        using Type = AnsiStringToken::TokenTypeEnum;
        switch (token.type) {
        case Type::ANSI:
            break;
        case Type::NEWLINE:
            /* unexpected */
            col = 0;
            break;
        case Type::SPACE:
        case Type::CONTROL:
        case Type::WORD:
            col = measureExpandedTabsOneLine(token.getQStringRef(), col);
            break;
        }
    }
    return col;
}

class QWidget;

/// Groups everything in the scope as a single undo action.
class NODISCARD RaiiGroupUndoActions final
{
private:
    QTextCursor cursor_;

public:
    explicit RaiiGroupUndoActions(QTextCursor _cursor)
        : cursor_{std::move(_cursor)}
    {
        cursor_.beginEditBlock();
    }
    ~RaiiGroupUndoActions() { cursor_.endEditBlock(); }
};

class LineHighlighter final : public QSyntaxHighlighter
{
public:
    explicit LineHighlighter(int maxLength, QTextDocument *parent);
    virtual ~LineHighlighter() override;

    void highlightBlock(const QString &line) override
    {
        highlightTabs(line);
        highlightOverflow(line);
        highlightTrailingSpace(line);
        highlightAnsi(line);
        highlightEntities(line);
        highlightEncodingErrors(line);
    }

    static QTextCharFormat getBackgroundFormat(const Qt::GlobalColor color)
    {
        QTextCharFormat fmt;
        fmt.setBackground(QBrush{color});
        return fmt;
    }
    static QTextCharFormat getBackgroundFormat(const QColor color)
    {
        QTextCharFormat fmt;
        fmt.setBackground(QBrush{color});
        return fmt;
    }

    void highlightTabs(const QString &line)
    {
        if (line.indexOf('\t') < 0)
            return;

        const auto fmt = getBackgroundFormat(Qt::yellow);
        foreachChar(line, '\t', [this, &fmt](const int at) { setFormat(at, 1, fmt); });
    }

    void highlightOverflow(const QString &line)
    {
        const int breakPos = (measureTabAndAnsiAware(line) <= maxLength) ? -1 : maxLength;
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
        const int breakPos = findTrailingWhitespace(line);
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

        foreachAnsi(line, [this, &red, &cyan](const int start, const QStringRef &ref) {
            setFormat(start, ref.length(), isValidAnsiColor(ref) ? cyan : red);
        });
    }

    void highlightEntities(const QString &line)
    {
        const auto red = getBackgroundFormat(Qt::red);
        const auto darkOrange = getBackgroundFormat(QColor{0xff, 0x8c, 0x00});
        const auto yellow = getBackgroundFormat(Qt::yellow);

        struct MyEntityCallback : entities::EntityCallback
        {
            LineHighlighter &self;
            const QTextCharFormat &red;
            const QTextCharFormat &darkOrange;
            const QTextCharFormat &yellow;

            explicit MyEntityCallback(LineHighlighter &self,
                                      const QTextCharFormat &red,
                                      const QTextCharFormat &darkOrange,
                                      const QTextCharFormat &yellow)
                : self{self}
                , red{red}
                , darkOrange{darkOrange}
                , yellow{yellow}
            {}

            void decodedEntity(int start, int len, OptQChar decoded) override
            {
                const auto get_fmt = [this, &decoded]() -> const QTextCharFormat & {
                    if (!decoded)
                        return red;
                    const auto val = decoded->unicode();
                    return (val < 256) ? yellow : darkOrange;
                };

                self.setFormat(start, len, get_fmt());
            }
        } callback{*this, red, darkOrange, yellow};
        entities::foreachEntity(line.midRef(0), callback);
    }

    void highlightEncodingErrors(const QString &line)
    {
        const auto red = getBackgroundFormat(Qt::red);
        const auto darkOrange = getBackgroundFormat(QColor{0xff, 0x8c, 0x00});
        const auto yellow = getBackgroundFormat(Qt::yellow);
        const auto cyan = getBackgroundFormat(Qt::cyan);

        using OptFmt = std::pair<QTextCharFormat, bool>; // poor-man's std::optional
#define DECL_FMT(name, color, tooltip) \
    OptFmt opt_##name{}; \
    const auto get_##name##_fmt = [&opt_##name, &color]() -> const QTextCharFormat & { \
        auto &first = opt_##name.first; \
        auto &second = opt_##name.second; \
        if (!second) { \
            second = true; \
            first = color; \
            if (USE_TOOLTIPS) \
                first.setToolTip(tooltip); \
        } \
        return first; \
    };
        DECL_FMT(unicode, red, "Unicode");
        DECL_FMT(nbsp, cyan, "NBSP");
        DECL_FMT(utf8, yellow, "UTF-8");
        DECL_FMT(unprintable, darkOrange, "(unprintable)")

#undef DECL_FMT

        int pos = 0;
        QChar last;
        bool hasLast = false;
        for (const auto &qc : line) {
            if (qc.unicode() > 0xff) {
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
                default:
                    if (hasLast
                        && (isClamped<int>(static_cast<unsigned char>(c), 0x80, 0xbf)
                            && (last == 0xc2 || last == 0xc3))) {
                        // Sometimes these are UTF-8 encoded Latin1 values,
                        // but they could also be intended, so they're not errors.
                        // TODO: add a feature to fix these on a case-by-case basis?
                        setFormat(pos - 1, 2, get_utf8_fmt());
                    } else if (std::iscntrl(c) || (!std::isprint(c) && !std::isspace(c))) {
                        setFormat(pos, 1, get_unprintable_fmt());
                    }
                }
            }

            last = qc;
            hasLast = true;
            ++pos;
        }
    }

private:
    const int maxLength;
};

LineHighlighter::LineHighlighter(int maxLength, QTextDocument *const parent)
    : QSyntaxHighlighter(parent)
    , maxLength(maxLength)
{}

LineHighlighter::~LineHighlighter() = default;

RemoteTextEdit::RemoteTextEdit(const QString &initialText, QWidget *const parent)
    : base(parent)
{
    base::setPlainText(initialText);
}
RemoteTextEdit::~RemoteTextEdit() = default;

void RemoteTextEdit::keyPressEvent(QKeyEvent *const event)
{
    switch (event->key()) {
    case Qt::Key_Tab:
        return handleEventTab(event);
    case Qt::Key_Backtab:
        return handleEventBacktab(event);
    default:
        return base::keyPressEvent(event);
    }
}

bool RemoteTextEdit::event(QEvent *const event)
{
    if ((USE_TOOLTIPS))
        if (event->type() == QEvent::ToolTip) {
            handle_toolTip(event);
            return true;
        }
    return base::event(event);
}

void RemoteTextEdit::handle_toolTip(QEvent *const event) const
{
    QHelpEvent *const helpEvent = static_cast<QHelpEvent *>(event);
    QTextCursor cursor = cursorForPosition(helpEvent->pos());
    if ((false))
        cursor.select(QTextCursor::WordUnderCursor);
    else {
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
    std::snprintf(unicode_buf, sizeof(unicode_buf), "U+%04x", static_cast<int>(c.unicode()));

    // REVISIT: Why doesn't this get the tooltip we applied in the LineHighlighter?
    // Apparently it's a reported bug that Qt refuses to acknowledge.
    // TODO: custom tooltips for error reporting and URLs
    QToolTip::showText(helpEvent->globalPos(), QString("%1 (%2)").arg(tooltip).arg(unicode_buf));
}

static bool lineHasTabs(const QTextCursor &line)
{
    if (line.isNull())
        return false;
    const auto &block = line.block();
    if (!block.isValid())
        return false;
    return block.text().indexOf(C_TAB) >= 0;
}

static void expandTabs(QTextCursor line)
{
    const QTextBlock &block = line.block();

    TextBuffer prefix;
    prefix.appendExpandedTabs(block.text().midRef(0), 0);

    line.setPosition(block.position());
    line.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    line.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    line.insertText(prefix.getQString());
}

static void tryRemoveLeadingSpaces(QTextCursor line, const int max_spaces)
{
    const auto &block = line.block();
    if (!block.isValid())
        return;

    const auto &text = block.text();
    if (text.isEmpty())
        return;

    const int to_remove = [&text, max_spaces]() -> int {
        const int len = std::min(max_spaces, text.length());
        int n = 0;
        while (n < len && text.at(n) == C_SPACE)
            ++n;
        return n;
    }();

    if (to_remove == 0)
        return;

    line.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    line.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, to_remove);
    line.removeSelectedText();
}

struct LineRange
{
    static auto beg(QTextCursor cur)
    {
        auto from = cur.document()->findBlock(cur.selectionStart());
        auto begCursor = QTextCursor(from);
        return begCursor;
    }

    static auto end(QTextCursor cur)
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

enum class CallbackResultEnum { KEEP_GOING, STOP };

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
            if (callback(static_cast<const QTextCursor &>(it)) == CallbackResultEnum::STOP)
                return;
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
        if (lineHasTabs(line))
            expandTabs(line);
        insertPrefix(line, prefix);
    });
}

void RemoteTextEdit::handleEventTab(QKeyEvent *const event)
{
    event->accept();
    auto cur = textCursor();
    if (cur.hasSelection())
        return prefixPartialSelection(S_TWO_SPACES);

    /* otherwise, insert a real tab, expand it, and restore the cursor */
    RaiiGroupUndoActions raii(cur);

    cur.insertText(S_TAB);

    const auto block = cur.block();
    const int col_before = cur.positionInBlock();
    const QStringRef &ref = block.text().leftRef(col_before);
    const int col_after = measureExpandedTabsOneLine(ref, 0);
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
        if (lineHasTabs(line))
            expandTabs(line);
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

    TextBuffer buffer;
    foreach_partly_selected_block(cur, [&buffer](const auto &line) -> void {
        const auto &block = line.block();
        const auto &text = block.text();
        if (text.isEmpty())
            return;
        if (!buffer.isEmpty())
            buffer.append(C_SPACE);
        buffer.appendExpandedTabs(text.midRef(0));
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
    if (!from.isValid())
        return;

    const int a = from.position();
    const int b = to.position() + to.length() - 1;

    TextBuffer buffer;
    for (auto it = from; it.isValid() && it.blockNumber() <= toBlockNumber; it = it.next()) {
        const auto &text = it.text();
        if (text.isEmpty())
            continue;
        if (!buffer.isEmpty())
            buffer.append(C_SPACE);
        buffer.appendJustified(text.midRef(0), maxLen);
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
    if (enabled)
        option.setFlags(option.flags() | QTextOption::ShowTabsAndSpaces);
    else
        option.setFlags(option.flags() & ~QTextOption::ShowTabsAndSpaces);
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
                                   const QString &title,
                                   const QString &body,
                                   QWidget *const parent)
    : QMainWindow(parent)
    , m_editSession(editSession)
    , m_title(title)
    , m_body(body)
{
    setWindowFlags(windowFlags() | Qt::WindowType::Widget);
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(m_title + " - MMapper " + (m_editSession ? "Editor" : "Viewer"));

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
    pTextEdit->setTabStopWidth(fm.width(" ") * 8); // A tab is 8 spaces wide
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

struct EditViewCommand final
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

struct EditCommand2 final
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

    for (auto &cmd : cmds) {
        switch (cmd.cmd_type) {
        case EditCmd2Enum::SPACER:
            editMenu->addSeparator();
            break;
        case EditCmd2Enum::EDIT_ONLY:
            if (m_editSession)
                addToMenu(editMenu, cmd, pTextEdit);
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

    const auto getMenu = [=](EditViewCmdEnum cmd) -> QMenu * {
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
#define X(a, b, c, d, e) \
    if (auto menu = getMenu(b)) { \
        addToMenu(menu, EditViewCommand{&RemoteEditWidget::a, (b), (c), (d), (e)}); \
    }
    XFOREACH_REMOTE_EDIT_MENU_ITEM(X)
#undef X
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
    connect(saveAction, &QAction::triggered, this, &RemoteEditWidget::finishEdit);
}

void RemoteEditWidget::addExit(QMenu *const fileMenu)
{
    // FIXME: This should require confirmation if the user has modified the document,
    // and possibly even if ANY change has been made (even an undone change).
    if ((false)) {
        auto *const doc = m_textEdit->document();
        bool isModified = doc->isModified();
        bool canUndo = doc->isUndoAvailable();
    }
    QAction *const quitAction = new QAction(QIcon::fromTheme("window-close",
                                                             QIcon(":/icons/exit.png")),
                                            tr("E&xit"),
                                            this);
    quitAction->setShortcut(tr("Ctrl+Q"));
    quitAction->setStatusTip(tr("Cancel and do not submit changes to MUME"));
    fileMenu->addAction(quitAction);
    connect(quitAction, &QAction::triggered, this, &RemoteEditWidget::cancelEdit);
}

void RemoteEditWidget::addToMenu(QMenu *const menu, const EditViewCommand &cmd)
{
    if (menu == nullptr) {
        assert(false);
        return;
    }

    QAction *const act = new QAction(tr(cmd.action), this);
    if (cmd.shortcut != nullptr)
        act->setShortcut(tr(cmd.shortcut));
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
    if (cmd.shortcut != nullptr)
        act->setShortcut(tr(cmd.shortcut));
    act->setStatusTip(tr(cmd.status));
    menu->addAction(act);

    connect(act, &QAction::triggered, pTextEdit, cmd.mem_fn_ptr);
}

void RemoteEditWidget::addStatusBar(const Editor *const pTextEdit)
{
    QStatusBar *const status = statusBar();
    status->showMessage(tr("Ready"));

    connect(pTextEdit, &Editor::cursorPositionChanged, this, &RemoteEditWidget::updateStatusBar);
    connect(pTextEdit, &Editor::selectionChanged, this, &RemoteEditWidget::updateStatusBar);
    connect(pTextEdit, &Editor::textChanged, this, &RemoteEditWidget::updateStatusBar);
}

struct CursorColumnInfo final
{
    int actual = 0;
    int tab_aware = 0;
    int tab_and_ansi_aware = 0;
};

static CursorColumnInfo getCursorColumn(QTextCursor &cursor)
{
    const int pos = cursor.positionInBlock();
    const auto &block = cursor.block();
    const auto &text = block.text().left(pos);

    CursorColumnInfo cci;
    cci.actual = pos;
    cci.tab_aware = measureExpandedTabsOneLine(text, 0);
    cci.tab_and_ansi_aware = measureTabAndAnsiAware(text);
    return cci;
}

struct CursorAnsiInfo final
{
    TextBuffer buffer;
    raw_ansi ansi;

    explicit operator bool() const { return !buffer.isEmpty(); }
};

static CursorAnsiInfo getCursorAnsi(QTextCursor cursor)
{
    const int pos = cursor.positionInBlock();

    CursorAnsiInfo result;
    const auto &line = cursor.block().text();
    foreachAnsi(line, [pos, &result](int start, const QStringRef &ref) {
        if (result || pos < start || pos >= start + ref.length())
            return;

        if (!isValidAnsiColor(ref)) {
            result.buffer.append("*invalid*");
            return;
        }

        bool first = true;
        Ansi ansi;
        AnsiColorParser::for_each_code(ref, [&ansi, &first, &result](int code) {
            ansi.process_code(code);
            if (first)
                first = false;
            else
                result.buffer.append(", ");
            result.buffer.append(raw_ansi::describe(code));
        });
        result.ansi = ansi.get_raw();
    });
    return result;
}

static bool linesHaveTabs(const QTextCursor &cur)
{
    return exists_partly_selected_block(cur, [](const QTextCursor &it) -> bool {
        return lineHasTabs(it);
    });
}

static bool linesHaveTrailingSpace(const QTextCursor &cur)
{
    return exists_partly_selected_block(cur, [](const QTextCursor &it) -> bool {
        const auto &block = it.block();
        const auto &line = block.text();
        return findTrailingWhitespace(line) >= 0;
    });
}

static bool hasLongLines(const QTextCursor &cur)
{
    return exists_partly_selected_block(cur, [](const QTextCursor &line) -> bool {
        const auto &block = line.block();
        const auto &s = block.text();
        return measureTabAndAnsiAware(s) > 80;
    });
}

void RemoteEditWidget::updateStatusBar()
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
        const int selectionLines = selection.count('\n') + (selection.endsWith('\n') ? 0 : 1);

        status.append(QString(", Selection: %1 char%2 on %3 line%4")
                          .arg(selectionLength)
                          .arg(plural(selectionLength))
                          .arg(selectionLines)
                          .arg(plural(selectionLines)));
    } else if (auto ansi = getCursorAnsi(cur)) {
        status.append(QString(", AnsiCode: {%1}").arg(ansi.buffer.getQString()));
    }

    const auto report_errors = [&status](QTextCursor cur) {
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

        if (linesHaveTabs(cur))
            err("Tabs");

        if (linesHaveTrailingSpace(cur))
            err("Trailing-Spaces");

        if (hasLongLines(cur))
            err("Long-lines");

        if (first)
            status.append(hasSelection ? ", Selected-Lines: Ok" : ", Document: Ok");
    };
    report_errors(cur);
    statusBar()->showMessage(status);
}

void RemoteEditWidget::justifyText()
{
    const QString &old = m_textEdit->toPlainText();
    TextBuffer text;
    text.reserve(2 * old.length()); // Just a wild guess in case there's a lot of wrapping.
    foreachLine(old, [&text, maxLen = MAX_LENGTH](const QStringRef &line, bool /*hasNewline*/) {
        text.appendJustified(line, maxLen);
        text.append('\n');
    });
    m_textEdit->replaceAll(text.getQString());
}

// TODO: Set the tabstops for the edit control to 8 spaces, so the text won't jump when you expand tabs.
void RemoteEditWidget::expandTabs()
{
    const QTextCursor &cur = m_textEdit->textCursor();
    RaiiGroupUndoActions raii{cur};

    foreach_partly_selected_block(cur, [](QTextCursor line) -> void {
        if (lineHasTabs(line))
            ::expandTabs(line);
    });
}

void RemoteEditWidget::removeTrailingWhitespace()
{
    const QTextCursor &cur = m_textEdit->textCursor();
    RaiiGroupUndoActions raii{cur};

    foreach_partly_selected_block(cur, [](QTextCursor line) -> void {
        static QRegularExpression trailingWhitespace(R"([[:space:]]+$)");
        assert(trailingWhitespace.isValid());
        QString text = line.block().text();
        if (!text.contains(trailingWhitespace))
            return;

        text.remove(trailingWhitespace);
        line.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        line.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        line.insertText(text);
    });
}

void RemoteEditWidget::removeDuplicateSpaces()
{
    const QTextCursor &cur = m_textEdit->textCursor();
    RaiiGroupUndoActions raii{cur};

    foreach_partly_selected_block(cur, [](QTextCursor line) -> void {
        static QRegularExpression spaces(R"((\t|[[:space:]]{2,}))");
        assert(spaces.isValid());
        QString text = line.block().text();
        if (!text.contains(spaces))
            return;

        text.replace(spaces, S_SPACE);
        line.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        line.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        line.insertText(text);
    });
}

void RemoteEditWidget::normalizeAnsi()
{
    const QString &old = m_textEdit->toPlainText();
    if (!containsAnsi(old))
        return;

    TextBuffer output = ::normalizeAnsi(old);
    if (!output.hasTrailingNewline())
        output.append('\n');

    m_textEdit->replaceAll(output.getQString());
}

void RemoteEditWidget::insertAnsiReset()
{
    m_textEdit->insertPlainText(AnsiString::get_reset_string().c_str());
}

void RemoteEditWidget::joinLines()
{
    m_textEdit->joinLines();
}

void RemoteEditWidget::quoteLines()
{
    m_textEdit->quoteLines();
}

void RemoteEditWidget::toggleWhitespace()
{
    m_textEdit->toggleWhitespace();
}

void RemoteEditWidget::justifyLines()
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
        if (m_submitted || maybeCancel()) {
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        cancelEdit();
        event->accept();
    }
}

bool RemoteEditWidget::maybeCancel()
{
    if (contentsChanged()) {
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

    cancelEdit();
    return true;
}

bool RemoteEditWidget::contentsChanged() const
{
    const QString text = m_textEdit->toPlainText();
    return QString::compare(text, m_body, Qt::CaseSensitive) != 0;
}

void RemoteEditWidget::cancelEdit()
{
    m_submitted = true;
    emit cancel();
    close();
}

void RemoteEditWidget::finishEdit()
{
    m_submitted = true;
    emit save(m_textEdit->toPlainText());
    close();
}

void RemoteEditWidget::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if (visible)
        updateStatusBar();
}
