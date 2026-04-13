#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/macros.h"

#include <QDialog>

namespace Ui {
class ManagePasswordDialog;
}

class NODISCARD_QOBJECT ManagePasswordDialog final : public QDialog
{
    Q_OBJECT

private:
    Ui::ManagePasswordDialog *const ui;

public:
    explicit ManagePasswordDialog(QWidget *parent = nullptr);
    ~ManagePasswordDialog() final;

    void setAccountName(const QString &name);
    NODISCARD QString accountName() const;

    void setPassword(const QString &password);
    NODISCARD QString password() const;

signals:
    void sig_deleteRequested();
};
