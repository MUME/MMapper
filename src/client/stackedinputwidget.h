#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QStackedWidget>
#include <QString>
#include <QtCore>

class InputWidget;
class QEvent;
class QLineEdit;
class QObject;
class QWidget;

class StackedInputWidget final : public QStackedWidget
{
private:
    Q_OBJECT

public:
    explicit StackedInputWidget(QWidget *parent);
    ~StackedInputWidget() final;

private:
    bool eventFilter(QObject *obj, QEvent *event) final;

public slots:
    void slot_toggleEchoMode(bool);
    void slot_gotPasswordInput();
    void slot_gotMultiLineInput(const QString &);
    void slot_cut();
    void slot_copy();
    void slot_paste();

signals:
    void sig_sendUserInput(const QString &);
    void sig_displayMessage(const QString &);
    void sig_showMessage(const QString &, int);

private:
    bool m_localEcho = false;
    InputWidget *m_inputWidget = nullptr;
    QLineEdit *m_passwordWidget = nullptr;
};
