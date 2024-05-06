// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mumeprotocolpage.h"

#include "../configuration/configuration.h"
#include "ui_mumeprotocolpage.h"

#include <utility>

#include <QString>
#include <QtWidgets>

MumeProtocolPage::MumeProtocolPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MumeProtocolPage)
{
    ui->setupUi(this);

    connect(ui->remoteEditCheckBox,
            &QCheckBox::stateChanged,
            this,
            &MumeProtocolPage::slot_remoteEditCheckBoxStateChanged);
    connect(ui->internalEditorRadioButton,
            &QAbstractButton::toggled,
            this,
            &MumeProtocolPage::slot_internalEditorRadioButtonChanged);
    connect(ui->externalEditorCommand,
            &QLineEdit::textChanged,
            this,
            &MumeProtocolPage::slot_externalEditorCommandTextChanged);
    connect(ui->externalEditorBrowseButton,
            &QAbstractButton::clicked,
            this,
            &MumeProtocolPage::slot_externalEditorBrowseButtonClicked);
}

MumeProtocolPage::~MumeProtocolPage()
{
    delete ui;
}

void MumeProtocolPage::slot_loadConfig()
{
    const auto &settings = getConfig().mumeClientProtocol;
    ui->remoteEditCheckBox->setChecked(settings.remoteEditing);
    ui->internalEditorRadioButton->setChecked(settings.internalRemoteEditor);
    ui->externalEditorRadioButton->setChecked(!settings.internalRemoteEditor);
    ui->externalEditorCommand->setText(settings.externalRemoteEditorCommand);
    ui->externalEditorCommand->setEnabled(!settings.internalRemoteEditor);
    ui->externalEditorBrowseButton->setEnabled(!settings.internalRemoteEditor);
}

void MumeProtocolPage::slot_remoteEditCheckBoxStateChanged(int /*unused*/)
{
    const auto useRemoteEdit = ui->remoteEditCheckBox->isChecked();

    setConfig().mumeClientProtocol.remoteEditing = useRemoteEdit;

    ui->externalEditorRadioButton->setEnabled(useRemoteEdit);
    ui->internalEditorRadioButton->setEnabled(useRemoteEdit);
    ui->externalEditorBrowseButton->setEnabled(useRemoteEdit);
    ui->externalEditorCommand->setEnabled(useRemoteEdit);
}

void MumeProtocolPage::slot_internalEditorRadioButtonChanged(bool /*unused*/)
{
    const bool useInternalEditor = ui->internalEditorRadioButton->isChecked();

    setConfig().mumeClientProtocol.internalRemoteEditor = useInternalEditor;

    ui->externalEditorCommand->setEnabled(!useInternalEditor);
    ui->externalEditorBrowseButton->setEnabled(!useInternalEditor);
}

void MumeProtocolPage::slot_externalEditorCommandTextChanged(QString text)
{
    setConfig().mumeClientProtocol.externalRemoteEditorCommand = std::move(text);
}

void MumeProtocolPage::slot_externalEditorBrowseButtonClicked(bool /*unused*/)
{
    auto &command = setConfig().mumeClientProtocol.externalRemoteEditorCommand;
    QFileInfo dirInfo(command);
    const auto dir = dirInfo.exists() ? dirInfo.absoluteDir().absolutePath() : QDir::homePath();
    QString fileName = QFileDialog::getOpenFileName(this, "Choose editor...", dir, "Editor (*)");
    if (!fileName.isEmpty()) {
        QString quotedFileName = QString(R"("%1")").arg(fileName.replace(R"(")", R"(\")"));
        ui->externalEditorCommand->setText(quotedFileName);
        command = quotedFileName;
    }
}
