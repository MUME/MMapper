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
  connect( charName, SIGNAL( textChanged(const QString&) ), SLOT( charNameTextChanged(const QString&) )  );
  connect( changeColor, SIGNAL(clicked()),SLOT(changeColorClicked()));
  connect( colorName, SIGNAL( textChanged(const QString&) ), SLOT( colorNameTextChanged(const QString&) )  );
  connect( acceptChar, SIGNAL( clicked() ), SLOT( acceptCharClicked() ) );
  // Client Section
  connect( localPort, SIGNAL( valueChanged(int) ), SLOT( localPortValueChanged(int) )  );
  connect( acceptClient, SIGNAL( clicked() ), SLOT( acceptClientClicked() )  );
  // Server Section
  connect( remoteHost, SIGNAL( textChanged(const QString&) ), SLOT( remoteHostTextChanged(const QString&) )  );
  connect( remotePort, SIGNAL( valueChanged(int) ), SLOT( remotePortValueChanged(int) )  );
  connect( acceptServer, SIGNAL( clicked() ), SLOT( acceptServerClicked() )  );
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

void GroupManagerPage::charNameTextChanged(const QString&)
{
  if (!m_groupManager->isNamePresent(QString(charName->text()).toAscii()))
    acceptChar->setEnabled(TRUE);
  else
    acceptChar->setEnabled(FALSE);
}

void GroupManagerPage::acceptCharClicked()
{
  if (Config().m_groupManagerCharName != QString(charName->text()).toAscii())
  {
    Config().m_groupManagerCharName = QString(charName->text()).toAscii();
    m_groupManager->resetName();
  }
  QColor newColor = QColor(colorName->text());
  if (newColor != Config().m_groupManagerColor)
  {
    Config().m_groupManagerColor = QColor(colorName->text());
    m_groupManager->resetColor();
  }
  acceptChar->setEnabled(FALSE);
}

void GroupManagerPage::colorNameTextChanged(const QString&)
{
  QColor newColor = QColor(colorName->text());
  if (newColor.isValid() && newColor != Config().m_groupManagerColor) {
    colorLabel->setPalette(QPalette(newColor));
    colorLabel->setAutoFillBackground(true);
    acceptChar->setEnabled(TRUE);
  }
  else
    acceptChar->setEnabled(FALSE);
}

void GroupManagerPage::changeColorClicked()
{
  QColor newColor = QColorDialog::getColor(Config().m_groupManagerColor, this);
  if (newColor.isValid() && newColor != Config().m_groupManagerColor) {
    colorName->setText(newColor.name());
    colorLabel->setPalette(QPalette(newColor));
    colorLabel->setAutoFillBackground(true);
    m_groupManager->resetColor();
    acceptChar->setEnabled(TRUE);
  }
  else
    acceptChar->setEnabled(FALSE);
}

void GroupManagerPage::remoteHostTextChanged(const QString&)
{
  if (QString(remoteHost->text()).toAscii() != Config().m_groupManagerHost)
    acceptClient->setEnabled(TRUE);
}

void GroupManagerPage::remotePortValueChanged(int)
{
  if (remotePort->value() != Config().m_groupManagerRemotePort)
    acceptClient->setEnabled(TRUE);
}

void GroupManagerPage::acceptClientClicked()
{
  acceptClient->setEnabled(FALSE);
  Config().m_groupManagerRemotePort = remotePort->value();
  Config().m_groupManagerHost = QString(remoteHost->text()).toAscii();

  if (m_groupManager->isConnected())
    m_groupManager->reconnect();
}

void GroupManagerPage::localPortValueChanged(int)
{
  if (localPort->value() != Config().m_groupManagerLocalPort && localPort->value() != int(Config().m_localPort))
    acceptServer->setEnabled(TRUE);
  else
    acceptServer->setEnabled(FALSE);
}

void GroupManagerPage::acceptServerClicked()
{
  acceptServer->setEnabled(FALSE);
  Config().m_groupManagerLocalPort = localPort->value();

  if (m_groupManager->getType() == CGroupCommunicator::Server)
    m_groupManager->reconnect();
}

void GroupManagerPage::rulesWarningChanged(int)
{
  Config().m_groupManagerRulesWarning = rulesWarning->isChecked();
}
