/************************************************************************
**
** Authors:   Azazello <lachupe@gmail.com>,
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include "configuration.h"

#include "CGroup.h"
#include "CGroupCommunicator.h"
#include "CGroupChar.h"

#include <QMessageBox>
#include <QAction>
#include <QCloseEvent>

CGroup::CGroup(QByteArray name, MapData* md, QWidget *parent) : QTreeWidget(parent)
{
  m_mapData = md;

  setColumnCount(8);
  setHeaderLabels(QStringList() << tr("Name") << tr("Room Name") << tr("HP") << tr("Mana") << tr("Moves") << tr("HP") << tr("Mana") << tr("Moves"));
  setRootIsDecorated(false);
  setAlternatingRowColors(true);
  setSelectionMode(QAbstractItemView::NoSelection);
  clear();
  hide();

  CGroupChar *ch = new CGroupChar(m_mapData, this);
  ch->setName(name);
  ch->setPosition(0);
  ch->setColor(Config().m_groupManagerColor);
  chars.append(ch);
  self = ch;

  emit log("GroupManager", "Starting up the GroupManager.\r\n");

  network = new CGroupCommunicator(CGroupCommunicator::Off, this);
  network->changeType(Config().m_groupManagerState);
}

void CGroup::setType(int newState)
{
  if (newState != CGroupCommunicator::Off && Config().m_groupManagerRulesWarning)
    QMessageBox::information(this, "Warning: MUME Rules", QString("Using the GroupManager in PK situations is ILLEGAL according to RULES ACTIONS.\n\nBe sure to disable the GroupManager under such conditions."));
  network->changeType(newState);
}

void CGroup::resetAllChars()
{
  for (int i = 0; i < chars.size(); ++i) {
    delete chars[i];
  }
  chars.clear();
}

void CGroup::resetChars()
{
  for (int i = 0; i < chars.size(); ++i) {
    if (chars[i] != self)
      delete chars[i];
  }
  chars.clear();
  chars.append(self);
}

CGroup::~CGroup()
{
  resetAllChars();
  delete network;
}



void CGroup::resetColor()
{
  if (self->getColor() == Config().m_groupManagerColor )
    return;

  self->setColor(Config().m_groupManagerColor);
  issueLocalCharUpdate();
}

void CGroup::setCharPosition(unsigned int pos)
{
  if (self->getPosition() != pos) {
    self->setPosition(pos);
    issueLocalCharUpdate();
  }
}

void CGroup::issueLocalCharUpdate()
{
  QDomNode data = self->toXML();
  self->updateFromXML(data);
  network->sendCharUpdate(data);
}

QByteArray CGroup::getNameFromBlob(QDomNode blob)
{
  CGroupChar *newChar;

  newChar = new CGroupChar(m_mapData, this);
  newChar->updateFromXML(blob);

  QByteArray name = newChar->getName();
  delete newChar;

  return name;
}


bool CGroup::addChar(QDomNode node)
{
  CGroupChar *newChar;

  newChar = new CGroupChar(m_mapData, this);
  newChar->updateFromXML(node);
  if ( isNamePresent(newChar->getName()) == true || newChar->getName() == "") {
    qDebug("Adding new char failed. The name %s already existed.", newChar->getName().constData());
    delete newChar;
    return false;
  } else {
    emit log("GroupManager", QString("Added new char. Name %1.").arg(newChar->getName().constData()));
    chars.append(newChar);
    return true;
  }
}

void CGroup::removeChar(QByteArray name)
{
  CGroupChar *ch;
  if (name == Config().m_groupManagerCharName)
    return; // just in case... should never happen

  for (int i = 0; i < chars.size(); i++)
    if (chars[i]->getName() == name) {
    emit log("GroupManager", QString("Removing char %1 from the list.").arg(chars[i]->getName().constData()));
    ch = chars[i];
    chars.remove(i);
    delete ch;
    }
}

void CGroup::removeChar(QDomNode node)
{
  QByteArray name = CGroupChar::getNameFromXML(node);
  if (name == "")
    return;

  removeChar(name);
}


bool CGroup::isNamePresent(QByteArray name)
{
  for (int i = 0; i < chars.size(); i++)
    if (chars[i]->getName() == name) {
    emit log("GroupManager", QString("The name %1 is already present").arg(name.constData()));
    return true;
    }

    return false;
}

CGroupChar* CGroup::getCharByName(QByteArray name)
{
  for (int i = 0; i < chars.size(); i++)
    if (chars[i]->getName() == name)
      return chars[i];

  return NULL;
}

void CGroup::sendAllCharsData(CGroupClient *conn)
{
  for (int i = 0; i < chars.size(); i++)
    network->sendCharUpdate(conn, chars[i]->toXML());
}


void CGroup::updateChar(QDomNode blob)
{
  CGroupChar *ch;

  ch = getCharByName(CGroupChar::getNameFromXML(blob));
  if (ch == NULL)
    return;

  if (ch->updateFromXML(blob) == true)
    emit drawCharacters();
}


void CGroup::connectionRefused(QString message)
{
  emit log("GroupManager", QString("Connection refused: %1").arg(message.toAscii().constData()));
  QMessageBox::information(this, "groupManager", QString("Connection refused: %1.").arg(message));

}

void CGroup::connectionFailed(QString message)
{
  emit log("GroupManager", QString("Failed to connect: %1").arg(message.toAscii().constData()));
  QMessageBox::information(this, "groupManager", QString("Failed to connect: %1.").arg(message));
}

void CGroup::connectionClosed(QString message)
{
  emit log("GroupManager", QString("Connection closed: %1").arg(message.toAscii().constData()));
  QMessageBox::information(this, "groupManager", QString("Connection closed: %1.").arg(message));
}

void CGroup::connectionError(QString message)
{
  emit log("GroupManager", QString("Connection error: %1.").arg(message.toAscii().constData()));
  QMessageBox::information(this, "groupManager", QString("Connection error: %1.").arg(message));
}

void CGroup::serverStartupFailed(QString message)
{
  emit log("GroupManager", QString("Failed to start the Group server: %1").arg(message.toAscii().constData()));
  QMessageBox::information(this, "groupManager", QString("Failed to start the groupManager server: %1.").arg(message));
}

void CGroup::gotKicked(QDomNode message)
{

  if (message.nodeName() != "data") {
    qDebug( "Called gotKicked with wrong node. No data node.");
    return;
  }

  QDomNode e = message.firstChildElement();

  if (e.nodeName() != "text") {
    qDebug( "Called gotKicked with wrong node. No text node.");
    return;
  }

  QDomElement text = e.toElement();
  emit log("GroupManager", QString("You got kicked! Reason [nodename %1] : %2").arg(text.nodeName().toAscii().constData()).arg(text.text().toAscii().constData()));
//      QMessageBox::critical(this, "groupManager", QString("You got kicked! Reason: %1.").arg(text.text()));
}

void CGroup::gTellArrived(QDomNode node)
{

  if (node.nodeName() != "data") {
    qDebug( "Called gTellArrived with wrong node. No data node.");
    return;
  }

  QDomNode e = node.firstChildElement();

//      QDomElement root = node.toElement();
  QString from = e.toElement().attribute("from");


  if (e.nodeName() != "gtell") {
    qDebug( "Called gTellArrived with wrong node. No text node.");
    return;
  }

  QDomElement text = e.toElement();
  emit log("GroupManager", QString("GTell from %1, Arrived : %2").arg(from.toAscii().constData()).arg(text.text().toAscii().constData()));

  QByteArray tell = QString("\r\n" + from + " tells you [GT] '" + text.text() + "'\r\n").toAscii();

  emit displayGroupTellEvent(tell.constData());
}

void CGroup::sendGTell(QByteArray tell)
{
  network->sendGTell(tell);
}

void CGroup::resetName()
{
  if (self->getName() == Config().m_groupManagerCharName)
    return;

  QByteArray oldname = self->getName();
  QByteArray newname = Config().m_groupManagerCharName;

  //printf("Sending name update: %s, %s\r\n", (const char *) oldname, (const char *) newname);
  network->sendUpdateName(oldname, newname);
  network->renameConnection(oldname, newname);

  self->setName(Config().m_groupManagerCharName);
}

void CGroup::renameChar(QDomNode blob)
{
  if (blob.nodeName() != "data") {
    qDebug( "Called renameChar with wrong node. No data node.");
    return;
  }

  QDomNode e = blob.firstChildElement();

//      QDomElement root = node.toElement();
  QString oldname = e.toElement().attribute("oldname");
  QString newname = e.toElement().attribute("newname");

  emit log("GroupManager", QString("Renaming a char from %1 to %2").arg(oldname.toAscii().constData()).arg(newname.toAscii().constData()));

  CGroupChar *ch;
  ch = getCharByName(oldname.toAscii());
  if (ch == NULL)
    return;

  ch->setName(newname.toAscii());
  ch->updateLabels();
}


void CGroup::parseScoreInformation(QByteArray score)
{
  emit log("GroupManager", QString("Caught a score line: %1").arg(score.constData()));

  if (score.contains("mana, ") == true) {
    score.replace(" hits, ", "/");
    score.replace(" mana, and ", "/");
    score.replace(" moves.", "");

    QString temp = score;
    QStringList list = temp.split('/');

    /*
    qDebug( "Hp: %s", (const char *) list[0].toAscii());
    qDebug( "Hp max: %s", (const char *) list[1].toAscii());
    qDebug( "Mana: %s", (const char *) list[2].toAscii());
    qDebug( "Max Mana: %s", (const char *) list[3].toAscii());
    qDebug( "Moves: %s", (const char *) list[4].toAscii());
    qDebug( "Max Moves: %s", (const char *) list[5].toAscii());
    */

    self->setScore(list[0].toInt(), list[1].toInt(), list[2].toInt(), list[3].toInt(),
                   list[4].toInt(), list[5].toInt()                        );

    issueLocalCharUpdate();

  } else {
    // 399/529 hits and 121/133 moves.
    score.replace(" hits and ", "/");
    score.replace(" moves.", "");

    QString temp = score;
    QStringList list = temp.split('/');

    /*
    qDebug( "Hp: %s", (const char *) list[0].toAscii());
    qDebug( "Hp max: %s", (const char *) list[1].toAscii());
    qDebug( "Moves: %s", (const char *) list[2].toAscii());
    qDebug( "Max Moves: %s", (const char *) list[3].toAscii());
*/
    self->setScore(list[0].toInt(), list[1].toInt(), 0, 0,
                   list[2].toInt(), list[3].toInt()                        );

    issueLocalCharUpdate();
  }
}

void CGroup::parsePromptInformation(QByteArray prompt)
{
  QByteArray hp, mana, moves;

  if (prompt.indexOf('>') == -1)
    return; // false prompt

  hp = "Healthy";
  mana = "Full";
  moves = "A lot";

  int index = prompt.indexOf("HP:");
  if (index != -1) {
    hp = "";
    int k = index + 3;
    while (prompt[k] != ' ' && prompt[k] != '>' )
      hp += prompt[k++];
  }

  index = prompt.indexOf("Mana:");
  if (index != -1) {
    mana = "";
    int k = index + 5;
    while (prompt[k] != ' ' && prompt[k] != '>' )
      mana += prompt[k++];
  }

  index = prompt.indexOf("Move:");
  if (index != -1) {
    moves = "";
    int k = index + 5;
    while (prompt[k] != ' ' && prompt[k] != '>' )
      moves += prompt[k++];
  }

  self->setTextScore(hp, mana, moves);
}

void CGroup::parseStateChangeLine(int message, QByteArray line)
{

}

void CGroup::sendLog(const QString& text)
{
  emit log("GroupManager", text);
}
QByteArray CGroup::getName() const {
    return self->getName();
}
int CGroup::getType() const {
    return network->getType();
}
bool CGroup::isConnected() const {
    return network->isConnected();
}
void CGroup::reconnect() {
    resetChars();
    network->reconnect();
}
QDomNode CGroup::getLocalCharData() {
    return self->toXML();
}
void CGroup::closeEvent(QCloseEvent* event) {
    event->accept();
}

