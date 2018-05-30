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
#include "configuration.h"

#include "CGroup.h"
#include "mmapper2group.h"

#include <QColorDialog>
#include <QDesktopServices>
#include <QUrl>

GroupManagerPage::GroupManagerPage(Mmapper2Group *gm, QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    m_groupManager = gm;

    // Character Section
    connect(charName, SIGNAL(editingFinished()), SLOT(charNameTextChanged()));
    connect(changeColor, SIGNAL(clicked()), SLOT(changeColorClicked()));
    // Host Section
    connect(localHost,
            SIGNAL(linkActivated(const QString &)),
            SLOT(localHostLinkActivated(const QString &)));
    connect(localPort, SIGNAL(valueChanged(int)), SLOT(localPortValueChanged(int)));
    // Client Section
    connect(remoteHost, SIGNAL(editingFinished()), SLOT(remoteHostTextChanged()));
    connect(remotePort, SIGNAL(valueChanged(int)), SLOT(remotePortValueChanged(int)));
    // Checkbox Section
    connect(rulesWarning, SIGNAL(stateChanged(int)), SLOT(rulesWarningChanged(int)));
    connect(shareSelfCheckBox, SIGNAL(stateChanged(int)), SLOT(shareSelfChanged(int)));

    // Inform Group Manager of changes
    connect(this, SIGNAL(setGroupManagerType(int)), m_groupManager, SLOT(setType(int)));
    connect(this, SIGNAL(updatedSelf()), m_groupManager, SLOT(updateSelf()));

    charName->setText(Config().m_groupManagerCharName);
    QPixmap charColorPixmap(16, 16);
    charColorPixmap.fill(Config().m_groupManagerColor);
    changeColor->setIcon(QIcon(charColorPixmap));
    localPort->setValue(Config().m_groupManagerLocalPort);
    remoteHost->setText(Config().m_groupManagerHost);
    remotePort->setValue(Config().m_groupManagerRemotePort);
    rulesWarning->setChecked(Config().m_groupManagerRulesWarning);
    shareSelfCheckBox->setChecked(Config().m_groupManagerShareSelf);
}

void GroupManagerPage::charNameTextChanged()
{
    const QString newName = charName->text();
    if (!m_groupManager->getGroup()->isNamePresent(newName.toLatin1())
        && Config().m_groupManagerCharName != newName) {
        Config().m_groupManagerCharName = newName.toLatin1();

        emit updatedSelf();
    }
}

void GroupManagerPage::changeColorClicked()
{
    const QColor newColor = QColorDialog::getColor(Config().m_groupManagerColor, this);
    if (newColor.isValid() && newColor != Config().m_groupManagerColor) {
        QPixmap charColorPixmap(16, 16);
        charColorPixmap.fill(newColor);
        changeColor->setIcon(QIcon(charColorPixmap));
        Config().m_groupManagerColor = newColor;

        emit updatedSelf();
    }
}

void GroupManagerPage::remoteHostTextChanged()
{
    if (QString(remoteHost->text()).toLatin1() != Config().m_groupManagerHost) {
        Config().m_groupManagerHost = QString(remoteHost->text()).toLatin1();

        if (m_groupManager->getType() == Mmapper2Group::Client) {
            emit setGroupManagerType(Mmapper2Group::Off);
        }
    }
}

void GroupManagerPage::remotePortValueChanged(int /*unused*/)
{
    if (remotePort->value() != Config().m_groupManagerRemotePort) {
        Config().m_groupManagerRemotePort = remotePort->value();

        if (m_groupManager->getType() == Mmapper2Group::Client) {
            emit setGroupManagerType(Mmapper2Group::Off);
        }
    }
}

void GroupManagerPage::localHostLinkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl::fromEncoded(link.toLatin1()));
}

void GroupManagerPage::localPortValueChanged(int /*unused*/)
{
    if (localPort->value() != Config().m_groupManagerLocalPort) {
        Config().m_groupManagerLocalPort = localPort->value();

        if (m_groupManager->getType() == Mmapper2Group::Server) {
            emit setGroupManagerType(Mmapper2Group::Off);
        }
    }
}

void GroupManagerPage::rulesWarningChanged(int /*unused*/)
{
    Config().m_groupManagerRulesWarning = rulesWarning->isChecked();
}

void GroupManagerPage::shareSelfChanged(int /*unused*/)
{
    Config().m_groupManagerShareSelf = shareSelfCheckBox->isChecked();
}
