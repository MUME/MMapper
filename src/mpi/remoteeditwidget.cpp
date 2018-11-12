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

#include "remoteeditwidget.h"

#include <utility>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMessageLogContext>
#include <QPlainTextEdit>
#include <QScopedPointer>
#include <QSize>
#include <QString>
#include <QVBoxLayout>
#include <QtGui>

#include "../configuration/configuration.h"

class QWidget;

RemoteEditWidget::RemoteEditWidget(const bool editSession,
                                   const QString &title,
                                   const QString &body,
                                   QWidget *const parent)
    : QDialog(parent)
    , m_editSession(editSession)
    , m_title(title)
    , m_body(body)
{
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(m_title + " - MMapper " + (m_editSession ? "Editor" : "Viewer"));

    // REVISIT: can this be called as an initializer?
    m_textEdit.reset(createTextEdit());

    show();
    m_textEdit->setFocus(); // REVISIT: can this be done in the creation function?
}

QPlainTextEdit *RemoteEditWidget::createTextEdit()
{
    QVBoxLayout *const mainLayout = createLayout();
    QPlainTextEdit *pTextEdit = createTextEdit(mainLayout);
    addMenuBar(mainLayout, pTextEdit);
    return pTextEdit;
}

QVBoxLayout *RemoteEditWidget::createLayout()
{
    auto *const mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    return mainLayout;
}

QPlainTextEdit *RemoteEditWidget::createTextEdit(QVBoxLayout *const mainLayout)
{
    const auto &font = QFont(getConfig().integratedClient.font);
    const QFontMetrics fm(font);
    const int x = fm.averageCharWidth() * 80;
    const int y = fm.lineSpacing() * 24;

    const auto pTextEdit = new QPlainTextEdit(this);
    pTextEdit->setFont(font);
    pTextEdit->setPlainText(m_body);
    pTextEdit->setReadOnly(!m_editSession);
    pTextEdit->setMinimumSize(QSize(x, y));
    mainLayout->addWidget(pTextEdit);
    return pTextEdit;
}

void RemoteEditWidget::addMenuBar(QVBoxLayout *const mainLayout, const QPlainTextEdit *pTextEdit)
{
    auto *const menuBar = new QMenuBar(this);
    mainLayout->setMenuBar(menuBar);
    addFileMenu(menuBar, pTextEdit);
}

void RemoteEditWidget::addFileMenu(QMenuBar *const menuBar, const QPlainTextEdit *pTextEdit)
{
    QMenu *const fileMenu = menuBar->addMenu(tr("&File"));
    if (m_editSession) {
        addSave(fileMenu);
    }
    addExit(fileMenu);
    addEditMenu(menuBar, pTextEdit);
}

void RemoteEditWidget::addEditMenu(QMenuBar *const menuBar, const QPlainTextEdit *pTextEdit)
{
    QMenu *const editMenu = menuBar->addMenu("&Edit");
    addCut(editMenu, pTextEdit);
    addCopy(editMenu, pTextEdit);
    addPaste(editMenu, pTextEdit);
}

void RemoteEditWidget::addSave(QMenu *const fileMenu)
{
    QAction *const saveAction = new QAction(QIcon::fromTheme("document-save",
                                                             QIcon(":/icons/save.png")),
                                            tr("&Submit"),
                                            this);
    saveAction->setShortcut(tr("Ctrl+S"));
    fileMenu->addAction(saveAction);
    connect(saveAction, &QAction::triggered, this, &RemoteEditWidget::finishEdit);
}

void RemoteEditWidget::addExit(QMenu *const fileMenu)
{
    QAction *const quitAction = new QAction(QIcon::fromTheme("window-close",
                                                             QIcon(":/icons/exit.png")),
                                            tr("E&xit"),
                                            this);
    quitAction->setShortcut(tr("Ctrl+Q"));
    fileMenu->addAction(quitAction);
    connect(quitAction, &QAction::triggered, this, &RemoteEditWidget::cancelEdit);
}

void RemoteEditWidget::addCut(QMenu *const editMenu, const QPlainTextEdit *pTextEdit)
{
    QAction *const cutAct = new QAction(QIcon::fromTheme("edit-cut", QIcon(":/icons/cut.png")),
                                        tr("Cu&t"),
                                        this);
    cutAct->setShortcut(tr("Ctrl+X"));
    cutAct->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
    editMenu->addAction(cutAct);
    connect(cutAct, &QAction::triggered, pTextEdit, &QPlainTextEdit::cut);
    cutAct->setDisabled(!m_editSession);
}

void RemoteEditWidget::addCopy(QMenu *const editMenu, const QPlainTextEdit *pTextEdit)
{
    QAction *const copyAct = new QAction(QIcon::fromTheme("edit-copy", QIcon(":/icons/copy.png")),
                                         tr("&Copy"),
                                         this);
    copyAct->setShortcut(tr("Ctrl+C"));
    copyAct->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
    editMenu->addAction(copyAct);
    connect(copyAct, &QAction::triggered, pTextEdit, &QPlainTextEdit::copy);
}

void RemoteEditWidget::addPaste(QMenu *const editMenu, const QPlainTextEdit *pTextEdit)
{
    QAction *const pasteAct = new QAction(QIcon::fromTheme("edit-paste", QIcon(":/icons/paste.png")),
                                          tr("&Paste"),
                                          this);
    pasteAct->setShortcut(tr("Ctrl+V"));
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
    editMenu->addAction(pasteAct);
    connect(pasteAct, &QAction::triggered, pTextEdit, &QPlainTextEdit::paste);
    pasteAct->setDisabled(!m_editSession);
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
