#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include <QAction>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QScopedPointer>
#include <QSize>
#include <QString>
#include <QtCore>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QVBoxLayout>

#include "remoteeditsession.h"

struct EditViewCommand;
struct EditCommand2;

class QCloseEvent;
class QMenu;
class QMenuBar;
class QObject;
class QPlainTextEdit;
class QWidget;
class QStatusBar;

enum class EditViewCmdType { VIEW_OPTION, EDIT_ALIGNMENT, EDIT_COLORS, EDIT_WHITESPACE };
enum class EditCmdType2 { EDIT_ONLY, EDIT_OR_VIEW, SPACER };

// NOTE: Ctrl+A is "Select All" by default.
#define XFOREACH_REMOTE_EDIT_MENU_ITEM(X) \
    X(justifyText, \
      EditViewCmdType::EDIT_ALIGNMENT, \
      "&Justify Entire Message", \
      "Justify text to 80 characters", \
      nullptr) \
    X(justifyLines, \
      EditViewCmdType::EDIT_ALIGNMENT, \
      "Justify &Selection", \
      "Justify selection to 80 characters", \
      "Ctrl+J") \
    X(expandTabs, \
      EditViewCmdType::EDIT_WHITESPACE, \
      "&Expand Tabs", \
      "Expand tabs to 8-character tabstops", \
      "Ctrl+E") \
    X(removeTrailingWhitespace, \
      EditViewCmdType::EDIT_WHITESPACE, \
      "Remove Trailing &Whitespace", \
      "Remove trailing whitespace", \
      "Ctrl+W") \
    X(removeDuplicateSpaces, \
      EditViewCmdType::EDIT_WHITESPACE, \
      "Remove &Duplicate Spaces", \
      "Remove duplicate spaces in any partly-selected lines", \
      "Ctrl+D") \
    X(normalizeAnsi, \
      EditViewCmdType::EDIT_COLORS, \
      "&Normalize Ansi Codes", \
      "Normalize ansi codes.", \
      "Ctrl+N") \
    X(insertAnsiReset, \
      EditViewCmdType::EDIT_COLORS, \
      "&Insert Ansi Reset Code", \
      "Insert an ansi reset code (ESC[0m).", \
      "Ctrl+I") \
    X(joinLines, \
      EditViewCmdType::EDIT_ALIGNMENT, \
      "Joi&n Lines", \
      "Join all partly-selected lines.", \
      "Ctrl+Shift+J") \
    X(quoteLines, \
      EditViewCmdType::EDIT_ALIGNMENT, \
      "&Quote Lines", \
      "Add a quote prefix to all partly-selected lines.", \
      "Ctrl+>" /* aka "Ctrl+Shift+." */) \
    X(toggleWhitespace, \
      EditViewCmdType::VIEW_OPTION, \
      "Toggle &Whitespace", \
      "Toggle the display of whitespace.", \
      "Ctrl+Shift+W")

class RemoteTextEdit final : public QPlainTextEdit
{
private:
    using base = QPlainTextEdit;

private:
    Q_OBJECT

public:
    explicit RemoteTextEdit(const QString &initialText, QWidget *parent = nullptr);
    ~RemoteTextEdit() override;

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
    bool isShowingWhitespace() const;
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

class RemoteEditWidget : public QMainWindow
{
    Q_OBJECT

public:
    using Editor = RemoteTextEdit;

public:
    explicit RemoteEditWidget(bool editSession,
                              const QString &title,
                              const QString &body,
                              QWidget *parent = nullptr);
    ~RemoteEditWidget() override;

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;
    void closeEvent(QCloseEvent *event) override;

public:
    void setVisible(bool visible) override;

protected slots:
    void cancelEdit();
    void finishEdit();
    bool maybeCancel();
    bool contentsChanged() const;
    void updateStatusBar();

#define X(a, b, c, d, e) void a();
    XFOREACH_REMOTE_EDIT_MENU_ITEM(X)
#undef X

signals:
    void cancel();
    void save(const QString &);

private:
    Editor *createTextEdit();

    void addToMenu(QMenu *menu, const EditViewCommand &cmd);
    void addToMenu(QMenu *menu, const EditCommand2 &cmd, const Editor *pTextEdit);

    void addFileMenu(QMenuBar *menuBar, const Editor *pTextEdit);
    void addEditAndViewMenus(QMenuBar *menuBar, const Editor *pTextEdit);
    void addSave(QMenu *fileMenu);
    void addExit(QMenu *fileMenu);
    void addStatusBar(const Editor *pTextEdit);

private:
    const bool m_editSession;
    const QString m_title;
    const QString m_body;
    int m_maxLength = 80;

    bool m_submitted = false;
    QScopedPointer<Editor> m_textEdit{};
};
