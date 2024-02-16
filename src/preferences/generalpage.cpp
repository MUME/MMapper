// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "generalpage.h"

#include <QSslSocket>
#include <QString>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "ui_generalpage.h"

// Order of entries in charsetComboBox drop down
static_assert(static_cast<int>(CharacterEncodingEnum::LATIN1) == 0);
static_assert(static_cast<int>(CharacterEncodingEnum::UTF8) == 1);
static_assert(static_cast<int>(CharacterEncodingEnum::ASCII) == 2);

GeneralPage::GeneralPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GeneralPage)
{
    ui->setupUi(this);

    connect(ui->remoteName, &QLineEdit::textChanged, this, &GeneralPage::slot_remoteNameTextChanged);
    connect(ui->remotePort,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &GeneralPage::slot_remotePortValueChanged);
    connect(ui->localPort,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &GeneralPage::slot_localPortValueChanged);
    connect(ui->secureCheckBox, &QCheckBox::clicked, this, [](const bool checked) {
        setConfig().connection.tlsEncryption = checked;
    });
    connect(ui->webProxyCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        ui->remotePort->setEnabled(!checked);
        if (checked) {
            // WebSockets Secure is secure
            ui->secureCheckBox->setEnabled(false);
            ui->secureCheckBox->setChecked(true);
        } else {
            if (QSslSocket::supportsSsl()) {
                ui->secureCheckBox->setEnabled(true);
                ui->secureCheckBox->setChecked(getConfig().connection.tlsEncryption);
            } else {
                ui->secureCheckBox->setEnabled(false);
                ui->secureCheckBox->setChecked(false);
            }
        }
    });
    connect(ui->webProxyCheckBox, &QCheckBox::clicked, this, [](const bool checked) {
        setConfig().connection.webSocket = checked;
    });
    connect(ui->proxyListensOnAnyInterfaceCheckBox, &QCheckBox::stateChanged, this, [this]() {
        setConfig().connection.proxyListensOnAnyInterface = ui->proxyListensOnAnyInterfaceCheckBox
                                                                ->isChecked();
    });
    connect(ui->charsetComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [](const int index) {
                setConfig().general.characterEncoding = static_cast<CharacterEncodingEnum>(index);
            });

    connect(ui->emulatedExitsCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::slot_emulatedExitsStateChanged);
    connect(ui->showHiddenExitFlagsCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::slot_showHiddenExitFlagsStateChanged);
    connect(ui->showNotesCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::slot_showNotesStateChanged);

    connect(ui->showClientPanelCheckBox, &QCheckBox::stateChanged, this, [this]() {
        setConfig().general.noClientPanel = !ui->showClientPanelCheckBox->isChecked();
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
            &GeneralPage::slot_autoLoadFileNameTextChanged);
    connect(ui->autoLoadCheck,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::slot_autoLoadCheckStateChanged);
    connect(ui->selectWorldFileButton,
            &QAbstractButton::clicked,
            this,
            &GeneralPage::slot_selectWorldFileButtonClicked);

    connect(ui->displayMumeClockCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::slot_displayMumeClockStateChanged);

    connect(ui->displayXPStatusCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GeneralPage::slot_displayXPStatusStateChanged);

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
            emit sig_factoryReset();
        }
    });
}

GeneralPage::~GeneralPage()
{
    delete ui;
}

void GeneralPage::slot_loadConfig()
{
    const auto &config = getConfig();
    const auto &connection = config.connection;
    const auto &mumeNative = config.mumeNative;
    const auto &autoLoad = config.autoLoad;
    const auto &general = config.general;

    ui->remoteName->setText(connection.remoteServerName);
    ui->remotePort->setValue(connection.remotePort);
    ui->localPort->setValue(connection.localPort);
    if (!QSslSocket::supportsSsl()) {
        ui->secureCheckBox->setEnabled(false);
        ui->secureCheckBox->setChecked(false);
    } else {
        ui->secureCheckBox->setChecked(connection.tlsEncryption);
    }
    if (NO_WEBSOCKETS) {
        ui->webProxyCheckBox->setEnabled(false);
        ui->webProxyCheckBox->setChecked(false);
    } else {
        ui->webProxyCheckBox->setChecked(connection.webSocket);
    }
    ui->proxyListensOnAnyInterfaceCheckBox->setChecked(connection.proxyListensOnAnyInterface);
    ui->charsetComboBox->setCurrentIndex(static_cast<int>(general.characterEncoding));

    ui->emulatedExitsCheckBox->setChecked(mumeNative.emulatedExits);
    ui->showHiddenExitFlagsCheckBox->setChecked(mumeNative.showHiddenExitFlags);
    ui->showNotesCheckBox->setChecked(mumeNative.showNotes);

    ui->showClientPanelCheckBox->setChecked(!config.general.noClientPanel);
    ui->autoStartGroupManagerCheckBox->setChecked(config.groupManager.autoStart);
    ui->checkForUpdateCheckBox->setChecked(config.general.checkForUpdate);
    ui->checkForUpdateCheckBox->setDisabled(NO_UPDATER);
    ui->autoLoadFileName->setText(autoLoad.fileName);
    ui->autoLoadCheck->setChecked(autoLoad.autoLoadMap);
    ui->autoLoadFileName->setEnabled(autoLoad.autoLoadMap);
    ui->selectWorldFileButton->setEnabled(autoLoad.autoLoadMap);

    ui->displayMumeClockCheckBox->setChecked(config.mumeClock.display);

    ui->displayXPStatusCheckBox->setChecked(config.adventurePanel.getDisplayXPStatus());

    ui->proxyThreadedCheckBox->setChecked(connection.proxyThreaded);
    ui->proxyConnectionStatusCheckBox->setChecked(connection.proxyConnectionStatus);
}

void GeneralPage::slot_selectWorldFileButtonClicked(bool /*unused*/)
{
    // FIXME: code duplication
    const auto &savedLastMapDir = getConfig().autoLoad.lastMapDirectory;
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Choose map file ...",
                                                    savedLastMapDir,
                                                    "MMapper2 (*.mm2);;MMapper (*.map)");
    if (!fileName.isEmpty()) {
        ui->autoLoadFileName->setText(fileName);
        ui->autoLoadCheck->setChecked(true);
        auto &savedAutoLoad = setConfig().autoLoad;
        savedAutoLoad.fileName = fileName;
        savedAutoLoad.autoLoadMap = true;
    }
}

void GeneralPage::slot_remoteNameTextChanged(const QString & /*unused*/)
{
    setConfig().connection.remoteServerName = ui->remoteName->text();
}

void GeneralPage::slot_remotePortValueChanged(int /*unused*/)
{
    setConfig().connection.remotePort = static_cast<quint16>(ui->remotePort->value());
}

void GeneralPage::slot_localPortValueChanged(int /*unused*/)
{
    setConfig().connection.localPort = static_cast<quint16>(ui->localPort->value());
}

void GeneralPage::slot_emulatedExitsStateChanged(int /*unused*/)
{
    setConfig().mumeNative.emulatedExits = ui->emulatedExitsCheckBox->isChecked();
}

void GeneralPage::slot_showHiddenExitFlagsStateChanged(int /*unused*/)
{
    setConfig().mumeNative.showHiddenExitFlags = ui->showHiddenExitFlagsCheckBox->isChecked();
}

void GeneralPage::slot_showNotesStateChanged(int /*unused*/)
{
    setConfig().mumeNative.showNotes = ui->showNotesCheckBox->isChecked();
}

void GeneralPage::slot_autoLoadFileNameTextChanged(const QString & /*unused*/)
{
    setConfig().autoLoad.fileName = ui->autoLoadFileName->text();
}

void GeneralPage::slot_autoLoadCheckStateChanged(int /*unused*/)
{
    setConfig().autoLoad.autoLoadMap = ui->autoLoadCheck->isChecked();
    ui->autoLoadFileName->setEnabled(ui->autoLoadCheck->isChecked());
    ui->selectWorldFileButton->setEnabled(ui->autoLoadCheck->isChecked());
}

void GeneralPage::slot_displayMumeClockStateChanged(int /*unused*/)
{
    setConfig().mumeClock.display = ui->displayMumeClockCheckBox->isChecked();
}

void GeneralPage::slot_displayXPStatusStateChanged([[maybe_unused]] int)
{
    setConfig().adventurePanel.setDisplayXPStatus(ui->displayXPStatusCheckBox->isChecked());
}
