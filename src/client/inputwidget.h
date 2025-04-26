#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"
#include "../global/macros.h"
#include "PaletteManager.h"

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
private:
    std::list<QString>::iterator m_iterator;

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
};

class NODISCARD TabHistory final : private std::list<QString>
{
    using base = std::list<QString>;

private:
    std::list<QString>::iterator m_iterator;

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
};

struct NODISCARD InputWidgetOutputs
{
public:
    explicit InputWidgetOutputs() = default;
    virtual ~InputWidgetOutputs();
    DELETE_CTORS_AND_ASSIGN_OPS(InputWidgetOutputs);

public:
    void sendUserInput(const QString &msg) { virt_sendUserInput(msg); }
    void displayMessage(const QString &msg) { virt_displayMessage(msg); }
    void showMessage(const QString &msg, const int timeout) { virt_showMessage(msg, timeout); }
    void gotPasswordInput(const QString &password) { virt_gotPasswordInput(password); }

private:
    virtual void virt_sendUserInput(const QString &msg) = 0;
    virtual void virt_displayMessage(const QString &msg) = 0;
    virtual void virt_showMessage(const QString &msg, int timeout) = 0;
    virtual void virt_gotPasswordInput(const QString &password) = 0;
};

class NODISCARD_QOBJECT InputWidget final : public QPlainTextEdit
{
    Q_OBJECT

private:
    using base = QPlainTextEdit;

private:
    InputWidgetOutputs &m_outputs;
    QString m_tabFragment;
    TabHistory m_tabHistory;
    InputHistory m_inputHistory;
    PaletteManager m_paletteManager;
    bool m_tabbing = false;

public:
    explicit InputWidget(QWidget *parent, InputWidgetOutputs &);
    ~InputWidget() final;

    NODISCARD QSize sizeHint() const override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;

private:
    void gotInput();
    NODISCARD bool tryHistory(int);
    void keypadMovement(int);
    void functionKeyPressed(const QString &keyName);

private:
    void tabComplete();

private:
    void forwardHistory();
    void backwardHistory();

private:
    void sendUserInput(const QString &msg) { m_outputs.sendUserInput(msg); }
};
