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

#include "groupmanagerpage.h"

#include <QString>
#include <QtGui>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "../pandoragroup/CGroup.h"
#include "../pandoragroup/groupauthority.h"
#include "../pandoragroup/mmapper2group.h"

GroupManagerPage::GroupManagerPage(Mmapper2Group *gm, QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    m_groupManager = gm;
    const Configuration::GroupManagerSettings &groupManager = getConfig().groupManager;
    auto authority = m_groupManager->getAuthority();
    connect(this, &GroupManagerPage::refresh, authority, &GroupAuthority::refresh);
    connect(authority, &GroupAuthority::secretRefreshed, this, [this](const GroupSecret &secret) {
        secretLineEdit->setText(secret);
        refreshButton->setEnabled(true);
        emit setGroupManagerType(GroupManagerState::Off);
    });

    // Character Section
    connect(charName, &QLineEdit::editingFinished, this, &GroupManagerPage::charNameTextChanged);
    connect(changeColor, &QAbstractButton::clicked, this, &GroupManagerPage::changeColorClicked);
    connect(refreshButton, &QAbstractButton::clicked, this, [this]() {
        refreshButton->setEnabled(false);
        // Refreshing the SSL certificate asynchronously
        emit refresh();
    });
    charName->setText(groupManager.charName);
    QPixmap charColorPixmap(16, 16);
    charColorPixmap.fill(groupManager.color);
    changeColor->setIcon(QIcon(charColorPixmap));
    secretLineEdit->setText(authority->getSecret());

    // Authorized Secrets Section
    connect(authorizationCheckBox, &QCheckBox::clicked, this, [this](const bool checked) {
        allowedComboBox->setEnabled(checked);
        if (!checked) {
            allowedComboBox->setCurrentText("");
        }
        setConfig().groupManager.requireAuth = checked;
        allowedSecretsChanged();
    });
    connect(allowedComboBox,
            &QComboBox::editTextChanged,
            this,
            &GroupManagerPage::allowedSecretsChanged);
    connect(authority->getItemModel(), &QAbstractItemModel::dataChanged, this, [this]() {
        allowedSecretsChanged();
    });
    connect(allowSecret, &QPushButton::pressed, this, [this]() {
        auto authority = m_groupManager->getAuthority();
        auto secret = allowedComboBox->currentText().simplified().toLatin1();
        authority->add(secret);
    });
    connect(revokeSecret, &QPushButton::pressed, this, [this]() {
        auto authority = m_groupManager->getAuthority();
        auto secret = allowedComboBox->currentText().simplified().toLatin1();
        authority->remove(secret);
    });
    secretLineEdit->setEnabled(!NO_OPEN_SSL);
    refreshButton->setEnabled(!NO_OPEN_SSL);
    authorizationCheckBox->setChecked(groupManager.requireAuth);
    authorizationCheckBox->setEnabled(!NO_OPEN_SSL);
    allowSecret->setEnabled(false);
    revokeSecret->setEnabled(false);
    allowedComboBox->setEnabled(groupManager.requireAuth);
    allowedComboBox->setModel(authority->getItemModel());
    allowedComboBox->setEditText("");
    allowedSecretsChanged();

    // Host Section
    connect(localHost, &QLabel::linkActivated, this, &GroupManagerPage::localHostLinkActivated);
    connect(localPort,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GroupManagerPage::localPortValueChanged);
    connect(shareSelfCheckBox, &QCheckBox::stateChanged, this, &GroupManagerPage::shareSelfChanged);

    // Host Section
    connect(localHost, &QLabel::linkActivated, this, &GroupManagerPage::localHostLinkActivated);
    connect(localPort,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GroupManagerPage::localPortValueChanged);
    connect(shareSelfCheckBox, &QCheckBox::stateChanged, this, &GroupManagerPage::shareSelfChanged);
    localPort->setValue(groupManager.localPort);
    shareSelfCheckBox->setChecked(groupManager.shareSelf);

    // Client Section
    connect(remoteHost, &QLineEdit::editingFinished, this, &GroupManagerPage::remoteHostTextChanged);
    connect(remotePort,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GroupManagerPage::remotePortValueChanged);
    remoteHost->setText(groupManager.host);
    remotePort->setValue(groupManager.remotePort);

    // Other Sections
    connect(rulesWarning, &QCheckBox::stateChanged, this, &GroupManagerPage::rulesWarningChanged);
    rulesWarning->setChecked(groupManager.rulesWarning);

    // Inform Group Manager of changes
    connect(this, &GroupManagerPage::setGroupManagerType, m_groupManager, &Mmapper2Group::setType);
    connect(this, &GroupManagerPage::updatedSelf, m_groupManager, &Mmapper2Group::updateSelf);
}

void GroupManagerPage::charNameTextChanged()
{
    // REVISIT: Remove non-valid characters (numbers, punctuation, etc)
    const QByteArray newName = charName->text().toLatin1().simplified();
    QByteArray &savedCharName = setConfig().groupManager.charName;
    if (auto group = m_groupManager->getGroup()) {
        if (!group->isNamePresent(newName) && savedCharName != newName) {
            savedCharName = newName;
            emit updatedSelf();
        }
    }

    // Correct any UTF-8 characters that we do not understand
    QString newNameStr = QString::fromLatin1(newName);
    if (charName->text() != newNameStr) {
        charName->setText(newNameStr);
    }
}

void GroupManagerPage::changeColorClicked()
{
    QColor &savedColor = setConfig().groupManager.color;
    const QColor newColor = QColorDialog::getColor(savedColor, this);
    if (newColor.isValid() && newColor != savedColor) {
        QPixmap charColorPixmap(16, 16);
        charColorPixmap.fill(newColor);
        changeColor->setIcon(QIcon(charColorPixmap));
        savedColor = newColor;

        emit updatedSelf();
    }
}

void GroupManagerPage::allowedSecretsChanged()
{
    static constexpr const int SHA1_LENGTH = 40;
    const auto authority = m_groupManager->getAuthority();
    const auto secretText = allowedComboBox->currentText().simplified();
    bool correctLength = secretText.length() == SHA1_LENGTH;
    bool isSelf = secretText.compare(authority->getSecret(), Qt::CaseInsensitive) == 0;
    bool alreadyPresent = authority->isAuthorized(secretText.toLatin1());

    bool enableAllowSecret = correctLength && !alreadyPresent && !isSelf;
    if (allowSecret->hasFocus() && !enableAllowSecret)
        allowedComboBox->setFocus();
    allowSecret->setEnabled(enableAllowSecret);

    bool enableRevokeSecret = correctLength && alreadyPresent && !isSelf;
    if (revokeSecret->hasFocus() && !enableRevokeSecret)
        allowedComboBox->setFocus();
    revokeSecret->setEnabled(enableRevokeSecret);

    if (authorizationCheckBox->isEnabled() && authorizationCheckBox->isChecked()) {
        secretCountLabel->setText(QString("<i>%1 contact%2 found</i>")
                                      .arg(allowedComboBox->count())
                                      .arg(allowedComboBox->count() == 1 ? "" : "s"));
    } else {
        secretCountLabel->setText("");
    }
}

void GroupManagerPage::remoteHostTextChanged()
{
    auto &savedHost = setConfig().groupManager.host;
    const auto currentHost = remoteHost->text().toLatin1();
    if (currentHost != savedHost) {
        savedHost = currentHost;

        if (m_groupManager->getType() == GroupManagerState::Client) {
            emit setGroupManagerType(GroupManagerState::Off);
        }
    }
}

void GroupManagerPage::remotePortValueChanged(int /*unused*/)
{
    auto &savedRemotePort = setConfig().groupManager.remotePort;
    const auto currentRemotePort = static_cast<quint16>(remotePort->value());
    if (currentRemotePort != savedRemotePort) {
        savedRemotePort = currentRemotePort;

        if (m_groupManager->getType() == GroupManagerState::Client) {
            emit setGroupManagerType(GroupManagerState::Off);
        }
    }
}

void GroupManagerPage::localHostLinkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl::fromEncoded(link.toLatin1()));
}

void GroupManagerPage::localPortValueChanged(int /*unused*/)
{
    auto &savedLocalPort = setConfig().groupManager.localPort;
    const auto currentLocalPort = static_cast<quint16>(this->localPort->value());
    if (currentLocalPort != savedLocalPort) {
        savedLocalPort = currentLocalPort;

        if (m_groupManager->getType() == GroupManagerState::Server) {
            emit setGroupManagerType(GroupManagerState::Off);
        }
    }
}

void GroupManagerPage::rulesWarningChanged(int /*unused*/)
{
    setConfig().groupManager.rulesWarning = rulesWarning->isChecked();
}

void GroupManagerPage::shareSelfChanged(int /*unused*/)
{
    setConfig().groupManager.shareSelf = shareSelfCheckBox->isChecked();
}
