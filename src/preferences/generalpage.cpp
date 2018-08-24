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

#include <QString>
#include <QtWidgets>

#include "../configuration/configuration.h"

/* TODO: merge with other use */
#if MMAPPER_NO_OPENSSL
static constexpr const bool NO_OPEN_SSL = true;
#else
static constexpr const bool NO_OPEN_SSL = false;
#endif

GeneralPage::GeneralPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    connect(remoteName, &QLineEdit::textChanged, this, &GeneralPage::remoteNameTextChanged);
    connect(remotePort, SIGNAL(valueChanged(int)), this, SLOT(remotePortValueChanged(int)));
    connect(localPort, SIGNAL(valueChanged(int)), this, SLOT(localPortValueChanged(int)));
    connect(tlsEncryptionCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::tlsEncryptionCheckBoxStateChanged);
    connect(proxyThreadedCheckBox, &QCheckBox::stateChanged, this, [this]() {
        Config().connection.proxyThreaded = proxyThreadedCheckBox->isChecked();
    });

    connect(emulatedExitsCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::emulatedExitsStateChanged);
    connect(showHiddenExitFlagsCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::showHiddenExitFlagsStateChanged);
    connect(showNotesCheckBox, &QCheckBox::stateChanged, this, &GeneralPage::showNotesStateChanged);

    connect(autoLoadFileName,
            &QLineEdit::textChanged,
            this,
            &GeneralPage::autoLoadFileNameTextChanged);
    connect(autoLoadCheck, &QCheckBox::stateChanged, this, &GeneralPage::autoLoadCheckStateChanged);

    connect(selectWorldFileButton,
            &QAbstractButton::clicked,
            this,
            &GeneralPage::selectWorldFileButtonClicked);

    connect(displayMumeClockCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::displayMumeClockStateChanged);

    const auto &config = Config();
    const auto &connection = config.connection;
    const auto &mumeNative = config.mumeNative;
    const auto &autoLoad = config.autoLoad;

    remoteName->setText(connection.remoteServerName);
    remotePort->setValue(connection.remotePort);
    localPort->setValue(connection.localPort);
    if (NO_OPEN_SSL) {
        tlsEncryptionCheckBox->setEnabled(false);
        tlsEncryptionCheckBox->setChecked(false);
    } else {
        tlsEncryptionCheckBox->setChecked(connection.tlsEncryption);
    }
    proxyThreadedCheckBox->setChecked(connection.proxyThreaded);

    emulatedExitsCheckBox->setChecked(mumeNative.emulatedExits);
    showHiddenExitFlagsCheckBox->setChecked(mumeNative.showHiddenExitFlags);
    showNotesCheckBox->setChecked(mumeNative.showNotes);

    autoLoadCheck->setChecked(autoLoad.autoLoadMap);
    autoLoadFileName->setText(autoLoad.fileName);

    displayMumeClockCheckBox->setChecked(config.mumeClock.display);
}

void GeneralPage::selectWorldFileButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Choose map file ...",
                                                    "",
                                                    "MMapper2 (*.mm2);;MMapper (*.map)");
    if (!fileName.isEmpty()) {
        autoLoadFileName->setText(fileName);
        autoLoadCheck->setChecked(true);
        auto &settings = Config().autoLoad;
        settings.fileName = fileName;
        settings.autoLoadMap = true;
    }
}

void GeneralPage::remoteNameTextChanged(const QString & /*unused*/)
{
    Config().connection.remoteServerName = remoteName->text();
}

void GeneralPage::remotePortValueChanged(int /*unused*/)
{
    Config().connection.remotePort = static_cast<quint16>(remotePort->value());
}

void GeneralPage::localPortValueChanged(int /*unused*/)
{
    Config().connection.localPort = static_cast<quint16>(localPort->value());
}

void GeneralPage::tlsEncryptionCheckBoxStateChanged(int /*unused*/)
{
    Config().connection.tlsEncryption = tlsEncryptionCheckBox->isChecked();
}

void GeneralPage::emulatedExitsStateChanged(int /*unused*/)
{
    Config().mumeNative.emulatedExits = emulatedExitsCheckBox->isChecked();
}

void GeneralPage::showHiddenExitFlagsStateChanged(int /*unused*/)
{
    Config().mumeNative.showHiddenExitFlags = showHiddenExitFlagsCheckBox->isChecked();
}

void GeneralPage::showNotesStateChanged(int /*unused*/)
{
    Config().mumeNative.showNotes = showNotesCheckBox->isChecked();
}

void GeneralPage::autoLoadFileNameTextChanged(const QString & /*unused*/)
{
    Config().autoLoad.fileName = autoLoadFileName->text();
}

void GeneralPage::autoLoadCheckStateChanged(int /*unused*/)
{
    Config().autoLoad.autoLoadMap = autoLoadCheck->isChecked();
}

void GeneralPage::displayMumeClockStateChanged(int /*unused*/)
{
    Config().mumeClock.display = displayMumeClockCheckBox->isChecked();
}
