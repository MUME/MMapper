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

#include <QDialog>
#include <QAction>

class QTextEdit;

class RemoteEditWidget : public QDialog
{
    Q_OBJECT

public:
    RemoteEditWidget(int key, QString title, QString body, QWidget *parent = 0);
    ~RemoteEditWidget();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    void closeEvent(QCloseEvent *event);
    bool isEditSession();

protected slots:
    void cancelEdit();
    void finishEdit();
    bool maybeCancel();
    bool contentsChanged();

signals:
    void cancel(const int);
    void save(const QString &, const int);

private:
    const int m_key;
    const QString m_title;
    const QString m_body;
    bool m_submitted;

    QTextEdit *m_textEdit;
};

#endif // REMOTEEDITWIDGET_H
