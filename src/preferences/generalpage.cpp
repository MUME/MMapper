/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include "generalpage.h"
#include "configuration/configuration.h"

#include <QFileDialog>

GeneralPage::GeneralPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    connect( remoteName, SIGNAL( textChanged(const QString &) ), this,
             SLOT( remoteNameTextChanged(const QString &) )  );
    connect( remotePort, SIGNAL( valueChanged(int) ), this, SLOT( remotePortValueChanged(int) )  );
    connect( localPort, SIGNAL( valueChanged(int) ), this, SLOT( localPortValueChanged(int) )  );
    connect( tlsEncryptionCheckBox, SIGNAL(stateChanged(int)),
             SLOT(tlsEncryptionCheckBoxStateChanged(int)));

    connect ( emulatedExitsCheckBox, SIGNAL(stateChanged(int)), SLOT(emulatedExitsStateChanged(int)));
    connect ( showHiddenExitFlagsCheckBox, SIGNAL(stateChanged(int)),
              SLOT(showHiddenExitFlagsStateChanged(int)));
    connect ( showNotesCheckBox, SIGNAL(stateChanged(int)), SLOT(showNotesStateChanged(int)));

    connect( autoLoadFileName, SIGNAL( textChanged(const QString &) ), this,
             SLOT( autoLoadFileNameTextChanged(const QString &) )  );
    connect( autoLoadCheck, SIGNAL(stateChanged(int)), SLOT(autoLoadCheckStateChanged(int)));

    connect( selectWorldFileButton, SIGNAL(clicked()), this, SLOT(selectWorldFileButtonClicked()));

    connect( displayMumeClockCheckBox, SIGNAL(stateChanged(int)),
             SLOT(displayMumeClockStateChanged(int)));

    remoteName->setText( Config().m_remoteServerName );
    remotePort->setValue( Config().m_remotePort );
    localPort->setValue( Config().m_localPort );
    tlsEncryptionCheckBox->setChecked( Config().m_tlsEncryption );

    emulatedExitsCheckBox->setChecked( Config().m_emulatedExits );
    showHiddenExitFlagsCheckBox->setChecked( Config().m_showHiddenExitFlags );
    showNotesCheckBox->setChecked( Config().m_showNotes );

    autoLoadCheck->setChecked( Config().m_autoLoadWorld );
    autoLoadFileName->setText( Config().m_autoLoadFileName );

    displayMumeClockCheckBox->setChecked(Config().m_displayMumeClock);
}

void GeneralPage::selectWorldFileButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Choose map file ...", "",
                                                    "MMapper2 (*.mm2);;MMapper (*.map)");
    if (!fileName.isEmpty()) {
        autoLoadFileName->setText( fileName );
        Config().m_autoLoadFileName = fileName;
        autoLoadCheck->setChecked(true);
        Config().m_autoLoadWorld = true;
    }
}

void GeneralPage::remoteNameTextChanged(const QString &)
{
    Config().m_remoteServerName = remoteName->text();
}

void GeneralPage::remotePortValueChanged(int)
{
    Config().m_remotePort = remotePort->value();
}

void GeneralPage::localPortValueChanged(int)
{
    Config().m_localPort = localPort->value();
}

void GeneralPage::tlsEncryptionCheckBoxStateChanged(int)
{
    Config().m_tlsEncryption = tlsEncryptionCheckBox->isChecked();
}

void GeneralPage::emulatedExitsStateChanged(int)
{
    Config().m_emulatedExits = emulatedExitsCheckBox->isChecked();
}

void GeneralPage::showHiddenExitFlagsStateChanged(int)
{
    Config().m_showHiddenExitFlags = showHiddenExitFlagsCheckBox->isChecked();
}

void GeneralPage::showNotesStateChanged(int)
{
    Config().m_showNotes = showNotesCheckBox->isChecked();
}

void GeneralPage::autoLoadFileNameTextChanged(const QString &)
{
    Config().m_autoLoadFileName = autoLoadFileName->text();
}

void GeneralPage::autoLoadCheckStateChanged(int)
{
    Config().m_autoLoadWorld = autoLoadCheck->isChecked();
}

void GeneralPage::displayMumeClockStateChanged(int)
{
    Config().m_displayMumeClock = displayMumeClockCheckBox->isChecked();
}
