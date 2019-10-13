// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mumeprotocolpage.h"

#include <utility>
#include <QString>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "ui_mumeprotocolpage.h"

MumeProtocolPage::MumeProtocolPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MumeProtocolPage)
{
    ui->setupUi(this);

    connect(ui->remoteEditCheckBox,
            &QCheckBox::stateChanged,
            this,
            &MumeProtocolPage::remoteEditCheckBoxStateChanged);
    connect(ui->internalEditorRadioButton,
            &QAbstractButton::toggled,
            this,
            &MumeProtocolPage::internalEditorRadioButtonChanged);
    connect(ui->externalEditorCommand,
            &QLineEdit::textChanged,
            this,
            &MumeProtocolPage::externalEditorCommandTextChanged);
}

MumeProtocolPage::~MumeProtocolPage()
{
    delete ui;
}

void MumeProtocolPage::loadConfig()
{
    const auto &settings = getConfig().mumeClientProtocol;
    ui->remoteEditCheckBox->setChecked(settings.remoteEditing);
    ui->internalEditorRadioButton->setChecked(settings.internalRemoteEditor);
    ui->externalEditorRadioButton->setChecked(!settings.internalRemoteEditor);
    ui->externalEditorCommand->setText(settings.externalRemoteEditorCommand);
    ui->externalEditorCommand->setEnabled(!settings.internalRemoteEditor);
}

void MumeProtocolPage::remoteEditCheckBoxStateChanged(int /*unused*/)
{
    const auto useExternalEditor = ui->remoteEditCheckBox->isChecked();

    setConfig().mumeClientProtocol.remoteEditing = useExternalEditor;

    ui->externalEditorRadioButton->setEnabled(useExternalEditor);
    ui->internalEditorRadioButton->setEnabled(useExternalEditor);
    ui->externalEditorCommand->setEnabled(!ui->internalEditorRadioButton->isChecked());
}

void MumeProtocolPage::internalEditorRadioButtonChanged(bool /*unused*/)
{
    setConfig().mumeClientProtocol.internalRemoteEditor = ui->internalEditorRadioButton->isChecked();
    ui->externalEditorCommand->setEnabled(!ui->internalEditorRadioButton->isChecked());
}

void MumeProtocolPage::externalEditorCommandTextChanged(QString text)
{
    setConfig().mumeClientProtocol.externalRemoteEditorCommand = std::move(text);
}
