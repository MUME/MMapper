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
    connect(localHost,
            &QLabel::linkActivated,
            this, &GroupManagerPage::localHostLinkActivated);
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

    charName->setText(Config().groupManager.charName);
    QPixmap charColorPixmap(16, 16);
    charColorPixmap.fill(Config().groupManager.color);
    changeColor->setIcon(QIcon(charColorPixmap));
    localPort->setValue(Config().groupManager.localPort);
    remoteHost->setText(Config().groupManager.host);
    remotePort->setValue(Config().groupManager.remotePort);
    rulesWarning->setChecked(Config().groupManager.rulesWarning);
    shareSelfCheckBox->setChecked(Config().groupManager.shareSelf);
}

void GroupManagerPage::charNameTextChanged()
{
    const QString newName = charName->text();
    if (!m_groupManager->getGroup()->isNamePresent(newName.toLatin1())
        && Config().groupManager.charName != newName) {
        Config().groupManager.charName = newName.toLatin1();

        emit updatedSelf();
    }
}

void GroupManagerPage::changeColorClicked()
{
    const QColor newColor = QColorDialog::getColor(Config().groupManager.color, this);
    if (newColor.isValid() && newColor != Config().groupManager.color) {
        QPixmap charColorPixmap(16, 16);
        charColorPixmap.fill(newColor);
        changeColor->setIcon(QIcon(charColorPixmap));
        Config().groupManager.color = newColor;

        emit updatedSelf();
    }
}

void GroupManagerPage::remoteHostTextChanged()
{
    if (QString(remoteHost->text()).toLatin1() != Config().groupManager.host) {
        Config().groupManager.host = QString(remoteHost->text()).toLatin1();

        if (m_groupManager->getType() == GroupManagerState::Client) {
            emit setGroupManagerType(GroupManagerState::Off);
        }
    }
}

void GroupManagerPage::remotePortValueChanged(int /*unused*/)
{
    if (remotePort->value() != Config().groupManager.remotePort) {
        Config().groupManager.remotePort = remotePort->value();

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
    if (localPort->value() != Config().groupManager.localPort) {
        Config().groupManager.localPort = localPort->value();

        if (m_groupManager->getType() == GroupManagerState::Server) {
            emit setGroupManagerType(GroupManagerState::Off);
        }
    }
}

void GroupManagerPage::rulesWarningChanged(int /*unused*/)
{
    Config().groupManager.rulesWarning = rulesWarning->isChecked();
}

void GroupManagerPage::shareSelfChanged(int /*unused*/)
{
    Config().groupManager.shareSelf = shareSelfCheckBox->isChecked();
}
