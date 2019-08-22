// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "generalpage.h"

#include <QString>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "ui_generalpage.h"

// Order of entries in charsetComboBox drop down
static_assert(static_cast<int>(CharacterEncoding::LATIN1) == 0);
static_assert(static_cast<int>(CharacterEncoding::UTF8) == 1);
static_assert(static_cast<int>(CharacterEncoding::ASCII) == 2);

GeneralPage::GeneralPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GeneralPage)
{
    ui->setupUi(this);

    connect(ui->remoteName, &QLineEdit::textChanged, this, &GeneralPage::remoteNameTextChanged);
    connect(ui->remotePort,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralPage::remotePortValueChanged);
    connect(ui->localPort,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralPage::localPortValueChanged);
    connect(ui->tlsEncryptionCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::tlsEncryptionCheckBoxStateChanged);
    connect(ui->charsetComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            [](const int index) {
                setConfig().general.characterEncoding = static_cast<CharacterEncoding>(index);
            });

    connect(ui->emulatedExitsCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::emulatedExitsStateChanged);
    connect(ui->showHiddenExitFlagsCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::showHiddenExitFlagsStateChanged);
    connect(ui->showNotesCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::showNotesStateChanged);

    connect(ui->showLaunchPanelCheckBox, &QCheckBox::stateChanged, this, [this]() {
        setConfig().general.noLaunchPanel = !ui->showLaunchPanelCheckBox->isChecked();
    });
    connect(ui->autoStartGroupManagerCheckBox, &QCheckBox::stateChanged, this, [this]() {
        setConfig().groupManager.autoStart = ui->autoStartGroupManagerCheckBox->isChecked();
    });
    connect(ui->checkForUpdateCheckBox, &QCheckBox::stateChanged, this, [this]() {
        setConfig().general.checkForUpdate = ui->checkForUpdateCheckBox->isChecked();
    });
    connect(ui->autoLoadFileName,
            &QLineEdit::textChanged,
            this,
            &GeneralPage::autoLoadFileNameTextChanged);
    connect(ui->autoLoadCheck,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::autoLoadCheckStateChanged);

    connect(ui->selectWorldFileButton,
            &QAbstractButton::clicked,
            this,
            &GeneralPage::selectWorldFileButtonClicked);

    connect(ui->displayMumeClockCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::displayMumeClockStateChanged);

    connect(ui->proxyThreadedCheckBox, &QCheckBox::stateChanged, this, [this]() {
        setConfig().connection.proxyThreaded = ui->proxyThreadedCheckBox->isChecked();
    });
    connect(ui->proxyConnectionStatusCheckBox, &QCheckBox::stateChanged, this, [this]() {
        setConfig().connection.proxyConnectionStatus = ui->proxyConnectionStatusCheckBox->isChecked();
    });

    connect(ui->configurationResetButton, &QAbstractButton::clicked, this, [this]() {
        QMessageBox::StandardButton reply
            = QMessageBox::question(this,
                                    "MMapper Factory Reset",
                                    "Are you sure you want to perform a factory reset?",
                                    QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            setConfig().reset();
        }
    });
    const auto &config = getConfig();
    const auto &connection = config.connection;
    const auto &mumeNative = config.mumeNative;
    const auto &autoLoad = config.autoLoad;
    const auto &general = config.general;

    ui->remoteName->setText(connection.remoteServerName);
    ui->remotePort->setValue(connection.remotePort);
    ui->localPort->setValue(connection.localPort);
    if (NO_OPEN_SSL) {
        ui->tlsEncryptionCheckBox->setEnabled(false);
        ui->tlsEncryptionCheckBox->setChecked(false);
    } else {
        ui->tlsEncryptionCheckBox->setChecked(connection.tlsEncryption);
    }
    ui->charsetComboBox->setCurrentIndex(static_cast<int>(general.characterEncoding));

    ui->emulatedExitsCheckBox->setChecked(mumeNative.emulatedExits);
    ui->showHiddenExitFlagsCheckBox->setChecked(mumeNative.showHiddenExitFlags);
    ui->showNotesCheckBox->setChecked(mumeNative.showNotes);

    ui->showLaunchPanelCheckBox->setChecked(!config.general.noLaunchPanel);
    ui->autoStartGroupManagerCheckBox->setChecked(config.groupManager.autoStart);
    ui->checkForUpdateCheckBox->setChecked(config.general.checkForUpdate);
    ui->autoLoadFileName->setText(autoLoad.fileName);
    ui->autoLoadCheck->setChecked(autoLoad.autoLoadMap);
    ui->autoLoadFileName->setEnabled(autoLoad.autoLoadMap);
    ui->selectWorldFileButton->setEnabled(autoLoad.autoLoadMap);

    ui->displayMumeClockCheckBox->setChecked(config.mumeClock.display);

    ui->proxyThreadedCheckBox->setChecked(connection.proxyThreaded);
    ui->proxyConnectionStatusCheckBox->setChecked(connection.proxyConnectionStatus);
}

GeneralPage::~GeneralPage()
{
    delete ui;
}

void GeneralPage::selectWorldFileButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Choose map file ...",
                                                    "",
                                                    "MMapper2 (*.mm2);;MMapper (*.map)");
    if (!fileName.isEmpty()) {
        ui->autoLoadFileName->setText(fileName);
        ui->autoLoadCheck->setChecked(true);
        auto &savedAutoLoad = setConfig().autoLoad;
        savedAutoLoad.fileName = fileName;
        savedAutoLoad.autoLoadMap = true;
    }
}

void GeneralPage::remoteNameTextChanged(const QString & /*unused*/)
{
    setConfig().connection.remoteServerName = ui->remoteName->text();
}

void GeneralPage::remotePortValueChanged(int /*unused*/)
{
    setConfig().connection.remotePort = static_cast<quint16>(ui->remotePort->value());
}

void GeneralPage::localPortValueChanged(int /*unused*/)
{
    setConfig().connection.localPort = static_cast<quint16>(ui->localPort->value());
}

void GeneralPage::tlsEncryptionCheckBoxStateChanged(int /*unused*/)
{
    setConfig().connection.tlsEncryption = ui->tlsEncryptionCheckBox->isChecked();
}

void GeneralPage::emulatedExitsStateChanged(int /*unused*/)
{
    setConfig().mumeNative.emulatedExits = ui->emulatedExitsCheckBox->isChecked();
}

void GeneralPage::showHiddenExitFlagsStateChanged(int /*unused*/)
{
    setConfig().mumeNative.showHiddenExitFlags = ui->showHiddenExitFlagsCheckBox->isChecked();
}

void GeneralPage::showNotesStateChanged(int /*unused*/)
{
    setConfig().mumeNative.showNotes = ui->showNotesCheckBox->isChecked();
}

void GeneralPage::autoLoadFileNameTextChanged(const QString & /*unused*/)
{
    setConfig().autoLoad.fileName = ui->autoLoadFileName->text();
}

void GeneralPage::autoLoadCheckStateChanged(int /*unused*/)
{
    setConfig().autoLoad.autoLoadMap = ui->autoLoadCheck->isChecked();
    ui->autoLoadFileName->setEnabled(ui->autoLoadCheck->isChecked());
    ui->selectWorldFileButton->setEnabled(ui->autoLoadCheck->isChecked());
}

void GeneralPage::displayMumeClockStateChanged(int /*unused*/)
{
    setConfig().mumeClock.display = ui->displayMumeClockCheckBox->isChecked();
}
