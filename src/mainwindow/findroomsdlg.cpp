/************************************************************************
**
** Authors:   Kalev Lember <kalev@smartlink.ee>,
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

#include "findroomsdlg.h"
#include "mmapper2room.h"  //getName
#include "mmapper2exit.h"  //getExit
#include "mapdata.h"

#include <QCloseEvent>
#include <QShortcut>

const QString FindRoomsDlg::nullString;

FindRoomsDlg::FindRoomsDlg(MapData* md, QWidget *parent)
  : QDialog(parent)
{
  setupUi(this);

  m_mapData = md;
  m_admin = NULL;
  adjustResultTable();

  m_showSelectedRoom = new QShortcut( QKeySequence( tr( "Space", "Select result item" ) ), resultTable );
  m_showSelectedRoom->setContext( Qt::WidgetShortcut );

  connect(lineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(enableFindButton(const QString &)));
  connect(findButton, SIGNAL(clicked()), this, SLOT(findClicked()));
  connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
  connect(resultTable, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem *)));
  connect(m_showSelectedRoom, SIGNAL(activated()), this, SLOT(showSelectedRoom()));
}

void FindRoomsDlg::findClicked()
{
  // Release rooms for a new search
  if (m_admin != NULL) m_mapData->unselect(m_roomSelection);

  m_roomSelection = m_mapData->select();
  Qt::CaseSensitivity cs = caseCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
  QString text = lineEdit->text();
  // remove latin1
  text = latinToAscii(text);
  resultTable->clear();
  roomsFoundLabel->clear();

  /*  for an absolute match do the below:
  m_mapData->lookingForRooms(this, createEvent(CID_UNKNOWN, text, nullString, nullString, 0, 0));
  */

  if (nameRadioButton->isChecked())
    m_mapData->searchNames(this, text, cs);
  else if (descRadioButton->isChecked())
    m_mapData->searchDescriptions(this, text, cs);
  else if (exitsRadioButton->isChecked())
    m_mapData->searchDoorNames(this, text, cs);
  else if (notesRadioButton->isChecked())
    m_mapData->searchNotes(this, text, cs);
}

void FindRoomsDlg::receiveRoom(RoomAdmin * sender, const Room * room)
{
  m_admin = sender;
  m_mapData->select(m_mapData->select(room->getId()), m_roomSelection);

//#define HACK_FIX_THE_HIDDEN_NAMELESS_EXITS
#ifdef HACK_FIX_THE_HIDDEN_NAMELESS_EXITS
  // Remember to also alter the way searching works to match the exits you want
  // to fix
  ExitsList exits = room->getExitsList();
  for(uint dir = 0; dir < exits.size(); ++dir)
  {
    const Exit & e = room->exit( dir );
    bool isSecret = ISSET( getFlags( e ), EF_DOOR ) && ISSET( getDoorFlags( e ), DF_HIDDEN );
    if (isSecret && QString((e)[0].toString()).isEmpty())
    {
      // I can't get MapActions to work here
      nandDoorFlags( const_cast<Room*>( room )->exit( dir ), DF_HIDDEN );
    }
  }
#endif

  QString id;
  id.setNum(room->getId());
  QString roomName = QString(getName(room));

  m_admin->releaseRoom(this, room->getId()); // When do I release rooms? Now? later?

  item = new QTreeWidgetItem(resultTable);
  item->setText(0, id);
  item->setText(1, roomName);
  roomsFoundLabel->setText(tr("%1 room(s) found").arg( resultTable->topLevelItemCount() ));
}

void FindRoomsDlg::showSelectedRoom()
{
  itemDoubleClicked( resultTable->currentItem() );
}

void FindRoomsDlg::itemDoubleClicked(QTreeWidgetItem *item)
{
  uint id;
  if (item == NULL)
    return;

  id = item->text(0).toInt();

  const Room *r = m_mapData->getRoom(id, m_roomSelection);
  Coordinate c = r->getPosition();
  emit center(c.x, c.y); // connects to MapWindow

  // taken from MapCanvas:
  QString etmp = "Exits:";
  for (int j = 0; j < 7; j++) {

    bool door = false;
    if (ISSET(getFlags(r->exit(j)),EF_DOOR))
    {
      door = true;
      etmp += " (";
    }

    if (ISSET(getFlags(r->exit(j)),EF_EXIT)) {
      if (!door) etmp += " ";

      switch(j)
      {
        case 0: etmp += "north"; break;
        case 1: etmp += "south"; break;
        case 2: etmp += "east"; break;
        case 3: etmp += "west"; break;
        case 4: etmp += "up"; break;
        case 5: etmp += "down"; break;
        case 6: etmp += "unknown"; break;
      }
    }

    if (door)
    {
      if (getDoorName(r->exit(j))!="")
        etmp += "/"+getDoorName(r->exit(j))+")";
      else
        etmp += ")";
    }
  }
  etmp += ".\n";
  QString idtemp = QString("Selected Room ID: %1").arg(id);
  emit log( "FindRooms", idtemp+"\n"+getName(r)+"\n"+getDescription(r)+getDynamicDescription(r)+etmp);
//  emit newRoomSelection(m_roomSelection);
}

void FindRoomsDlg::adjustResultTable()
{
  resultTable->setColumnCount(2);
  resultTable->setHeaderLabels(QStringList() << tr("Room ID") << tr("Room Name"));
  resultTable->header()->setResizeMode(1, QHeaderView::Stretch);
  resultTable->setRootIsDecorated(false);
  resultTable->setAlternatingRowColors(true);
}

void FindRoomsDlg::enableFindButton(const QString &text)
{
  findButton->setEnabled(!text.isEmpty());
}

void FindRoomsDlg::closeEvent( QCloseEvent * event )
{
  // Release data back to the m_admin
  if (m_admin != NULL) m_mapData->unselect(m_roomSelection);
  resultTable->clear();
  roomsFoundLabel->clear();
  m_admin = NULL;
  event->accept();
}

void FindRoomsDlg::on_lineEdit_textChanged()
{
  findButton->setEnabled(lineEdit->hasAcceptableInput());
}

QString& FindRoomsDlg::latinToAscii(QString& str) {
    // latin1 to 7-bit Ascii
    // taken from Pandora
  const unsigned char table[] = {
/*192*/   'A',
          'A',
          'A',
          'A',
          'A',
          'A',
          'A',
          'C',
          'E',
          'E',
          'E',
          'E',
          'I',
          'I',
          'I',
          'I',
          'D',
          'N',
          'O',
          'O',
          'O',
          'O',
          'O',
          'x',
          'O',
          'U',
          'U',
          'U',
          'U',
          'Y',
          'b',
          'B',
          'a',
          'a',
          'a',
          'a',
          'a',
          'a',
          'a',
          'c',
          'e',
          'e',
          'e',
          'e',
          'i',
          'i',
          'i',
          'i',
          'o',
          'n',
          'o',
          'o',
          'o',
          'o',
          'o',
          ':',
          'o',
          'u',
          'u',
          'u',
          'u',
          'y',
          'b',
          'y'
  };
  unsigned char ch;
  int pos;

  for (pos = 0; pos < str.length(); pos++) {
    ch = str.at(pos).toLatin1();
    if (ch > 128) {
      if (ch < 192)
        ch = 'z';
      else
        ch = table[ ch - 192 ];
      str[pos] = ch;
    }
  }
  return str;
}
