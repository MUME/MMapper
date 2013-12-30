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

#include "CGroupChar.h"
#include "roomselection.h"
#include "mapdata.h"

#include <QDomNode>
#include <QTreeWidgetItem>

CGroupChar::CGroupChar(MapData* md, QTreeWidget* qtw)
{
  m_mapData = md;
  charTable = qtw;

  name = "";
  pos = 0;
  hp = 0;
  maxhp = 0;
  moves = 0;
  maxmoves = 0;
  mana = 0;
  maxmana = 0;
  state = NORMAL;
  textHP = "Healthy";
  textMoves = "Full";
  textMana = "Full";
  lastMovement = "";
  charItem = new QTreeWidgetItem(charTable);
}

CGroupChar::~CGroupChar()
{
  delete charItem;
}

void CGroupChar::setItemText(uint itemNumber, const QString& text)
{
  charItem->setText(itemNumber, text);
  charItem->setBackgroundColor(itemNumber, color);
  if (color.value() < 150)
    charItem->setTextColor(itemNumber, Qt::white);
  else
    charItem->setTextColor(itemNumber, Qt::black);
}

void CGroupChar::updateLabels()
{
  setItemText(0, name);
  if (pos == 0 || m_mapData->isEmpty()) {
    setItemText(1, "Unknown");
  } else {

    const RoomSelection* selection = m_mapData->select();
    const Room* r = m_mapData->getRoom(pos, selection);

    if (r == NULL)
      setItemText(1, "Unknown");
    else 
    {
      //setItemText(1, QString("%1:%2").arg(r->getId()).arg( QString((*r)[0].toString()) ));
      setItemText(1, QString((*r)[0].toString()));
    }
    m_mapData->unselect(pos, selection);
  }

  setItemText(2,textHP);
  setItemText(3, textMana);
  setItemText(4, textMoves);

  setItemText(5, QString("%1/%2").arg(hp).arg(maxhp) );
  setItemText(6, QString("%1/%2").arg(mana).arg(maxmana) );
  setItemText(7, QString("%1/%2").arg(moves).arg(maxmoves) );

  /*
  switch (state) {
    case BASHED:
      setItemText(8, "BASHED");
      break;
    case INCAPACITATED:
      setItemText(8, "INCAP");
      break;
    case DEAD:
      setItemText(8, "DEAD");
      break;
    default:
      setItemText(8, "Normal");
      break;
  }
  */
}

QDomNode CGroupChar::toXML()
{

  QDomDocument doc("charinfo");

  QDomElement root = doc.createElement("playerData");
  root.setAttribute("room", pos );
  root.setAttribute("name", QString(name) );
  root.setAttribute("color", color.name() );
  root.setAttribute("textHP", QString(textHP) );
  root.setAttribute("textMana", QString(textMana) );
  root.setAttribute("textMoves", QString(textMoves) );
  root.setAttribute("hp", hp );
  root.setAttribute("maxhp", maxhp );
  root.setAttribute("mana", mana);
  root.setAttribute("maxmana", maxmana);
  root.setAttribute("moves", moves );
  root.setAttribute("maxmoves", maxmoves );
  root.setAttribute("state", state );
  root.setAttribute("lastMovement", QString(lastMovement));

  doc.appendChild(root);

  return root;
}

bool CGroupChar::updateFromXML(QDomNode node)
{
  bool updated;


  updated = false;
  if (node.nodeName() != "playerData") {
    qDebug( "Called updateFromXML with wrong node. The name does not fit.");
    return false;
  }

  QString s;
  QByteArray str;
  int newval;

  QDomElement e = node.toElement();

  unsigned int newpos  = e.attribute("room").toInt();
  if (newpos != pos) {
    updated = true;
    pos = newpos;
  }


  str = e.attribute("name").toAscii();
  if (str != name) {
    updated = true;
    name = str;
  }

  str = e.attribute("lastMovement").toAscii();
  if (str != lastMovement) {
    updated = true;
    lastMovement = str;
  }


  str = e.attribute("color").toAscii();
  if (str != color.name().toAscii()) {
    updated = true;
    color = QColor(QString(str) );
  }

//  printf(" 6.\r\n");

  str = e.attribute("textHP").toAscii();
  if (s != textHP) {
    updated = true;
    textHP = str;
  }

  str = e.attribute("textMana").toAscii();
  if (s != textMana) {
    updated = true;
    textMana = str;
  }

  str = e.attribute("textMoves").toAscii();
  if (s != textMoves) {
    updated = true;
    textMoves = str;
  }

  newval  = e.attribute("hp").toInt();
  if (newval != hp) {
    updated = true;
    hp = newval;
  }

  newval  = e.attribute("maxhp").toInt();
  if (newval != maxhp) {
    updated = true;
    maxhp = newval;
  }

  newval  = e.attribute("mana").toInt();
  if (newval != mana) {
    updated = true;
    mana = newval;
  }

  newval  = e.attribute("maxmana").toInt();
  if (newval != maxmana) {
    updated = true;
    maxmana = newval;
  }

  newval  = e.attribute("moves").toInt();
  if (newval != moves) {
    updated = true;
    moves = newval;
  }

  newval  = e.attribute("maxmoves").toInt();
  if (newval != maxmoves) {
    updated = true;
    maxmoves = newval;
  }

  newval  = e.attribute("state").toInt();
  if (newval != state) {
    updated = true;
    state = newval;
  }

//  printf("Tut 6.\r\n");

  if (updated == true)
    updateLabels();

//  printf("Tut 7.\r\n");

  return updated; // hrmpf!
}

QByteArray CGroupChar::getNameFromXML(QDomNode node)
{
  if (node.nodeName() != "playerData") {
    qDebug( "Called updateFromXML with wrong node. The name does not fit.");
    return QByteArray();
  }

  QDomElement e = node.toElement();

  return e.attribute("name").toAscii();
}

