#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "remoteeditsession.h"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <QAction>
#include <QDateTime>
#include <QDialog>
#include <QPlainTextEdit>
#include <QScopedPointer>
#include <QSize>
#include <QString>
#include <QtCore>

struct EditViewCommand;
struct EditCommand2;

class QFrame;
class QKeyEvent;
class QLabel;
class QMenu;
class QMenuBar;
class QObject;
class QPlainTextEdit;
class QStatusBar;
class QVBoxLayout;
class QWidget;
class GotoWidget;
class FindReplaceWidget;

enum class NODISCARD EditViewCmdEnum { VIEW_OPTION, EDIT_ALIGNMENT, EDIT_COLORS, EDIT_WHITESPACE };
enum class NODISCARD EditCmd2Enum { EDIT_ONLY, EDIT_OR_VIEW, SPACER };

// NOTE: Ctrl+A is "Select All" by default.
#define XFOREACH_REMOTE_EDIT_MENU_ITEM(X) \
    X(justifyText, \
      EditViewCmdEnum::EDIT_ALIGNMENT, \
      "&Justify Entire Message", \
      "Justify text to 80 characters", \
      nullptr) \
    X(justifyLines, \
      EditViewCmdEnum::EDIT_ALIGNMENT, \
      "Justify &Selection", \
      "Justify selection to 80 characters", \
      "Ctrl+J") \
    X(expandTabs, \
      EditViewCmdEnum::EDIT_WHITESPACE, \
      "&Expand Tabs", \
      "Expand tabs to 8-character tabstops", \
      "Ctrl+E") \
    X(removeTrailingWhitespace, \
      EditViewCmdEnum::EDIT_WHITESPACE, \
      "Remove Trailing &Whitespace", \
      "Remove trailing whitespace", \
      "Ctrl+W") \
    X(removeDuplicateSpaces, \
      EditViewCmdEnum::EDIT_WHITESPACE, \
      "Remove &Duplicate Spaces", \
      "Remove duplicate spaces in any partly-selected lines", \
      "Ctrl+D") \
    X(normalizeAnsi, \
      EditViewCmdEnum::EDIT_COLORS, \
      "&Normalize Ansi Codes", \
      "Normalize ansi codes.", \
      "Ctrl+N") \
    X(insertAnsiReset, \
      EditViewCmdEnum::EDIT_COLORS, \
      "&Insert Ansi Reset Code", \
      "Insert an ansi reset code (ESC[0m).", \
      "Ctrl+I") \
    X(joinLines, \
      EditViewCmdEnum::EDIT_ALIGNMENT, \
      "Joi&n Lines", \
      "Join all partly-selected lines.", \
      "Ctrl+Shift+J") \
    X(quoteLines, \
      EditViewCmdEnum::EDIT_ALIGNMENT, \
      "&Quote Lines", \
      "Add a quote prefix to all partly-selected lines.", \
      "Ctrl+>" /* aka "Ctrl+Shift+." */) \
    X(previewAnsi, \
      EditViewCmdEnum::VIEW_OPTION, \
      "&Preview Ansi Codes", \
      "Preview message with ansi coloring.", \
      "Ctrl+P") \
    X(toggleWhitespace, \
      EditViewCmdEnum::VIEW_OPTION, \
      "Toggle &Whitespace", \
      "Toggle the display of whitespace.", \
      "Ctrl+Shift+W")

class NODISCARD_QOBJECT RemoteTextEdit final : public QPlainTextEdit
{
    Q_OBJECT

private:
    using base = QPlainTextEdit;

public:
    explicit RemoteTextEdit(const QString &initialText, QWidget *parent);
    ~RemoteTextEdit() final;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;

private:
    /// Purposely hides base::setPlainText(), because it clears the UNDO history.
    /// You should call replaceAll() instead.
    void setPlainText(const QString &str) = delete;

public:
    void replaceAll(const QString &str);
    void showWhitespace(bool enabled);
    NODISCARD bool isShowingWhitespace() const;
    void toggleWhitespace();

    void joinLines();
    void quoteLines();
    void justifyLines(int maxLength);

public:
    void prefixPartialSelection(const QString &prefix);

private:
    void handleEventTab(QKeyEvent *event);
    void handleEventBacktab(QKeyEvent *event);
    void handle_toolTip(QEvent *event) const;
};

/// One editor/viewer page; hosted as a tab by RemoteEditPanel. Its lifetime
/// belongs to the owning RemoteEditInternalSession, which deletes it (via
/// closeSilently()) when the session ends, and the hosting tab disappears with it.
class NODISCARD_QOBJECT RemoteEditWidget : public QWidget
{
    Q_OBJECT

private:
    QMenuBar *m_menuBar = nullptr;
    QStatusBar *m_statusBar = nullptr;
    QFrame *m_banner = nullptr;
    QLabel *m_bannerLabel = nullptr;
    QAction *m_saveAction = nullptr;

public:
    using Editor = RemoteTextEdit;

private:
    const bool m_editSession;
    const bool m_draftRecovery;
    const QString m_title;
    const QString m_body;
    QString m_lastNotifiedText;

    bool m_submitted = false;
    bool m_connected = true;
    QScopedPointer<Editor> m_textEdit;
    QScopedPointer<GotoWidget> m_gotoWidget;
    QScopedPointer<FindReplaceWidget> m_findReplaceWidget;
    // See AnsiViewWindow.h's makeAnsiViewWindow() doc comment for why this
    // holds a plain QDialog rather than AnsiViewWindow.
    std::unique_ptr<QDialog> m_preview;

public:
    explicit RemoteEditWidget(
        bool editSession, bool draftRecovery, QString title, QString body, QWidget *parent);
    ~RemoteEditWidget() override;

public:
    NODISCARD QSize minimumSizeHint() const override;
    NODISCARD QSize sizeHint() const override;
    NODISCARD const QString &getTitle() const { return m_title; }
    NODISCARD bool isEditSession() const { return m_editSession; }
    NODISCARD bool isDraftRecovery() const { return m_draftRecovery; }
    NODISCARD bool isModified() const;
    /// What the tab's close button does: prompts if there are unsaved edits,
    /// otherwise cancels the session.
    void requestClose();
    /// Asks the hosting panel to bring this page to the front.
    void focus() { emit sig_focusRequested(); }
    /// Offers to replace the text with an unsent draft of the same title.
    void offerRecoveredDraft(const QDateTime &lastModified,
                             std::function<void()> restore,
                             std::function<void()> discard);
    /// MUME went away: Submit is disabled and the page explains where the text goes.
    void showDisconnected();
    void replaceText(const QString &text);
    /// Schedules deletion without the "discard changes?" prompt; used when the
    /// manager is tearing down the session for reasons other than the user
    /// choosing Submit/Exit/Discard in this widget.
    void closeSilently();

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    NODISCARD QAction *findActionForKey(const QKeyEvent &key) const;

private:
    NODISCARD Editor *createTextEdit();
    NODISCARD GotoWidget *createGotoWidget();
    NODISCARD FindReplaceWidget *createFindReplaceWidget();

    void addToMenu(QMenu *menu, const EditViewCommand &cmd);
    void addToMenu(QMenu *menu, const EditCommand2 &cmd, const Editor *pTextEdit);

    void addFileMenu(const Editor *pTextEdit);
    void addEditAndViewMenus(const Editor *pTextEdit);
    void addSave(QMenu *fileMenu);
    void addExit(QMenu *fileMenu);
    void addStatusBar(const Editor *pTextEdit);
    void promptDiscardChanges();
    void showBanner(const QString &text,
                    const std::vector<std::pair<QString, std::function<void()>>> &buttons);
    void hideBanner();

signals:
    void sig_cancel();
    void sig_save(const QString &);
    void sig_textModified(const QString &);
    void sig_discard();
    void sig_focusRequested();

protected slots:
    void slot_cancelEdit();
    void slot_finishEdit();
    void slot_discardDraft();
    void slot_updateStatusBar();
    void slot_updateStatus(const QString &message);
    void slot_handleFindRequested(const QString &term, QTextDocument::FindFlags flags);
    void slot_handleReplaceCurrentRequested(const QString &findTerm,
                                            const QString &replaceTerm,
                                            QTextDocument::FindFlags flags);
    void slot_handleReplaceAllRequested(const QString &findTerm,
                                        const QString &replaceTerm,
                                        QTextDocument::FindFlags flags);

#define X_DECLARE_SLOT(a, b, c, d, e) void slot_##a();
    XFOREACH_REMOTE_EDIT_MENU_ITEM(X_DECLARE_SLOT)
#undef X_DECLARE_SLOT
};
