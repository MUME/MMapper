// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "ManagePasswordDialog.h"

#include "../global/ConfigConsts-Computed.h"
#include "../global/ConfigEnums.h"
#include "ui_ManagePasswordDialog.h"

#include <QLineEdit>
#include <QPushButton>

ManagePasswordDialog::ManagePasswordDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ManagePasswordDialog)
{
    ui->setupUi(this);

    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        setWindowTitle("Manage Account");
        ui->label_2->hide();
        ui->accountPassword->hide();
        ui->showPassword->hide();
        ui->deleteButton->hide();
    } else {
        connect(ui->showPassword, &QToolButton::toggled, this, [this](bool checked) {
            ui->accountPassword->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        });

        connect(ui->deleteButton, &QPushButton::clicked, this, [this]() {
            emit sig_deleteRequested();
            ui->accountPassword->clear();
            ui->deleteButton->setEnabled(false);
        });
    }
}

ManagePasswordDialog::~ManagePasswordDialog()
{
    delete ui;
}

void ManagePasswordDialog::setAccountName(const QString &name)
{
    ui->accountName->setText(name);
}

QString ManagePasswordDialog::accountName() const
{
    return ui->accountName->text();
}

void ManagePasswordDialog::setPassword(const QString &password)
{
    ui->accountPassword->setText(password);
    if constexpr (CURRENT_PLATFORM != PlatformEnum::Wasm) {
        ui->deleteButton->setEnabled(!password.isEmpty());
    }
}

QString ManagePasswordDialog::password() const
{
    return ui->accountPassword->text();
}
