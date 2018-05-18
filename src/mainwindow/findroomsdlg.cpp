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
#include "mapdata.h"
#include "mmapper2exit.h"
#include "mmapper2room.h"
#include "parserutils.h"
#include "roomselection.h"

#include <QCloseEvent>
#include <QShortcut>

const QString FindRoomsDlg::nullString;

FindRoomsDlg::FindRoomsDlg(MapData *md, QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    m_roomSelection = nullptr;
    m_mapData = md;
    adjustResultTable();

    m_showSelectedRoom = new QShortcut( QKeySequence( tr( "Space", "Select result item" ) ),
                                        resultTable );
    m_showSelectedRoom->setContext( Qt::WidgetShortcut );

    connect(lineEdit, SIGNAL(textChanged(const QString &)), this,
            SLOT(enableFindButton(const QString &)));
    connect(findButton, SIGNAL(clicked()), this, SLOT(findClicked()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(resultTable, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this,
            SLOT(itemDoubleClicked(QTreeWidgetItem *)));
    connect(m_showSelectedRoom, SIGNAL(activated()), this, SLOT(showSelectedRoom()));
}

FindRoomsDlg::~FindRoomsDlg()
{
    if (m_roomSelection != nullptr) {
        m_mapData->unselect(m_roomSelection);
        m_roomSelection = nullptr;
    }
    delete m_showSelectedRoom;
    resultTable->clear();
}

void FindRoomsDlg::findClicked()
{
    // Release rooms for a new search
    if (m_roomSelection != nullptr) {
        m_mapData->unselect(m_roomSelection);
        m_roomSelection = nullptr;
    }

    m_roomSelection = m_mapData->select();
    Qt::CaseSensitivity cs = caseCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QString text = lineEdit->text();
    // remove latin1
    text = ParserUtils::latinToAscii(text);
    resultTable->clear();
    roomsFoundLabel->clear();

    /*  for an absolute match do the below:
    m_mapData->lookingForRooms(this, createEvent(CID_UNKNOWN, text, nullString, nullString, 0, 0));
    */

    char kind = PAT_ALL;
    if (nameRadioButton->isChecked()) {
        kind = PAT_NAME;
    } else if (descRadioButton->isChecked()) {
        kind = PAT_DESC;
    } else if (dynDescRadioButton->isChecked()) {
        kind = PAT_DYNDESC;
    } else if (exitsRadioButton->isChecked()) {
        kind = PAT_EXITS;
    } else if (notesRadioButton->isChecked()) {
        kind = PAT_NOTE;
    }
    m_mapData->genericSearch(m_roomSelection, RoomFilter(text, cs, kind));

    for (const Room *room : m_roomSelection->values()) {
//#define HACK_FIX_THE_HIDDEN_NAMELESS_EXITS
#ifdef HACK_FIX_THE_HIDDEN_NAMELESS_EXITS
        // Remember to also alter the way searching works to match the exits you want
        // to fix
        ExitsList exits = room->getExitsList();
        for (uint dir = 0; dir < exits.size(); ++dir) {
            const Exit &e = room->exit( dir );
            bool isSecret = ISSET( Mmapper2Exit::getFlags( e ), EF_DOOR )
                            && ISSET( Mmapper2Exit::getDoorFlags( e ), DF_HIDDEN );
            if (isSecret && QString((e)[0].toString()).isEmpty()) {
                // I can't get MapActions to work here
                nandDoorFlags( const_cast<Room *>( room )->exit( dir ), DF_HIDDEN );
            }
        }
#endif

        QString id;
        id.setNum(room->getId());
        QString roomName = Mmapper2Room::getName(room);
        QString toolTip = constructToolTip(room);

        item = new QTreeWidgetItem(resultTable);
        item->setText(0, id);
        item->setText(1, roomName);
        item->setToolTip(0, toolTip);
        item->setToolTip(1, toolTip);
    }
    roomsFoundLabel->setText(tr("%1 room%2 found")
                             .arg( resultTable->topLevelItemCount() )
                             .arg( (resultTable->topLevelItemCount() == 1) ? "" : "s" ));
}

QString FindRoomsDlg::constructToolTip(const Room *r)
{
    // taken from MapCanvas:
    QString etmp = "Exits:";
    for (int j = 0; j < 7; j++) {

        bool door = false;
        if (ISSET(Mmapper2Exit::getFlags(r->exit(j)), EF_DOOR)) {
            door = true;
            etmp += " (";
        }

        if (ISSET(Mmapper2Exit::getFlags(r->exit(j)), EF_EXIT)) {
            if (!door) {
                etmp += " ";
            }

            switch (j) {
            case 0:
                etmp += "north";
                break;
            case 1:
                etmp += "south";
                break;
            case 2:
                etmp += "east";
                break;
            case 3:
                etmp += "west";
                break;
            case 4:
                etmp += "up";
                break;
            case 5:
                etmp += "down";
                break;
            case 6:
                etmp += "unknown";
                break;
            }
        }

        if (door) {
            QString doorName = Mmapper2Exit::getDoorName(r->exit(j));
            if (!doorName.isEmpty()) {
                etmp += "/" + doorName + ")";
            } else {
                etmp += ")";
            }
        }
    }
    etmp += ".\n";

    QString note = Mmapper2Room::getNote(r);
    if (!note.isEmpty()) {
        note = "Note: " + note;
    }

    return QString("Selected Room ID: %1").arg(r->getId()) + "\n" + Mmapper2Room::getName(r) + "\n" +
           Mmapper2Room::getDescription(r) + Mmapper2Room::getDynamicDescription(r) + etmp + note;
}

void FindRoomsDlg::showSelectedRoom()
{
    itemDoubleClicked( resultTable->currentItem() );
}

void FindRoomsDlg::itemDoubleClicked(QTreeWidgetItem *item)
{
    uint id;
    if (item == nullptr) {
        return;
    }

    id = item->text(0).toInt();

    const Room *r = m_mapData->getRoom(id, m_roomSelection);
    Coordinate c = r->getPosition();
    emit center(c.x, c.y); // connects to MapWindow

    emit log( "FindRooms", item->toolTip(0));
//  emit newRoomSelection(m_roomSelection);
}

void FindRoomsDlg::adjustResultTable()
{
    resultTable->setColumnCount(2);
    resultTable->setHeaderLabels(QStringList() << tr("Room ID") << tr("Room Name"));
    resultTable->header()->setSectionResizeMode(QHeaderView::Stretch);
    resultTable->setRootIsDecorated(false);
    resultTable->setAlternatingRowColors(true);
}

void FindRoomsDlg::enableFindButton(const QString &text)
{
    findButton->setEnabled(!text.isEmpty());
}

void FindRoomsDlg::closeEvent( QCloseEvent *event )
{
    // Release room locks
    if (m_roomSelection != nullptr) {
        m_mapData->unselect(m_roomSelection);
        m_roomSelection = nullptr;
    }
    resultTable->clear();
    roomsFoundLabel->clear();
    event->accept();
}

void FindRoomsDlg::on_lineEdit_textChanged()
{
    findButton->setEnabled(lineEdit->hasAcceptableInput());
}
