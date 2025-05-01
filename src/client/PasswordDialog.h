#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
#include "../global/macros.h"

#include <QDialog>
#include <QLineEdit>

struct InputWidgetOutputs;

class NODISCARD_QOBJECT PasswordDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit PasswordDialog(InputWidgetOutputs &outputs, QWidget *parent = nullptr);

protected:
    NODISCARD bool focusNextPrevChild(bool next) override;

private slots:
    void accept() override;
    void reject() override;

private:
    QLineEdit *m_passwordLineEdit;
    InputWidgetOutputs &m_outputs;
};
