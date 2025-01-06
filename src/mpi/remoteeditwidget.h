#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "remoteeditsession.h"

#include <QAction>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QScopedPointer>
#include <QSize>
#include <QString>
#include <QtCore>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QVBoxLayout>

struct EditViewCommand;
struct EditCommand2;

class QCloseEvent;
class QMenu;
class QMenuBar;
class QObject;
class QPlainTextEdit;
class QWidget;
class QStatusBar;

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

class NODISCARD_QOBJECT RemoteEditWidget : public QMainWindow
{
    Q_OBJECT

public:
    using Editor = RemoteTextEdit;

private:
    const bool m_editSession;
    const QString m_title;
    const QString m_body;

    bool m_submitted = false;
    QScopedPointer<Editor> m_textEdit;

public:
    explicit RemoteEditWidget(bool editSession,
                              const QString &title,
                              const QString &body,
                              QWidget *parent);
    ~RemoteEditWidget() override;

public:
    NODISCARD QSize minimumSizeHint() const override;
    NODISCARD QSize sizeHint() const override;
    void closeEvent(QCloseEvent *event) override;

public:
    void setVisible(bool visible) override;

protected slots:
    void slot_cancelEdit();
    void slot_finishEdit();
    NODISCARD bool slot_maybeCancel();
    NODISCARD bool slot_contentsChanged() const;
    void slot_updateStatusBar();

#define X_DECLARE_SLOT(a, b, c, d, e) void slot_##a();
    XFOREACH_REMOTE_EDIT_MENU_ITEM(X_DECLARE_SLOT)
#undef X_DECLARE_SLOT

signals:
    void sig_cancel();
    void sig_save(const QString &);

private:
    NODISCARD Editor *createTextEdit();

    void addToMenu(QMenu *menu, const EditViewCommand &cmd);
    void addToMenu(QMenu *menu, const EditCommand2 &cmd, const Editor *pTextEdit);

    void addFileMenu(QMenuBar *menuBar, const Editor *pTextEdit);
    void addEditAndViewMenus(QMenuBar *menuBar, const Editor *pTextEdit);
    void addSave(QMenu *fileMenu);
    void addExit(QMenu *fileMenu);
    void addStatusBar(const Editor *pTextEdit);
};
