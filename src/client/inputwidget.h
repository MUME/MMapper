#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <iterator>
#include <list>
#include <QEvent>
#include <QObject>
#include <QPlainTextEdit>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QtCore>

class QKeyEvent;
class QObject;
class QWidget;

class InputHistory final : private std::list<QString>
{
public:
    InputHistory() { m_iterator = begin(); }

public:
    void addInputLine(const QString &);

public:
    void forward() { std::advance(m_iterator, 1); }
    void backward() { std::advance(m_iterator, -1); }

public:
    const QString &value() const { return *m_iterator; }

public:
    bool atFront() const { return m_iterator == begin(); }
    bool atEnd() const { return m_iterator == end(); }

private:
    std::list<QString>::iterator m_iterator;
};

class TabHistory final : private std::list<QString>
{
public:
    TabHistory() { m_iterator = begin(); }

public:
    void addInputLine(const QString &);

public:
    void forward() { std::advance(m_iterator, 1); }
    void reset() { m_iterator = begin(); }

public:
    const QString &value() const { return *m_iterator; }

public:
    using std::list<QString>::empty;
    bool atEnd() const { return m_iterator == end(); }

private:
    std::list<QString>::iterator m_iterator;
};

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
    bool tryHistory(int);
    void keypadMovement(int);

private:
    void tabComplete();
    bool m_tabbing = false;
    QString m_tabFragment;
    TabHistory m_tabHistory;

private:
    void forwardHistory();
    void backwardHistory();
    InputHistory m_inputHistory;

signals:
    void sendUserInput(const QString &);
    void displayMessage(const QString &);
    void showMessage(const QString &, int);
};
