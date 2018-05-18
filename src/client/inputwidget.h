/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
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

#ifndef INPUTWIDGET_H
#define INPUTWIDGET_H

#include <QPlainTextEdit>
#include <QRegExp>
#include <QLinkedList>

typedef QString InputHistoryEntry;
typedef QMutableLinkedListIterator<InputHistoryEntry> InputHistoryIterator;
typedef QString WordHistoryEntry;
typedef QMutableLinkedListIterator<WordHistoryEntry> TabCompletionIterator;

class InputWidget : public QPlainTextEdit
{
    Q_OBJECT

public:
    InputWidget(QWidget *parent = 0);
    ~InputWidget();

    QSize sizeHint() const;

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    void gotInput();
    void wordHistory(int);
    void keypadMovement(int);

    InputHistoryIterator *m_lineIterator;
    bool m_newInput;
    QLinkedList<QString> m_lineHistory;
    QLinkedList<QString> m_tabCompletionDictionary;

    void addLineHistory(const InputHistoryEntry &);
    void forwardHistory();
    void backwardHistory();

    void tabComplete();

    static const QRegExp s_whitespaceRx;
    bool m_tabbing{};
    QString m_tabFragment;
    TabCompletionIterator *m_tabIterator;
    void addTabHistory(const WordHistoryEntry &);

signals:
    void sendUserInput(const QString &);
    void displayMessage(const QString &);
    void showMessage(const QString &, int);
};


#endif /* INPUTWIDGET_H */
