// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "UpdateDialog.h"

#include <QGridLayout>
#include <QIcon>
#include <QPushButton>

UpdateDialog::UpdateDialog(QWidget *const parent)
    : QDialog(parent)
    , m_checker(this)
{
    setWindowTitle("MMapper Updater");
    setWindowIcon(QIcon(":/icons/mmapper-lo.svg"));

    m_text = new QLabel(this);
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("&Upgrade"));
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_checker.isUpgradeAvailable());
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &UpdateDialog::accepted);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        if (m_checker.upgrade()) {
            close();
        }
    });
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &UpdateDialog::reject);

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(m_text);
    mainLayout->addWidget(m_buttonBox);

    connect(&m_checker, &UpdateChecker::sig_statusTextChanged, this, [this]() {
        m_text->setText(m_checker.getStatusText());
    });
    connect(&m_checker, &UpdateChecker::sig_upgradeAvailableChanged, this, [this]() {
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_checker.isUpgradeAvailable());
    });
    connect(&m_checker, &UpdateChecker::sig_showDialog, this, [this]() {
        show();
        raise();
        activateWindow();
    });
}

void UpdateDialog::open()
{
    m_checker.check();
}
