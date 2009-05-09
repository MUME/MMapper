/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper2 project. 
** Maintained by Marek Krejza <krejza@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file COPYING included in the packaging of
** this file.  
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
*************************************************************************/

#include <QtGui>
#include "groupmanagerpage.h"
#include "configuration.h"

GroupManagerPage::GroupManagerPage(CGroup* gm, QWidget *parent)
  : QWidget(parent)
{
  setupUi(this);
  m_groupManager = gm;

  // Character Section
  connect( charName, SIGNAL( editingFinished() ), SLOT( charNameTextChanged() )  );
  connect( changeColor, SIGNAL(clicked()),SLOT(changeColorClicked()));
  connect( colorName, SIGNAL( editingFinished() ), SLOT( colorNameTextChanged() )  );
  // Client Section
  connect( localPort, SIGNAL( valueChanged(int) ), SLOT( localPortValueChanged(int) )  );
  // Server Section
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
  if (remotePort->value() != Config().m_groupManagerRemotePort) {
    Config().m_groupManagerHost = QString(remoteHost->text()).toAscii();
  }

  if (m_groupManager->isConnected() && m_groupManager->getType() == CGroupCommunicator::Client)
    m_groupManager->reconnect();
}

void GroupManagerPage::remotePortValueChanged(int)
{
  if (QString(remoteHost->text()).toAscii() != Config().m_groupManagerHost) {
    Config().m_groupManagerRemotePort = remotePort->value();
  }
  
  if (m_groupManager->isConnected() && m_groupManager->getType() == CGroupCommunicator::Client)
    m_groupManager->reconnect();
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
