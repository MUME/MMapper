// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "PasswordDialog.h"

#include "inputwidget.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

PasswordDialog::PasswordDialog(InputWidgetOutputs &outputs, QWidget *const parent)
    : QDialog(parent)
    , m_outputs(outputs)
{
    setWindowTitle("Password");

    m_passwordLineEdit = new QLineEdit(this);
    m_passwordLineEdit->setEchoMode(QLineEdit::Password);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                           | QDialogButtonBox::Cancel,
                                                       this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_passwordLineEdit);
    layout->addWidget(buttonBox);

    setLayout(layout);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &PasswordDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &PasswordDialog::reject);
}

bool PasswordDialog::focusNextPrevChild(bool /*next*/)
{
    // Disable tabbing
    return false;
}

void PasswordDialog::accept()
{
    QString password = m_passwordLineEdit->text();
    m_passwordLineEdit->clear();
    m_outputs.gotPasswordInput(password);
    QDialog::accept();
}

void PasswordDialog::reject()
{
    m_passwordLineEdit->clear();
    m_outputs.gotPasswordInput(QString());
    QDialog::reject();
}
