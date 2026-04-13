// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "generalpage.h"

#include "../configuration/configuration.h"
#include "ui_generalpage.h"

#include <QSslSocket>
#include <QString>
#include <QtWidgets>

// Order of entries in charsetComboBox drop down
static_assert(static_cast<int>(CharacterEncodingEnum::LATIN1) == 0);
static_assert(static_cast<int>(CharacterEncodingEnum::UTF8) == 1);
static_assert(static_cast<int>(CharacterEncodingEnum::ASCII) == 2);

// Order of entries in themeComboBox drop down
static_assert(static_cast<int>(ThemeEnum::System) == 0);
static_assert(static_cast<int>(ThemeEnum::Dark) == 1);
static_assert(static_cast<int>(ThemeEnum::Light) == 2);

GeneralPage::GeneralPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GeneralPage)
    , passCfg(this)
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
    connect(ui->encryptionCheckBox, &QCheckBox::clicked, this, [](const bool checked) {
        setConfig().connection.tlsEncryption = checked;
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
    connect(ui->themeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &GeneralPage::slot_themeComboBoxChanged);

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

    connect(ui->proxyConnectionStatusCheckBox, &QCheckBox::stateChanged, this, [this]() {
        setConfig().connection.proxyConnectionStatus = ui->proxyConnectionStatusCheckBox->isChecked();
    });

    connect(ui->configurationResetButton, &QAbstractButton::clicked, this, [this]() {
        auto *box = new QMessageBox(QMessageBox::Question,
                                    "MMapper Factory Reset",
                                    "Are you sure you want to perform a factory reset?",
                                    QMessageBox::Yes | QMessageBox::No,
                                    this);
        box->setAttribute(Qt::WA_DeleteOnClose);
        connect(box, &QMessageBox::finished, this, [this](int result) {
            if (result == QMessageBox::Yes) {
                setConfig().reset();
                emit sig_reloadConfig();
            }
        });
        box->open();
    });

    connect(ui->configurationExportButton, &QAbstractButton::clicked, this, []() {
        QTemporaryFile temp(QDir::tempPath() + "/mmapper_XXXXXX.ini");
        temp.setAutoRemove(false);
        if (!temp.open()) {
            qWarning() << "Failed to create temporary file for export";
            return;
        }
        const QString fileName = temp.fileName();
        temp.close();

        {
            QSettings settings(fileName, QSettings::IniFormat);
            getConfig().writeTo(settings);
            settings.sync();
        }

        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            const QByteArray content = file.readAll();
            file.close();
            QFileDialog::saveFileContent(content, "mmapper.ini");
        }

        QFile::remove(fileName);
    });

    connect(ui->configurationImportButton, &QAbstractButton::clicked, this, [this]() {
        auto importFile = [this](const QString &fileName,
                                 const std::optional<QByteArray> &fileContent) {
            if (fileName.isEmpty() || !fileContent.has_value()) {
                return;
            }

            const QByteArray &content = fileContent.value();
            if (content.isEmpty()) {
                return;
            }

            QTemporaryFile temp(QDir::tempPath() + "/mmapper_import_XXXXXX.ini");
            temp.setAutoRemove(true);
            if (temp.open()) {
                temp.write(content);
                temp.close();

                {
                    auto &cfg = setConfig();
                    QSettings settings(temp.fileName(), QSettings::IniFormat);
                    cfg.readFrom(settings);
                    cfg.write();
                }
                emit sig_reloadConfig();
            }
        };

        const auto nameFilter = QStringLiteral("Configuration (*.ini);;All files (*)");
        QFileDialog::getOpenFileContent(nameFilter, importFile);
    });

    connect(ui->autoLogin, &QCheckBox::clicked, this, [this](bool /*checked*/) {
        setConfig().account.rememberLogin = ui->autoLogin->isChecked();
    });

    connect(&passCfg, &PasswordConfig::sig_error, this, [this](const QString &msg) {
        qWarning() << msg;
        auto *box = new QMessageBox(QMessageBox::Warning,
                                    "Password Error",
                                    msg,
                                    QMessageBox::Ok,
                                    this);
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->open();
    });

    connect(&passCfg, &PasswordConfig::sig_incomingPassword, this, [this](const QString &password) {
        if (ui->setPassword->property("requesting").toBool()) {
            ui->setPassword->setProperty("requesting", false);
            const QString accountName = getConfig().account.accountName;
            auto *dlg = new ManagePasswordDialog(this);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->setAccountName(accountName);
            dlg->setPassword(password);
            connect(dlg, &ManagePasswordDialog::sig_deleteRequested, this, [this]() {
                passCfg.deletePassword();
            });

            connect(dlg, &QDialog::accepted, this, [this, dlg]() {
                const QString newAccountName = dlg->accountName();
                const QString newPassword = dlg->password();
                if (!newPassword.isEmpty()) {
                    setConfig().account.accountName = newAccountName;
                    passCfg.setPassword(newPassword);
                }
            });
            dlg->open();
        }
    });

    connect(&passCfg, &PasswordConfig::sig_passwordSaved, this, [this]() {
        setConfig().account.accountPassword = true;
        updateAutoLoginEnabled();

        if (!ui->autoLogin->isChecked()) {
            auto *box = new QMessageBox(QMessageBox::Question,
                                        "Login",
                                        "A password was saved. Would you like to also enable "
                                        "automatic login for this account?",
                                        QMessageBox::Yes | QMessageBox::No,
                                        this);
            box->setAttribute(Qt::WA_DeleteOnClose);
            connect(box, &QMessageBox::finished, this, [this](int result) {
                if (result == QMessageBox::Yes) {
                    ui->autoLogin->setChecked(true);
                    setConfig().account.rememberLogin = true;
                }
            });
            box->open();
        }
    });

    connect(&passCfg, &PasswordConfig::sig_passwordDeleted, this, [this]() {
        auto &account = setConfig().account;
        account.accountPassword = false;
        account.rememberLogin = false;
        ui->autoLogin->setChecked(false);
        updateAutoLoginEnabled();
    });

    connect(ui->setPassword, &QPushButton::clicked, this, &GeneralPage::slot_setPasswordClicked);

    connect(ui->remoteName, &QLineEdit::textChanged, this, [this]() { updateAutoLoginEnabled(); });

    connect(ui->resourceLineEdit, &QLineEdit::textChanged, this, [](const QString &text) {
        setConfig().canvas.resourcesDirectory = text;
    });
    connect(ui->resourcePushButton, &QAbstractButton::clicked, this, [this](bool /*unused*/) {
        const auto &resourceDir = getConfig().canvas.resourcesDirectory;
        QString directory = QFileDialog::getExistingDirectory(ui->resourcePushButton,
                                                              "Choose resource location ...",
                                                              resourceDir);
        if (!directory.isEmpty()) {
            ui->resourceLineEdit->setText(directory);
            setConfig().canvas.resourcesDirectory = directory;
        }
    });

    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        ui->remotePort->setDisabled(true);
        ui->localPort->setDisabled(true);
        ui->charsetComboBox->setDisabled(true);
        ui->proxyListensOnAnyInterfaceCheckBox->setDisabled(true);
        ui->proxyConnectionStatusCheckBox->setDisabled(true);
        ui->resourceLineEdit->setDisabled(true);
        ui->resourcePushButton->setDisabled(true);
    }
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
    const auto &account = config.account;

    ui->remoteName->setText(connection.remoteServerName);
    ui->remotePort->setValue(connection.remotePort);
    ui->localPort->setValue(connection.localPort);
#ifdef Q_OS_WASM
    ui->encryptionCheckBox->setDisabled(true);
    ui->encryptionCheckBox->setChecked(true);
#else
    if (!QSslSocket::supportsSsl() && NO_WEBSOCKET) {
        ui->encryptionCheckBox->setEnabled(false);
        ui->encryptionCheckBox->setChecked(false);
    } else {
        ui->encryptionCheckBox->setChecked(connection.tlsEncryption);
    }
#endif
    ui->proxyListensOnAnyInterfaceCheckBox->setChecked(connection.proxyListensOnAnyInterface);
    ui->charsetComboBox->setCurrentIndex(static_cast<int>(general.characterEncoding));
    ui->themeComboBox->setCurrentIndex(static_cast<int>(general.getTheme()));

    ui->emulatedExitsCheckBox->setChecked(mumeNative.emulatedExits);
    ui->showHiddenExitFlagsCheckBox->setChecked(mumeNative.showHiddenExitFlags);
    ui->showNotesCheckBox->setChecked(mumeNative.showNotes);

    ui->checkForUpdateCheckBox->setChecked(config.general.checkForUpdate);
    ui->checkForUpdateCheckBox->setDisabled(NO_UPDATER);
    ui->autoLoadFileName->setText(autoLoad.fileName);
    ui->autoLoadCheck->setChecked(autoLoad.autoLoadMap);
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        ui->autoLoadFileName->setDisabled(true);
        ui->selectWorldFileButton->setDisabled(true);
    } else {
        ui->autoLoadFileName->setEnabled(autoLoad.autoLoadMap);
        ui->selectWorldFileButton->setEnabled(autoLoad.autoLoadMap);
    }

    ui->displayMumeClockCheckBox->setChecked(config.mumeClock.display);

    ui->displayXPStatusCheckBox->setChecked(config.adventurePanel.getDisplayXPStatus());

    ui->proxyConnectionStatusCheckBox->setChecked(connection.proxyConnectionStatus);

    ui->resourceLineEdit->setText(config.canvas.resourcesDirectory);

    if constexpr (NO_QTKEYCHAIN) {
        ui->autoLogin->setEnabled(false);
        ui->setPassword->setEnabled(false);
    } else {
        ui->autoLogin->setChecked(account.rememberLogin);
        updateAutoLoginEnabled();
    }
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
    setConfig().connection.remotePort = static_cast<uint16_t>(ui->remotePort->value());
}

void GeneralPage::slot_localPortValueChanged(int /*unused*/)
{
    setConfig().connection.localPort = static_cast<uint16_t>(ui->localPort->value());
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
    if (CURRENT_PLATFORM != PlatformEnum::Wasm) {
        ui->autoLoadFileName->setEnabled(ui->autoLoadCheck->isChecked());
        ui->selectWorldFileButton->setEnabled(ui->autoLoadCheck->isChecked());
    }
}

void GeneralPage::slot_displayMumeClockStateChanged(int /*unused*/)
{
    setConfig().mumeClock.display = ui->displayMumeClockCheckBox->isChecked();
}

void GeneralPage::slot_displayXPStatusStateChanged([[maybe_unused]] int)
{
    setConfig().adventurePanel.setDisplayXPStatus(ui->displayXPStatusCheckBox->isChecked());
}

void GeneralPage::slot_themeComboBoxChanged(int index)
{
    setConfig().general.setTheme(static_cast<ThemeEnum>(index));
}

void GeneralPage::updateAutoLoginEnabled()
{
    if constexpr (NO_QTKEYCHAIN) {
        ui->autoLogin->setEnabled(false);
        return;
    }

    const auto &account = getConfig().account;
    const bool hasAccountName = !account.accountName.isEmpty();
    const bool hasPassword = account.accountPassword;

    ui->autoLogin->setEnabled(hasAccountName && hasPassword);
}

void GeneralPage::slot_setPasswordClicked()
{
    const QString accountName = getConfig().account.accountName;

    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        auto *dlg = new ManagePasswordDialog(this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->setAccountName(accountName);
        connect(dlg, &QDialog::accepted, this, [this, dlg]() {
            const QString newAccountName = dlg->accountName();
            if (!newAccountName.isEmpty()) {
                setConfig().account.accountName = newAccountName;
                passCfg.setPassword("");
            }
        });
        dlg->open();
    } else {
        if (getConfig().account.accountPassword) {
            ui->setPassword->setProperty("requesting", true);
            passCfg.getPassword();
        } else {
            auto *dlg = new ManagePasswordDialog(this);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->setAccountName(accountName);
            connect(dlg, &ManagePasswordDialog::sig_deleteRequested, this, [this]() {
                passCfg.deletePassword();
            });

            connect(dlg, &QDialog::accepted, this, [this, dlg]() {
                const QString newAccountName = dlg->accountName();
                const QString password = dlg->password();
                if (!password.isEmpty()) {
                    setConfig().account.accountName = newAccountName;
                    passCfg.setPassword(password);
                }
            });
            dlg->open();
        }
    }
}
