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

#include "CGroupCommunicator.h"
#include "CGroup.h"

#include <QColorDialog>
#include <QDesktopServices>
#include <QUrl>

GroupManagerPage::GroupManagerPage(CGroup* gm, QWidget *parent)
  : QWidget(parent)
{
  setupUi(this);
  m_groupManager = gm;

  // Character Section
  connect( charName, SIGNAL( editingFinished() ), SLOT( charNameTextChanged() )  );
  connect( changeColor, SIGNAL(clicked()),SLOT(changeColorClicked()));
  connect( colorName, SIGNAL( editingFinished() ), SLOT( colorNameTextChanged() )  );
  // Host Section
  connect( localHost, SIGNAL( linkActivated (const QString&) ),
	   SLOT( localHostLinkActivated(const QString&) ));
  connect( localPort, SIGNAL( valueChanged(int) ), SLOT( localPortValueChanged(int) )  );
  // Client Section
  connect( remoteHost, SIGNAL( editingFinished() ), SLOT( remoteHostTextChanged() )  );
  connect( remotePort, SIGNAL( valueChanged(int) ), SLOT( remotePortValueChanged(int) )  );
  // Checkbox Section
  connect( rulesWarning, SIGNAL(stateChanged(int)), SLOT(rulesWarningChanged(int)));

  charName->setText( Config().m_groupManagerCharName );
  colorName->setText( Config().m_groupManagerColor.name() );
  colorLabel->setPalette(QPalette(Config().m_groupManagerColor));
  colorLabel->setAutoFillBackground(true);
  localPort->setValue( Config().m_groupManagerLocalPort );
  remoteHost->setText( Config().m_groupManagerHost );
  remotePort->setValue( Config().m_groupManagerRemotePort );
  rulesWarning->setChecked( Config().m_groupManagerRulesWarning );
}

void GroupManagerPage::charNameTextChanged()
{
  const QString newName = charName->text();
  if (!m_groupManager->isNamePresent(newName.toAscii()) &&
      Config().m_groupManagerCharName != newName) {
    Config().m_groupManagerCharName = newName.toAscii();
    m_groupManager->resetName();
  }
}

void GroupManagerPage::colorNameTextChanged()
{
  const QColor newColor = QColor(colorName->text());
  if (newColor.isValid() && newColor != Config().m_groupManagerColor) {
    colorLabel->setPalette(QPalette(newColor));
    colorLabel->setAutoFillBackground(true);
    Config().m_groupManagerColor = newColor;
    m_groupManager->resetColor();
  }
}

void GroupManagerPage::changeColorClicked()
{
  const QColor newColor = QColorDialog::getColor(Config().m_groupManagerColor, this);
  if (newColor.isValid() && newColor != Config().m_groupManagerColor) {
    colorName->setText(newColor.name());
    colorLabel->setPalette(QPalette(newColor));
    colorLabel->setAutoFillBackground(true);
    Config().m_groupManagerColor = newColor;
    m_groupManager->resetColor();
  }
}

void GroupManagerPage::remoteHostTextChanged()
{
  if (QString(remoteHost->text()).toAscii() != Config().m_groupManagerHost) {
    Config().m_groupManagerHost = QString(remoteHost->text()).toAscii();
  }

  if (m_groupManager->isConnected() && m_groupManager->getType() == CGroupCommunicator::Client)
    m_groupManager->reconnect();
}

void GroupManagerPage::remotePortValueChanged(int)
{
  if (remotePort->value() != Config().m_groupManagerRemotePort) {
    Config().m_groupManagerRemotePort = remotePort->value();
  }
  
  if (m_groupManager->isConnected() && m_groupManager->getType() == CGroupCommunicator::Client)
    m_groupManager->reconnect();
}

void GroupManagerPage::localHostLinkActivated(const QString &link) {
  QDesktopServices::openUrl(QUrl::fromEncoded(link.toAscii()));
}

void GroupManagerPage::localPortValueChanged(int)
{
  if (localPort->value() != Config().m_groupManagerLocalPort && localPort->value() != int(Config().m_localPort))
    Config().m_groupManagerLocalPort = localPort->value();
  
  if (m_groupManager->getType() == CGroupCommunicator::Server)
    m_groupManager->reconnect();
}

void GroupManagerPage::rulesWarningChanged(int)
{
  Config().m_groupManagerRulesWarning = rulesWarning->isChecked();
}
