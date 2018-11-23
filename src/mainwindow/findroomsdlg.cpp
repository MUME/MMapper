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

#include <cstdint>
#include <QString>
#include <QtGui>
#include <QtWidgets>

#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/roomfilter.h"
#include "../mapdata/roomselection.h"
#include "../parser/parserutils.h"

const QString FindRoomsDlg::nullString;

FindRoomsDlg::FindRoomsDlg(MapData *md, QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    m_mapData = md;
    adjustResultTable();

    m_showSelectedRoom = new QShortcut(QKeySequence(tr("Space", "Select result item")), resultTable);
    m_showSelectedRoom->setContext(Qt::WidgetShortcut);

    selectButton->setEnabled(false);
    editButton->setEnabled(false);

    connect(lineEdit, &QLineEdit::textChanged, this, &FindRoomsDlg::enableFindButton);
    connect(findButton, &QAbstractButton::clicked, this, &FindRoomsDlg::findClicked);
    connect(closeButton, &QAbstractButton::clicked, this, &QWidget::close);
    connect(resultTable, &QTreeWidget::itemDoubleClicked, this, &FindRoomsDlg::itemDoubleClicked);
    connect(m_showSelectedRoom, &QShortcut::activated, this, &FindRoomsDlg::showSelectedRoom);
    connect(resultTable, &QTreeWidget::itemSelectionChanged, this, [this]() {
        const bool enabled = !resultTable->selectedItems().isEmpty();
        selectButton->setEnabled(enabled);
        editButton->setEnabled(enabled);
    });
    connect(selectButton, &QAbstractButton::clicked, this, [this]() {
        const RoomSelection *tmpSel = m_mapData->select();
        for (const auto &item : resultTable->selectedItems()) {
            const auto id = RoomId{item->text(0).toUInt()};
            m_mapData->getRoom(id, tmpSel);
        }
        if (!tmpSel->isEmpty()) {
            int64_t avgX = 0;
            int64_t avgY = 0;
            for (const Room *const r : tmpSel->values()) {
                const Coordinate &c = r->getPosition();
                avgX += c.x;
                avgY += c.y;
            }
            avgX = avgX / tmpSel->size();
            avgY = avgY / tmpSel->size();
            emit center(static_cast<qint32>(avgX), static_cast<qint32>(avgY));
        }
        emit newRoomSelection(tmpSel);
    });
    connect(editButton, &QAbstractButton::clicked, this, [this]() {
        const RoomSelection *tmpSel = m_mapData->select();
        for (const auto &item : resultTable->selectedItems()) {
            const auto id = RoomId{item->text(0).toUInt()};
            m_mapData->getRoom(id, tmpSel);
        }
        emit newRoomSelection(tmpSel);
        emit editSelection();
    });

    setFocus();
    label->setFocusProxy(lineEdit);
    lineEdit->setFocus();
}

FindRoomsDlg::~FindRoomsDlg()
{
    delete m_showSelectedRoom;
    resultTable->clear();
}

void FindRoomsDlg::findClicked()
{
    const RoomSelection *tmpSel = m_mapData->select();
    Qt::CaseSensitivity cs = caseCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QString text = lineEdit->text();
    // remove latin1
    text = ParserUtils::latinToAsciiInPlace(text);
    resultTable->clear();
    roomsFoundLabel->clear();

    /*  for an absolute match do the below:
    m_mapData->lookingForRooms(this, createEvent(CommandIdType::UNKNOWN, text, nullString, nullString, 0, 0));
    */

    auto kind = pattern_kinds::ALL;
    if (nameRadioButton->isChecked()) {
        kind = pattern_kinds::NAME;
    } else if (descRadioButton->isChecked()) {
        kind = pattern_kinds::DESC;
    } else if (dynDescRadioButton->isChecked()) {
        kind = pattern_kinds::DYN_DESC;
    } else if (exitsRadioButton->isChecked()) {
        kind = pattern_kinds::EXITS;
    } else if (notesRadioButton->isChecked()) {
        kind = pattern_kinds::NOTE;
    }
    m_mapData->genericSearch(tmpSel, RoomFilter(text, cs, kind));

    for (const Room *const room : tmpSel->values()) {
        QString id;
        id.setNum(room->getId().asUint32());
        QString roomName = room->getName();
        QString toolTip = constructToolTip(room);

        item = new QTreeWidgetItem(resultTable);
        item->setText(0, id);
        item->setText(1, roomName);
        item->setToolTip(0, toolTip);
        item->setToolTip(1, toolTip);
    }
    roomsFoundLabel->setText(tr("%1 room%2 found")
                                 .arg(resultTable->topLevelItemCount())
                                 .arg((resultTable->topLevelItemCount() == 1) ? "" : "s"));
    m_mapData->unselect(tmpSel);
}

QString FindRoomsDlg::constructToolTip(const Room *r)
{
    // taken from MapCanvas:
    QString etmp = "Exits:";
    for (const auto j : ALL_EXITS7) {
        bool door = false;
        if (r->exit(j).isDoor()) {
            door = true;
            etmp += " (";
        }

        if (r->exit(j).isExit()) {
            if (!door) {
                etmp += " ";
            }

            etmp += lowercaseDirection(j);
        }
        if (door) {
            const QString doorName = r->exit(j).getDoorName();
            if (!doorName.isEmpty()) {
                etmp += "/" + doorName;
            }
            etmp += ")";
        }
    }
    etmp += ".\n";

    QString note = r->getNote();
    if (!note.isEmpty()) {
        note = "Note: " + note;
    }

    return QString("Selected Room ID: %1").arg(r->getId().asUint32()) + "\n" + r->getName() + "\n"
           + r->getStaticDescription() + r->getDynamicDescription() + etmp + note;
}

void FindRoomsDlg::showSelectedRoom()
{
    itemDoubleClicked(resultTable->currentItem());
}

void FindRoomsDlg::itemDoubleClicked(QTreeWidgetItem *const inputItem)
{
    if (inputItem == nullptr) {
        return;
    }

    const RoomSelection *tmpSel = m_mapData->select();
    const auto id = RoomId{inputItem->text(0).toUInt()};
    m_mapData->getRoom(id, tmpSel);
    if (!tmpSel->isEmpty()) {
        for (const Room *const r : tmpSel->values()) {
            if (r && r->getId() == id) {
                const Coordinate &c = r->getPosition();
                emit center(c.x, c.y); // connects to MapWindow
            }
        }
    }
    m_mapData->unselect(tmpSel);
    emit log("FindRooms", inputItem->toolTip(0));
}

void FindRoomsDlg::adjustResultTable()
{
    resultTable->setColumnCount(2);
    resultTable->setHeaderLabels(QStringList() << tr("Room ID") << tr("Room Name"));
    resultTable->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    resultTable->setRootIsDecorated(false);
    resultTable->setAlternatingRowColors(true);
    resultTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    resultTable->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
}

void FindRoomsDlg::enableFindButton(const QString &text)
{
    findButton->setEnabled(!text.isEmpty());
}

void FindRoomsDlg::closeEvent(QCloseEvent *event)
{
    resultTable->clear();
    roomsFoundLabel->clear();
    lineEdit->setFocus();
    selectButton->setEnabled(false);
    editButton->setEnabled(false);
    event->accept();
}

void FindRoomsDlg::on_lineEdit_textChanged()
{
    findButton->setEnabled(lineEdit->hasAcceptableInput());
}
