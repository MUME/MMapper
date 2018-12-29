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

#ifndef REMOTEEDITWIDGET_H
#define REMOTEEDITWIDGET_H

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

class QCloseEvent;
class QMenu;
class QMenuBar;
class QObject;
class QPlainTextEdit;
class QWidget;
class QStatusBar;

class RemoteEditWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit RemoteEditWidget(bool editSession,
                              const QString &title,
                              const QString &body,
                              QWidget *parent = nullptr);
    ~RemoteEditWidget() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;
    void closeEvent(QCloseEvent *event) override;

protected slots:
    void cancelEdit();
    void finishEdit();
    bool maybeCancel();
    bool contentsChanged() const;
    void updateStatusBar();
    void justifyText();

signals:
    void cancel();
    void save(const QString &);

private:
    QPlainTextEdit *createTextEdit();

    void addFileMenu(QMenuBar *menuBar, const QPlainTextEdit *pTextEdit);
    void addEditMenu(QMenuBar *menuBar, const QPlainTextEdit *pTextEdit);
    void addSave(QMenu *fileMenu);
    void addExit(QMenu *fileMenu);
    void addCut(QMenu *editMenu, const QPlainTextEdit *pTextEdit);
    void addCopy(QMenu *editMenu, const QPlainTextEdit *pTextEdit);
    void addPaste(QMenu *editMenu, const QPlainTextEdit *pTextEdit);
    void addJustify(QMenu *editMenu);
    void addStatusBar(const QPlainTextEdit *pTextEdit);

private:
    const bool m_editSession;
    const QString m_title;
    const QString m_body;

    bool m_submitted = false;
    QScopedPointer<QPlainTextEdit> m_textEdit{};
};

#endif // REMOTEEDITWIDGET_H
