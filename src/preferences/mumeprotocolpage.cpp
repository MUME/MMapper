/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

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

    connect(ui->IACPromptCheckBox,
            &QCheckBox::stateChanged,
            this, &MumeProtocolPage::IACPromptCheckBoxStateChanged);
    connect(ui->remoteEditCheckBox,
            &QCheckBox::stateChanged,
            this, &MumeProtocolPage::remoteEditCheckBoxStateChanged);
    connect(ui->internalEditorRadioButton,
            &QAbstractButton::toggled,
            this, &MumeProtocolPage::internalEditorRadioButtonChanged);
    connect(ui->externalEditorCommand,
            &QLineEdit::textChanged,
            this, &MumeProtocolPage::externalEditorCommandTextChanged);

    const auto &settings = Config().mumeClientProtocol;
    ui->IACPromptCheckBox->setChecked(settings.IAC_prompt_parser);
    ui->remoteEditCheckBox->setChecked(settings.remoteEditing);
    ui->internalEditorRadioButton->setChecked(settings.internalRemoteEditor);
    ui->externalEditorRadioButton->setChecked(!settings.internalRemoteEditor);
    ui->externalEditorCommand->setText(settings.externalRemoteEditorCommand);
}

MumeProtocolPage::~MumeProtocolPage()
{
    delete ui;
}

void MumeProtocolPage::IACPromptCheckBoxStateChanged(int /*unused*/)
{
    Config().mumeClientProtocol.IAC_prompt_parser = ui->IACPromptCheckBox->isChecked();
}

void MumeProtocolPage::remoteEditCheckBoxStateChanged(int /*unused*/)
{
    const auto useExternalEditor = ui->remoteEditCheckBox->isChecked();

    Config().mumeClientProtocol.remoteEditing = useExternalEditor;

    ui->externalEditorCommand->setEnabled(useExternalEditor);
    ui->externalEditorRadioButton->setEnabled(useExternalEditor);
    ui->internalEditorRadioButton->setEnabled(useExternalEditor);
}

void MumeProtocolPage::internalEditorRadioButtonChanged(bool /*unused*/)
{
    Config().mumeClientProtocol.internalRemoteEditor = ui->internalEditorRadioButton->isChecked();
}

void MumeProtocolPage::externalEditorCommandTextChanged(QString text)
{
    Config().mumeClientProtocol.externalRemoteEditorCommand = std::move(text);
}
