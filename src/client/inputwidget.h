#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"

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

class NODISCARD InputHistory final : private std::list<QString>
{
public:
    InputHistory() { m_iterator = begin(); }

public:
    void addInputLine(const QString &);

public:
    void forward() { std::advance(m_iterator, 1); }
    void backward() { std::advance(m_iterator, -1); }

public:
    NODISCARD const QString &value() const { return *m_iterator; }

public:
    NODISCARD bool atFront() const { return m_iterator == begin(); }
    NODISCARD bool atEnd() const { return m_iterator == end(); }

private:
    std::list<QString>::iterator m_iterator;
};

class NODISCARD TabHistory final : private std::list<QString>
{
    using base = std::list<QString>;

public:
    TabHistory() { m_iterator = begin(); }

public:
    void addInputLine(const QString &);

public:
    void forward() { std::advance(m_iterator, 1); }
    void reset() { m_iterator = begin(); }

public:
    NODISCARD const QString &value() const { return *m_iterator; }

public:
    NODISCARD bool empty() { return base::empty(); }
    NODISCARD bool atEnd() const { return m_iterator == end(); }

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
    explicit InputWidget(QWidget *parent);
    ~InputWidget() final;

    NODISCARD QSize sizeHint() const override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void gotInput();
    NODISCARD bool tryHistory(int);
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

private:
    void sendUserInput(const QString &msg) { emit sig_sendUserInput(msg); }

signals:
    void sig_sendUserInput(const QString &);
    void sig_displayMessage(const QString &);
    void sig_showMessage(const QString &, int);
};
