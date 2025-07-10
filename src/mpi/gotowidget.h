#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../global/macros.h"

#include <QLineEdit>
#include <QWidget>

class QKeyEvent;

class NODISCARD_QOBJECT GotoWidget final : public QWidget
{
    Q_OBJECT

private:
    QLineEdit *m_lineEdit;

public:
    explicit GotoWidget(QWidget *parent = nullptr);
    ~GotoWidget() override;

    void setFocusToInput();

signals:
    void sig_gotoLineRequested(int lineNum);
    void sig_closeRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;
};
