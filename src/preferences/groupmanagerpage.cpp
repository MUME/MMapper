// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "groupmanagerpage.h"

#include "../configuration/configuration.h"
#include "../pandoragroup/CGroup.h"
#include "../pandoragroup/groupauthority.h"
#include "../pandoragroup/mmapper2group.h"
#include "AnsiColorDialog.h"
#include "ansicombo.h"
#include "ui_groupmanagerpage.h"

#include <QSet>
#include <QString>
#include <QtGui>
#include <QtWidgets>

namespace mmqt {
NODISCARD static GroupSecret getSecretBytes(const QString &secretText)
{
    return GroupSecret{secretText.toUtf8().toLower()};
}
} // namespace mmqt

GroupManagerPage::GroupManagerPage(Mmapper2Group *const gm, QWidget *const parent)
    : QWidget(parent)
    , m_groupManager(gm)
    , ui(new Ui::GroupManagerPage)
{
    ui->setupUi(this);
    GroupAuthority *const authority = m_groupManager->getAuthority();
    std::ignore = deref(authority);

    connect(this, &GroupManagerPage::sig_refresh, authority, &GroupAuthority::slot_refresh);
    connect(authority, &GroupAuthority::sig_secretRefreshed, this, [this](const GroupSecret &secret) {
        ui->secretLineEdit->setText(secret.getQByteArray());
        ui->refreshButton->setEnabled(true);
    });

    // Character Section
    connect(ui->charName,
            &QLineEdit::editingFinished,
            this,
            &GroupManagerPage::slot_charNameTextChanged);
    connect(ui->changeColor,
            &QAbstractButton::clicked,
            this,
            &GroupManagerPage::slot_changeColorClicked);
    connect(ui->refreshButton, &QAbstractButton::clicked, this, [this]() {
        ui->refreshButton->setEnabled(false);
        // Refreshing the SSL certificate asynchronously
        emit sig_refresh();
    });

    // Authorized Secrets Section
    connect(ui->authorizationCheckBox, &QCheckBox::clicked, this, [this](const bool checked) {
        ui->allowedComboBox->setEnabled(checked);
        if (!checked) {
            ui->allowedComboBox->setCurrentText("");
        }
        setConfig().groupManager.requireAuth = checked;
        slot_allowedSecretsChanged();
    });
    connect(ui->allowedComboBox,
            &QComboBox::editTextChanged,
            this,
            &GroupManagerPage::slot_allowedSecretsChanged);
    connect(authority->getItemModel(), &QAbstractItemModel::dataChanged, this, [this]() {
        slot_allowedSecretsChanged();
    });

    static auto get_secret = [](GroupManagerPage *const self) -> GroupSecret {
        GroupManagerPage &page = deref(self);
        Ui::GroupManagerPage &page_ui = deref(page.ui);
        QComboBox &combo = deref(page_ui.allowedComboBox);
        const QString s = combo.currentText().simplified();
        return mmqt::getSecretBytes(s);
    };
    connect(ui->allowSecret, &QPushButton::pressed, this, [this, authority]() {
        auto secret = get_secret(this);
        authority->add(secret);
    });
    connect(ui->revokeSecret, &QPushButton::pressed, this, [this, authority]() {
        auto secret = get_secret(this);
        authority->remove(secret);
    });

    // Host Section
    connect(ui->localHost, &QLabel::linkActivated, this, [](const QString &link) {
        QDesktopServices::openUrl(QUrl::fromEncoded(link.toUtf8())); //  percent-encoded ASCII
    });
    connect(ui->localPort,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &GroupManagerPage::slot_localPortValueChanged);
    connect(ui->shareSelfCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GroupManagerPage::slot_shareSelfChanged);
    connect(ui->localPort,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &GroupManagerPage::slot_localPortValueChanged);
    connect(ui->shareSelfCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GroupManagerPage::slot_shareSelfChanged);
    connect(ui->lockGroupCheckBox, &QCheckBox::stateChanged, this, [this]() {
        setConfig().groupManager.lockGroup = ui->lockGroupCheckBox->isChecked();
    });

    connect(ui->remoteHost,
            &QComboBox::editTextChanged,
            this,
            &GroupManagerPage::slot_remoteHostTextChanged);

    // Group Tells Section
    connect(ui->groupTellColorPushButton, &QPushButton::pressed, this, [this]() {
        QString ansiString = AnsiColorDialog::getColor(getConfig().groupManager.groupTellColor,
                                                       this);
        AnsiCombo::makeWidgetColoured(ui->groupTellColorLabel, ansiString, false);
        setConfig().groupManager.groupTellColor = ansiString;
    });
    connect(ui->groupTellColorAnsi256RadioButton, &QAbstractButton::toggled, this, [this]() {
        setConfig().groupManager.useGroupTellAnsi256Color = ui->groupTellColorAnsi256RadioButton
                                                                ->isChecked();
    });

    // Other Sections
    connect(ui->rulesWarning,
            &QCheckBox::stateChanged,
            this,
            &GroupManagerPage::slot_rulesWarningChanged);

    // Inform Group Manager of changes
    connect(this,
            &GroupManagerPage::sig_updatedSelf,
            m_groupManager,
            &Mmapper2Group::slot_updateSelf);
}

GroupManagerPage::~GroupManagerPage()
{
    delete ui;
}

void GroupManagerPage::slot_loadConfig()
{
    const Configuration::GroupManagerSettings &settings = getConfig().groupManager;
    const auto authority = m_groupManager->getAuthority();
    const auto &itemModel = authority->getItemModel();
    ui->charName->setText(settings.charName);
    QPixmap charColorPixmap(16, 16);
    charColorPixmap.fill(settings.color);
    ui->changeColor->setIcon(QIcon(charColorPixmap));
    ui->secretLineEdit->setText(authority->getSecret().getQByteArray());
    ui->secretLineEdit->setEnabled(!NO_OPEN_SSL);
    ui->refreshButton->setEnabled(!NO_OPEN_SSL);
    ui->authorizationCheckBox->setChecked(settings.requireAuth);
    ui->authorizationCheckBox->setEnabled(!NO_OPEN_SSL);
    ui->allowSecret->setEnabled(false);
    ui->revokeSecret->setEnabled(false);
    ui->allowedComboBox->setEnabled(settings.requireAuth);
    ui->allowedComboBox->setModel(itemModel);
    ui->allowedComboBox->setEditText("");
    slot_allowedSecretsChanged();

    ui->localPort->setValue(settings.localPort);
    ui->shareSelfCheckBox->setChecked(settings.shareSelf);
    ui->lockGroupCheckBox->setChecked(settings.lockGroup);

    // Client Section
    loadRemoteHostConfig();

    ui->groupTellColorAnsi256RadioButton->setChecked(settings.useGroupTellAnsi256Color);
    AnsiCombo::makeWidgetColoured(ui->groupTellColorLabel, settings.groupTellColor, false);

    ui->rulesWarning->setChecked(settings.rulesWarning);
}

void GroupManagerPage::loadRemoteHostConfig()
{
    const Configuration::GroupManagerSettings &settings = getConfig().groupManager;
    const auto remoteHostText = QString("%1:%2").arg(settings.host).arg(settings.remotePort);

    const auto authority = m_groupManager->getAuthority();
    const auto &itemModel = authority->getItemModel();

    // Remember the old text because the combinations are going to be cleared
    const auto oldText = ui->remoteHost->currentText();
    ui->remoteHost->clear();
    bool remoteHostIsContact = false;
    QSet<QString> contacts;
    for (int i = 0; i < itemModel->rowCount(); ++i) {
        // Pre-populate entries from Authorized Contacts
        const auto key = GroupSecret{
            itemModel->index(i, 0).data(Qt::DisplayRole).toByteArray().toLower()};
        const auto ip = GroupAuthority::getMetadata(key, GroupMetadataEnum::IP_ADDRESS);
        const auto port = GroupAuthority::getMetadata(key, GroupMetadataEnum::PORT).toInt();
        const auto name = GroupAuthority::getMetadata(key, GroupMetadataEnum::NAME);

        // IP and Port must be valid
        if (ip.isEmpty() || port <= 0) {
            continue;
        }

        // Ignore contacts with duplicate ip/port combinations
        const auto contact = QString("%1:%2").arg(ip).arg(port);
        if (contacts.contains(contact)) {
            continue;
        }

        // Populate contact
        contacts.insert(contact);
        ui->remoteHost->addItem(contact);
        ui->remoteHost->setItemData(i, name.isEmpty() ? "Unknown" : name, Qt::ToolTipRole);
        if (remoteHostText.compare(contact, Qt::CaseInsensitive) == 0) {
            remoteHostIsContact = true;
        }
    }
    if (!remoteHostIsContact) {
        // Add the entry from config if it wasn't already prepopulated
        ui->remoteHost->addItem(remoteHostText);
        ui->remoteHost->setItemData(ui->remoteHost->count() - 1, "Unknown", Qt::ToolTipRole);
    }
    auto find_remote_host_index = [this, &remoteHostText, &oldText]() -> int {
        if (oldText.isEmpty()) {
            // On first start up select the remote host in the configuration
            for (int i = 0; i < ui->remoteHost->count(); ++i) {
                const auto itemText = ui->remoteHost->itemText(i);
                if (remoteHostText.compare(itemText, Qt::CaseInsensitive) == 0) {
                    return i;
                }
            }
        } else {
            const int i = ui->remoteHost->findText(oldText);
            if (i >= 0) {
                return i;
            }
        }
        return 0;
    };
    ui->remoteHost->setCurrentIndex(find_remote_host_index());
}

void GroupManagerPage::slot_charNameTextChanged()
{
    // REVISIT: Remove non-valid characters (numbers, punctuation, etc)
    QString newNameStr = ui->charName->text().simplified();

    // Force first character to be uppercase
    if (!newNameStr.isEmpty()) {
        newNameStr.replace(0, 1, newNameStr.front().toUpper());
    }

    // Apply corrections back to the input field
    if (ui->charName->text() != newNameStr) {
        ui->charName->setText(newNameStr);
    }

    // REVISIT: Rename this and other functions to charLabel
    setConfig().groupManager.charName = newNameStr;
    emit sig_updatedSelf();
}

void GroupManagerPage::slot_changeColorClicked()
{
    QColor &savedColor = setConfig().groupManager.color;
    const QColor newColor = QColorDialog::getColor(savedColor, this);
    if (newColor.isValid() && newColor != savedColor) {
        QPixmap charColorPixmap(16, 16);
        charColorPixmap.fill(newColor);
        ui->changeColor->setIcon(QIcon(charColorPixmap));
        savedColor = newColor;

        emit sig_updatedSelf();
    }
}

void GroupManagerPage::slot_allowedSecretsChanged()
{
    static constexpr const int SHA1_LENGTH = 40;
    const auto authority = m_groupManager->getAuthority();
    const auto secretText = ui->allowedComboBox->currentText().simplified().toLower();
    const bool correctLength = secretText.length() == SHA1_LENGTH;
    const bool isSelf = secretText == authority->getSecret().getQByteArray();
    const bool alreadyPresent = authority->validSecret(GroupSecret{secretText.toUtf8()});

    const bool enableAllowSecret = correctLength && !alreadyPresent && !isSelf;
    if (ui->allowSecret->hasFocus() && !enableAllowSecret) {
        ui->allowedComboBox->setFocus();
    }
    ui->allowSecret->setEnabled(enableAllowSecret);

    const bool enableRevokeSecret = correctLength && alreadyPresent && !isSelf;
    if (ui->revokeSecret->hasFocus() && !enableRevokeSecret) {
        ui->allowedComboBox->setFocus();
    }
    ui->revokeSecret->setEnabled(enableRevokeSecret);

    if (correctLength && alreadyPresent) {
        const auto key = mmqt::getSecretBytes(secretText);
        const auto lastLogin = GroupAuthority::getMetadata(key, GroupMetadataEnum::LAST_LOGIN);
        QString line;
        if (lastLogin.isEmpty()) {
            line = "<i>Never seen before</i>";
        } else {
            const auto name = GroupAuthority::getMetadata(key, GroupMetadataEnum::NAME);
            const auto ip = GroupAuthority::getMetadata(key, GroupMetadataEnum::IP_ADDRESS);
            const auto port = GroupAuthority::getMetadata(key, GroupMetadataEnum::PORT).toInt();
            line = QString("<i>Last seen %1%2 from %3</i>")
                       .arg(lastLogin)
                       .arg(name.isEmpty() ? "" : QString(" as '%2'").arg(name))
                       .arg(port > 0 ? QString("%1:%2").arg(ip).arg(port) : ip);
        }
        ui->secretMetadataLabel->setText(line);
    } else {
        ui->secretMetadataLabel->setText("");
    }

    ui->secretCountLabel->setText(QString("<i>%1 contact%2 found</i>")
                                      .arg(ui->allowedComboBox->count())
                                      .arg(ui->allowedComboBox->count() == 1 ? "" : "s"));
}

void GroupManagerPage::slot_remoteHostTextChanged()
{
    const QString currentText = ui->remoteHost->currentText().simplified();

    const auto parts = currentText.split(char_consts::C_COLON);
    if (parts.size() != 2) {
        // should this warn?
        return;
    }

    const QString &currentHost = parts[0];
    if (QString &savedHost = setConfig().groupManager.host; currentHost != savedHost) {
        savedHost = currentHost;
    }

    const auto currentRemotePort = static_cast<uint16_t>(QString(parts[1]).toInt());
    if (uint16_t &savedRemotePort = setConfig().groupManager.remotePort;
        currentRemotePort != savedRemotePort) {
        savedRemotePort = currentRemotePort;
    }
}

void GroupManagerPage::slot_localPortValueChanged(int /*unused*/)
{
    auto &savedLocalPort = setConfig().groupManager.localPort;
    const auto currentLocalPort = static_cast<uint16_t>(ui->localPort->value());
    if (currentLocalPort != savedLocalPort) {
        savedLocalPort = currentLocalPort;
    }
}

void GroupManagerPage::slot_rulesWarningChanged(int /*unused*/)
{
    setConfig().groupManager.rulesWarning = ui->rulesWarning->isChecked();
}

void GroupManagerPage::slot_shareSelfChanged(int /*unused*/)
{
    setConfig().groupManager.shareSelf = ui->shareSelfCheckBox->isChecked();
}
