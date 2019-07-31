#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QEvent>
#include <QLinkedList>
#include <QObject>
#include <QPlainTextEdit>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QtCore>

class QKeyEvent;
class QObject;
class QWidget;

using InputHistoryEntry = QString;
using InputHistoryIterator = QMutableLinkedListIterator<InputHistoryEntry>;
using WordHistoryEntry = QString;
using TabCompletionIterator = QMutableLinkedListIterator<WordHistoryEntry>;

class InputWidget final : public QPlainTextEdit
{
private:
    using base = QPlainTextEdit;

private:
    Q_OBJECT

public:
    explicit InputWidget(QWidget *parent = nullptr);
    ~InputWidget() override;

    QSize sizeHint() const override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void gotInput();
    void wordHistory(int);
    void keypadMovement(int);

    InputHistoryIterator *m_lineIterator = nullptr;
    bool m_newInput = false;
    QLinkedList<QString> m_lineHistory;
    QLinkedList<QString> m_tabCompletionDictionary;

    void addLineHistory(const InputHistoryEntry &);
    void forwardHistory();
    void backwardHistory();

    void tabComplete();

    bool m_tabbing = false;
    QString m_tabFragment;
    TabCompletionIterator *m_tabIterator = nullptr;
    void addTabHistory(const WordHistoryEntry &);

signals:
    void sendUserInput(const QString &);
    void displayMessage(const QString &);
    void showMessage(const QString &, int);
};
