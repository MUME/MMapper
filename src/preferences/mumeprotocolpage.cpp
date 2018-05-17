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

#include "ui_mumeprotocolpage.h"
#include <utility>

#include "configuration/configuration.h"

MumeProtocolPage::MumeProtocolPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MumeProtocolPage)
{
    ui->setupUi(this);

    connect(ui->IACPromptCheckBox, SIGNAL(stateChanged(int)), SLOT(IACPromptCheckBoxStateChanged(int)));
    connect(ui->remoteEditCheckBox, SIGNAL(stateChanged(int)),
            SLOT(remoteEditCheckBoxStateChanged(int)));
    connect(ui->internalEditorRadioButton, SIGNAL(toggled(bool)),
            SLOT(internalEditorRadioButtonChanged(bool)));
    connect(ui->externalEditorCommand, SIGNAL(textChanged(QString)),
            SLOT(externalEditorCommandTextChanged(QString)));

    ui->IACPromptCheckBox->setChecked(Config().m_IAC_prompt_parser);
    ui->remoteEditCheckBox->setChecked(Config().m_remoteEditing);
    ui->internalEditorRadioButton->setChecked(Config().m_internalRemoteEditor);
    ui->externalEditorRadioButton->setChecked(!Config().m_internalRemoteEditor);
    ui->externalEditorCommand->setText(Config().m_externalRemoteEditorCommand);
}

MumeProtocolPage::~MumeProtocolPage()
{
    delete ui;
}

void MumeProtocolPage::IACPromptCheckBoxStateChanged(int /*unused*/)
{
    Config().m_IAC_prompt_parser = ui->IACPromptCheckBox->isChecked();
}

void MumeProtocolPage::remoteEditCheckBoxStateChanged(int /*unused*/)
{
    Config().m_remoteEditing = ui->remoteEditCheckBox->isChecked();
    ui->externalEditorCommand->setEnabled(Config().m_remoteEditing);
    ui->externalEditorRadioButton->setEnabled(Config().m_remoteEditing);
    ui->internalEditorRadioButton->setEnabled(Config().m_remoteEditing);
}

void MumeProtocolPage::internalEditorRadioButtonChanged(bool /*unused*/)
{
    Config().m_internalRemoteEditor = ui->internalEditorRadioButton->isChecked();
}

void MumeProtocolPage::externalEditorCommandTextChanged(QString text)
{
    Config().m_externalRemoteEditorCommand = std::move(text);
}
