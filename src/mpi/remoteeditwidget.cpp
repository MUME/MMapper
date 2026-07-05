// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "remoteeditwidget.h"

#include "../client/displaywidget.h"
#include "../configuration/configuration.h"
#include "../global/AnsiTextUtils.h"
#include "../global/Consts.h"
#include "../global/TabUtils.h"
#include "../global/TextUtils.h"
#include "../global/window_utils.h"
#include "../viewers/AnsiViewWindow.h"
#include "RemoteEditDocumentOps.h"
#include "RemoteEditHighlighter.h"
#include "findreplacewidget.h"
#include "gotowidget.h"

#include <cassert>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace char_consts;

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QMessageLogContext>
#include <QPlainTextEdit>
#include <QScopedPointer>
#include <QSize>
#include <QString>
#include <QTextDocument>
#include <QtGui>
#include <QtWidgets>

static constexpr const bool USE_TOOLTIPS = false;

class QWidget;

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

/* perform a 2-space block indentation of all partly-selected lines */
void RemoteTextEdit::prefixPartialSelection(const QString &prefix)
{
    mmqt::remoteEditPrefixPartialSelection(textCursor(), prefix);
}

void RemoteTextEdit::handleEventTab(QKeyEvent *const event)
{
    event->accept();
    auto cur = textCursor();
    if (cur.hasSelection()) {
        return prefixPartialSelection(string_consts::S_TWO_SPACES);
    }

    /* otherwise, insert a real tab, expand it, and restore the cursor */
    mmqt::RemoteEditUndoGroup raii(cur);

    cur.insertText(string_consts::S_TAB);

    const int col_before = cur.positionInBlock();
    const auto &block = cur.block();
    const QString &s = block.text().left(col_before);
    const int col_after = mmqt::measureExpandedTabsOneLine(s, 0);
    mmqt::remoteEditExpandTabsOnLine(cur);

    cur.setPosition(block.position() + col_after);
    setTextCursor(cur);
}

/* 2-space block de-indentation of all partly-selected lines */
void RemoteTextEdit::handleEventBacktab(QKeyEvent *const event)
{
    event->accept();
    mmqt::remoteEditUnindentPartialSelection(textCursor());
}

void RemoteTextEdit::joinLines()
{
    mmqt::remoteEditJoinLines(textCursor());
}

void RemoteTextEdit::quoteLines()
{
    prefixPartialSelection("> ");
}

void RemoteTextEdit::justifyLines(const int maxLen)
{
    mmqt::remoteEditJustifyLines(textCursor(), maxLen);
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
    : QDialog(parent)
    , m_editSession(editSession)
    , m_title(std::move(title))
    , m_body(std::move(body))
{
    setWindowFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint
                   | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
    mmqt::setWindowTitle2(*this,
                          QString("MMapper %1").arg(m_editSession ? "Editor" : "Viewer"),
                          m_title);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    m_menuBar = new QMenuBar(this);
    mainLayout->setMenuBar(m_menuBar);

    m_gotoWidget.reset(createGotoWidget());
    mainLayout->addWidget(m_gotoWidget.get(), 0);

    m_findReplaceWidget.reset(createFindReplaceWidget());
    mainLayout->addWidget(m_findReplaceWidget.get(), 0);

    m_textEdit.reset(createTextEdit());
    mainLayout->addWidget(m_textEdit.get(), 1);

    m_statusBar = new QStatusBar(this);
    setAttribute(Qt::WA_DeleteOnClose);
    mainLayout->addWidget(m_statusBar);

    addStatusBar(m_textEdit.get());
    addFileMenu(m_textEdit.get());

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
    new RemoteEditHighlighter(doc, mmqt::REMOTE_EDIT_MAX_LENGTH);
    return pTextEdit;
}

auto RemoteEditWidget::createGotoWidget() -> GotoWidget *
{
    const auto pGotoWidget = new GotoWidget(this);
    pGotoWidget->hide();

    connect(pGotoWidget, &GotoWidget::sig_gotoLineRequested, this, [this](int lineNum) {
        if (auto cursor = mmqt::remoteEditGotoLine(*m_textEdit->document(), lineNum)) {
            m_textEdit->setTextCursor(*cursor);
            m_textEdit->ensureCursorVisible();
            slot_updateStatus(QString("Moved to line %1").arg(lineNum));
            m_gotoWidget->hide();
            m_textEdit->setFocus(Qt::OtherFocusReason);
        } else {
            slot_updateStatus(QString("Error: Line %1 not found.").arg(lineNum));
            m_gotoWidget->setFocusToInput();
        }
    });
    connect(pGotoWidget, &GotoWidget::sig_closeRequested, this, [this]() {
        m_gotoWidget->hide();
        m_textEdit->setFocus(Qt::OtherFocusReason);
    });
    return pGotoWidget;
}

auto RemoteEditWidget::createFindReplaceWidget() -> FindReplaceWidget *
{
    const auto pFindReplaceWidget = new FindReplaceWidget(m_editSession, this);
    pFindReplaceWidget->hide();

    connect(pFindReplaceWidget,
            &FindReplaceWidget::sig_findRequested,
            this,
            &RemoteEditWidget::slot_handleFindRequested);
    connect(pFindReplaceWidget,
            &FindReplaceWidget::sig_replaceCurrentRequested,
            this,
            &RemoteEditWidget::slot_handleReplaceCurrentRequested);
    connect(pFindReplaceWidget,
            &FindReplaceWidget::sig_replaceAllRequested,
            this,
            &RemoteEditWidget::slot_handleReplaceAllRequested);
    connect(pFindReplaceWidget, &FindReplaceWidget::sig_closeRequested, this, [this]() {
        m_findReplaceWidget->hide();
        m_textEdit->setFocus(Qt::OtherFocusReason);
    });
    connect(pFindReplaceWidget,
            &FindReplaceWidget::sig_statusMessage,
            this,
            &RemoteEditWidget::slot_updateStatus);
    return pFindReplaceWidget;
}

void RemoteEditWidget::addFileMenu(const Editor *const pTextEdit)
{
    QMenu *const fileMenu = m_menuBar->addMenu(tr("&File"));
    if (m_editSession) {
        addSave(fileMenu);
    }
    addExit(fileMenu);
    addEditAndViewMenus(pTextEdit);
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

void RemoteEditWidget::addEditAndViewMenus(const Editor *const pTextEdit)
{
    QMenu *const editMenu = m_menuBar->addMenu("&Edit");
    QMenu *const viewMenu = m_menuBar->addMenu("&View");

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

    QAction *findAction = new QAction(QIcon::fromTheme("edit-find"), tr("&Find/Replace..."), this);
    findAction->setShortcut(QKeySequence::Find);
    findAction->setStatusTip(tr("Show Find/Replace bar"));
    connect(findAction, &QAction::triggered, this, [this]() {
        m_gotoWidget->hide();
        m_findReplaceWidget->show();
        m_findReplaceWidget->setFocusToFindInput();
    });
    editMenu->addAction(findAction);

    QAction *gotoAction = new QAction(QIcon::fromTheme("go-jump"), tr("&Go to Line..."), this);
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Mac) {
        gotoAction->setShortcut(QKeySequence("Ctrl+L"));
    } else {
        gotoAction->setShortcut(QKeySequence("Ctrl+G"));
    }
    gotoAction->setStatusTip(tr("Show Go to Line bar"));
    connect(gotoAction, &QAction::triggered, this, [this]() {
        m_findReplaceWidget->hide();
        m_gotoWidget->show();
        m_gotoWidget->setFocusToInput();
    });
    editMenu->addAction(gotoAction);
    editMenu->addSeparator();

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
    m_statusBar->showMessage(tr("Ready"));

    connect(pTextEdit,
            &QPlainTextEdit::cursorPositionChanged,
            this,
            &RemoteEditWidget::slot_updateStatusBar);
    connect(pTextEdit,
            &QPlainTextEdit::selectionChanged,
            this,
            &RemoteEditWidget::slot_updateStatusBar);
    connect(pTextEdit, &QPlainTextEdit::textChanged, this, &RemoteEditWidget::slot_updateStatusBar);
    connect(pTextEdit, &QPlainTextEdit::textChanged, this, [this, pTextEdit]() {
        if (m_editSession) {
            emit sig_textModified(pTextEdit->toPlainText());
        }
    });
}

void RemoteEditWidget::slot_updateStatus(const QString &message_param)
{
    if (message_param.isEmpty()) {
        slot_updateStatusBar();
    } else {
        m_statusBar->showMessage(message_param);
    }
}

void RemoteEditWidget::slot_handleFindRequested(const QString &term, QTextDocument::FindFlags flags)
{
    if (term.isEmpty()) {
        return;
    }

    const auto outcome = mmqt::remoteEditFind(*m_textEdit->document(),
                                              m_textEdit->textCursor(),
                                              term,
                                              flags);
    switch (outcome.result) {
    case mmqt::RemoteEditFindResultEnum::FOUND:
        m_textEdit->setTextCursor(outcome.cursor);
        slot_updateStatus(QString("Found: '%1'").arg(term));
        break;
    case mmqt::RemoteEditFindResultEnum::FOUND_AFTER_WRAP:
        m_textEdit->setTextCursor(outcome.cursor);
        slot_updateStatus(QString("Found (from %1): '%2'")
                              .arg(flags.testFlag(QTextDocument::FindBackward) ? "end" : "top")
                              .arg(term));
        break;
    case mmqt::RemoteEditFindResultEnum::NOT_FOUND:
        // Cursor was never moved, so there's nothing to restore.
        slot_updateStatus(QString("Not found: '%1'").arg(term));
        break;
    }
}

void RemoteEditWidget::slot_handleReplaceCurrentRequested(const QString &findTerm,
                                                          const QString &replaceTerm,
                                                          QTextDocument::FindFlags flags)
{
    if (findTerm.isEmpty()) {
        return;
    }

    QTextCursor cursor = m_textEdit->textCursor();
    if (mmqt::remoteEditReplaceCurrentIfMatches(cursor, findTerm, replaceTerm, flags)) {
        m_textEdit->setTextCursor(cursor);
        slot_updateStatus(QString("Replaced: '%1'").arg(findTerm));
    }

    // Always find the next occurrence after attempting replacement
    slot_handleFindRequested(findTerm, flags);
}

void RemoteEditWidget::slot_handleReplaceAllRequested(const QString &findTerm,
                                                      const QString &replaceTerm,
                                                      QTextDocument::FindFlags flags)
{
    if (findTerm.isEmpty()) {
        return;
    }

    QTextCursor originalCursor = m_textEdit->textCursor();
    const int replacements = mmqt::remoteEditReplaceAll(*m_textEdit->document(),
                                                        findTerm,
                                                        replaceTerm,
                                                        flags);
    m_textEdit->setTextCursor(originalCursor);

    if (replacements > 0) {
        slot_updateStatus(
            QString("Replaced %1 occurrence(s) of '%2'.").arg(replacements).arg(findTerm));
    } else {
        slot_updateStatus(QString("Not found for replace all: '%1'.").arg(findTerm));
    }
}

void RemoteEditWidget::slot_updateStatusBar()
{
    const auto info = mmqt::computeRemoteEditStatus(m_textEdit->textCursor());

    QString status = QString("Line %1, Column %2").arg(info.line).arg(info.col);
    if (info.nonAnsiAwareCol) {
        status.append(QString(" (non-ansi-aware: %1)").arg(*info.nonAnsiAwareCol));
    }

    if (info.selection) {
        const auto plural = [](auto n) { return (n == 1) ? "" : "s"; };

        status.append(QString(", Selection: %1 char%2 on %3 line%4")
                          .arg(info.selection->chars)
                          .arg(plural(info.selection->chars))
                          .arg(info.selection->lines)
                          .arg(plural(info.selection->lines)));
    } else if (!info.ansiCode.isEmpty()) {
        status.append(QString(", AnsiCode: {%1}").arg(info.ansiCode));
    }

    const bool hasSelection = info.selection.has_value();
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

    if (info.hasTabs) {
        err("Tabs");
    }

    if (info.hasTrailingSpace) {
        err("Trailing-Spaces");
    }

    if (info.hasLongLines) {
        err("Long-lines");
    }

    if (first) {
        status.append(hasSelection ? ", Selected-Lines: Ok" : ", Document: Ok");
    }

    m_statusBar->showMessage(status);
}

void RemoteEditWidget::slot_justifyText()
{
    const QString &old = m_textEdit->toPlainText();
    m_textEdit->replaceAll(mmqt::remoteEditJustifyText(old, mmqt::REMOTE_EDIT_MAX_LENGTH));
}

// TODO: Set the tabstops for the edit control to 8 spaces, so the text won't jump when you expand tabs.
void RemoteEditWidget::slot_expandTabs()
{
    mmqt::remoteEditExpandTabs(m_textEdit->textCursor());
}

void RemoteEditWidget::slot_removeTrailingWhitespace()
{
    mmqt::remoteEditRemoveTrailingWhitespace(m_textEdit->textCursor());
}

void RemoteEditWidget::slot_removeDuplicateSpaces()
{
    mmqt::remoteEditRemoveDuplicateSpaces(m_textEdit->textCursor());
}

void RemoteEditWidget::slot_normalizeAnsi()
{
    const QString &old = m_textEdit->toPlainText();
    if (auto output = mmqt::remoteEditNormalizeAnsi(old)) {
        m_textEdit->replaceAll(*output);
    }
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
    m_textEdit->justifyLines(mmqt::REMOTE_EDIT_MAX_LENGTH);
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
    if (m_submitted) {
        event->accept();
        return;
    }

    if (m_editSession && slot_contentsChanged()) {
        // Ask first; if the user discards, slot_cancelEdit() closes again.
        event->ignore();
        promptDiscardChanges();
        return;
    }

    slot_cancelEdit();
    event->accept();
}

void RemoteEditWidget::promptDiscardChanges()
{
    // Non-blocking: nested event loops are unavailable on wasm.
    auto *const dlg = new QMessageBox(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setIcon(QMessageBox::Warning);
    dlg->setWindowTitle(m_title);
    dlg->setText(tr("You have edited the document.\n"
                    "Are you sure you want to discard all changes?"));
    dlg->setStandardButtons(QMessageBox::Discard | QMessageBox::Cancel);
    dlg->setDefaultButton(QMessageBox::Cancel);
    dlg->setEscapeButton(QMessageBox::Cancel);
    connect(dlg, &QMessageBox::finished, this, [this](const int result) {
        if (result == QMessageBox::Discard) {
            slot_cancelEdit();
        }
    });
    dlg->open();
}

/* Qt virtual */
void RemoteEditWidget::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    slot_updateStatusBar();
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
