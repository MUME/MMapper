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
#include "../pandoragroup/mmapper2group.h"

GroupManagerPage::GroupManagerPage(Mmapper2Group *gm, QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    m_groupManager = gm;

    // Character Section
    connect(charName, &QLineEdit::editingFinished, this, &GroupManagerPage::charNameTextChanged);
    connect(changeColor, &QAbstractButton::clicked, this, &GroupManagerPage::changeColorClicked);
    // Host Section
    connect(localHost, &QLabel::linkActivated, this, &GroupManagerPage::localHostLinkActivated);
    connect(localPort, SIGNAL(valueChanged(int)), SLOT(localPortValueChanged(int)));
    // Client Section
    connect(remoteHost, &QLineEdit::editingFinished, this, &GroupManagerPage::remoteHostTextChanged);
    connect(remotePort, SIGNAL(valueChanged(int)), SLOT(remotePortValueChanged(int)));
    // Checkbox Section
    connect(rulesWarning, &QCheckBox::stateChanged, this, &GroupManagerPage::rulesWarningChanged);
    connect(shareSelfCheckBox, &QCheckBox::stateChanged, this, &GroupManagerPage::shareSelfChanged);

    // Inform Group Manager of changes
    connect(this, &GroupManagerPage::setGroupManagerType, m_groupManager, &Mmapper2Group::setType);
    connect(this, &GroupManagerPage::updatedSelf, m_groupManager, &Mmapper2Group::updateSelf);

    const Configuration::GroupManagerSettings &groupManager = getConfig().groupManager;
    charName->setText(groupManager.charName);
    QPixmap charColorPixmap(16, 16);
    charColorPixmap.fill(groupManager.color);
    changeColor->setIcon(QIcon(charColorPixmap));
    localPort->setValue(groupManager.localPort);
    remoteHost->setText(groupManager.host);
    remotePort->setValue(groupManager.remotePort);
    rulesWarning->setChecked(groupManager.rulesWarning);
    shareSelfCheckBox->setChecked(groupManager.shareSelf);
}

void GroupManagerPage::charNameTextChanged()
{
    const QString newName = charName->text();
    QByteArray &savedCharName = setConfig().groupManager.charName;
    if (!m_groupManager->getGroup()->isNamePresent(newName.toLatin1()) && savedCharName != newName) {
        savedCharName = newName.toLatin1();
        emit updatedSelf();
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
