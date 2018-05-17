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
#include "configuration/configuration.h"

#include <QCloseEvent>
#include <QDebug>
#include <QMenuBar>
#include <QMessageBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <cassert>
#include <utility>

RemoteEditWidget::RemoteEditWidget(int key, QString title, QString body,
                                   QWidget *parent)
    : QDialog(parent), m_key(key), m_title(std::move(title)), m_body(std::move(body)),
      m_submitted(false)
{
    setAttribute(Qt::WA_DeleteOnClose);
    assert(testAttribute(Qt::WA_DeleteOnClose));

    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(m_title + " - MMapper " + (isEditSession() ? "Editor" : "Viewer"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QFontMetrics fm(Config().m_clientFont);
    int x = fm.averageCharWidth() * 80;
    int y = fm.lineSpacing() * 24;

    m_textEdit = new QTextEdit(this);
    m_textEdit->setFont(Config().m_clientFont);
    m_textEdit->setText(m_body);
    m_textEdit->setReadOnly(!isEditSession());
    m_textEdit->setMinimumSize(QSize(x, y));
    mainLayout->addWidget(m_textEdit);

    auto *menuBar = new QMenuBar(this);
    mainLayout->setMenuBar(menuBar);

    QMenu *fileMenu = menuBar->addMenu(tr("&File"));
    if (isEditSession()) {
        QAction *saveAction = new QAction(QIcon::fromTheme("document-save", QIcon(":/icons/save.png")),
                                          tr("&Submit"), this);
        saveAction->setShortcut(tr("Ctrl+S"));
        fileMenu->addAction(saveAction);
        connect(saveAction, SIGNAL(triggered()), SLOT(finishEdit()));
    }

    QAction *quitAction = new QAction(QIcon::fromTheme("window-close", QIcon(":/icons/exit.png")),
                                      tr("E&xit"), this);
    quitAction->setShortcut(tr("Ctrl+Q"));
    fileMenu->addAction(quitAction);
    connect(quitAction, SIGNAL(triggered()), SLOT(cancelEdit()));

    QMenu *editMenu = menuBar->addMenu("&Edit");
    QAction *cutAct = new QAction(QIcon::fromTheme("edit-cut", QIcon(":/icons/cut.png")), tr("Cu&t"),
                                  this);
    cutAct->setShortcut(tr("Ctrl+X"));
    cutAct->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
    editMenu->addAction(cutAct);
    connect(cutAct, SIGNAL(triggered()), m_textEdit, SLOT(cut()));

    QAction *copyAct = new QAction(QIcon::fromTheme("edit-copy", QIcon(":/icons/copy.png")),
                                   tr("&Copy"), this);
    copyAct->setShortcut(tr("Ctrl+C"));
    copyAct->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
    editMenu->addAction(copyAct);
    connect(copyAct, SIGNAL(triggered()), m_textEdit, SLOT(copy()));

    QAction *pasteAct = new QAction(QIcon::fromTheme("edit-paste", QIcon(":/icons/paste.png")),
                                    tr("&Paste"), this);
    pasteAct->setShortcut(tr("Ctrl+V"));
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
    editMenu->addAction(pasteAct);
    connect(pasteAct, SIGNAL(triggered()), m_textEdit, SLOT(paste()));

    if (!isEditSession()) {
        cutAct->setDisabled(true);
        pasteAct->setDisabled(true);
    }

    show();
    m_textEdit->setFocus();
}

RemoteEditWidget::~RemoteEditWidget()
    = default;

QSize RemoteEditWidget::minimumSizeHint() const
{
    return {100, 100};
}

QSize RemoteEditWidget::sizeHint() const
{
    return {640, 480};
}

void RemoteEditWidget::closeEvent(QCloseEvent *event)
{
    if (m_submitted || !isEditSession() || maybeCancel()) {
        event->accept();
    } else {
        event->ignore();
    }
}

bool RemoteEditWidget::isEditSession()
{
    return m_key != -1;
}

bool RemoteEditWidget::maybeCancel()
{
    if (contentsChanged()) {
        int ret = QMessageBox::warning(this, m_title,
                                       tr("You have edited the document.\n"
                                          "Do you want to cancel your changes?"),
                                       QMessageBox::Yes,
                                       QMessageBox::No | QMessageBox::Escape | QMessageBox::Default);
        if (ret == QMessageBox::No) {
            return false;
        }
    }

    cancelEdit();
    return true;
}

bool RemoteEditWidget::contentsChanged()
{
    QString text = m_textEdit->toPlainText();
    return QString::compare(text, m_body, Qt::CaseSensitive) != 0;
}

void RemoteEditWidget::cancelEdit()
{
    m_submitted = true;
    emit cancel(m_key);
    close();
}


void RemoteEditWidget::finishEdit()
{
    m_submitted = true;
    emit save(m_textEdit->toPlainText().toLatin1(), m_key);
    close();
}

