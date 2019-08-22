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
    explicit StackedInputWidget(QWidget *parent = nullptr);
    ~StackedInputWidget() override;
    bool eventFilter(QObject *obj, QEvent *event) override;

public slots:
    void toggleEchoMode(bool);
    void gotPasswordInput();
    void gotMultiLineInput(const QString &);
    void cut();
    void copy();
    void paste();

signals:
    void sendUserInput(const QString &);
    void displayMessage(const QString &);
    void showMessage(const QString &, int);

private:
    bool m_localEcho = false;
    InputWidget *m_inputWidget = nullptr;
    QLineEdit *m_passwordWidget = nullptr;
};
