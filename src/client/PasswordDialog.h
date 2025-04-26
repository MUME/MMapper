#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <QDialog>
#include <QLineEdit>

struct InputWidgetOutputs;

class PasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasswordDialog(InputWidgetOutputs &outputs, QWidget *parent = nullptr);

private slots:
    void accept() override;
    void reject() override;

private:
    QLineEdit *m_passwordLineEdit;
    InputWidgetOutputs &m_outputs;
};
