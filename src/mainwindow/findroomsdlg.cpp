// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Kalev Lember <kalev@smartlink.ee> (Kalev)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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

FindRoomsDlg::FindRoomsDlg(MapData *const md, QWidget *const parent)
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
        const auto tmpSel = RoomSelection::createSelection(*m_mapData);
        for (const auto &selectedItem : resultTable->selectedItems()) {
            const auto id = RoomId{selectedItem->text(0).toUInt()};
            tmpSel->getRoom(id);
        }
        if (!tmpSel->isEmpty()) {
            int64_t avgX = 0;
            int64_t avgY = 0;
            for (const Room *const r : *tmpSel) {
                const Coordinate &c = r->getPosition();
                avgX += c.x;
                avgY += c.y;
            }
            avgX = avgX / tmpSel->size();
            avgY = avgY / tmpSel->size();
            emit center(static_cast<qint32>(avgX), static_cast<qint32>(avgY));
        }
        emit newRoomSelection(SigRoomSelection{tmpSel});
    });
    connect(editButton, &QAbstractButton::clicked, this, [this]() {
        const auto tmpSel = RoomSelection::createSelection(*m_mapData);
        for (const auto &selectedItem : resultTable->selectedItems()) {
            const auto id = RoomId{selectedItem->text(0).toUInt()};
            tmpSel->getRoom(id);
        }
        emit newRoomSelection(SigRoomSelection{tmpSel});
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
    const Qt::CaseSensitivity cs = caseCheckBox->isChecked() ? Qt::CaseSensitive
                                                             : Qt::CaseInsensitive;
    QString text = lineEdit->text();
    // remove latin1
    text = ParserUtils::latinToAsciiInPlace(text);
    resultTable->clear();
    roomsFoundLabel->clear();

    /*  for an absolute match do the below:
    m_mapData->lookingForRooms(this, createEvent(CommandEnum::UNKNOWN, text, nullString, nullString, 0, 0));
    */

    auto kind = PatternKindsEnum::ALL;
    if (nameRadioButton->isChecked()) {
        kind = PatternKindsEnum::NAME;
    } else if (descRadioButton->isChecked()) {
        kind = PatternKindsEnum::DESC;
    } else if (dynDescRadioButton->isChecked()) {
        kind = PatternKindsEnum::DYN_DESC;
    } else if (exitsRadioButton->isChecked()) {
        kind = PatternKindsEnum::EXITS;
    } else if (notesRadioButton->isChecked()) {
        kind = PatternKindsEnum::NOTE;
    } else if (flagsRadioButton->isChecked()) {
        kind = PatternKindsEnum::FLAGS;
    }

    try {
        auto tmpSel = RoomSelection(*m_mapData);
        tmpSel.genericSearch(RoomFilter(text, cs, kind));

        for (const Room *const room : tmpSel) {
            QString id;
            id.setNum(room->getId().asUint32());
            QString roomName = room->getName().toQString();
            QString toolTip = constructToolTip(room);

            item = new QTreeWidgetItem(resultTable);
            item->setText(0, id);
            item->setText(1, roomName);
            item->setToolTip(0, toolTip);
            item->setToolTip(1, toolTip);
        }
    } catch (const std::exception &ex) {
        qWarning() << "Exception: " << ex.what();
        QMessageBox::critical(this,
                              "Internal Error",
                              QString::asprintf("An exception occurred: %s\r\n", ex.what()));
    }
    roomsFoundLabel->setText(tr("%1 room%2 found")
                                 .arg(resultTable->topLevelItemCount())
                                 .arg((resultTable->topLevelItemCount() == 1) ? "" : "s"));
}

// FIXME: This code is almost identical to the code in MapCanvas::mouseReleaseEvent. Refactor!
QString FindRoomsDlg::constructToolTip(const Room *const r)
{
    return QString("Selected Room ID: %1\n%2").arg(r->getId().asUint32()).arg(r->toQString());
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

    auto tmpSel = RoomSelection(*m_mapData);
    const auto id = RoomId{inputItem->text(0).toUInt()};
    if (const Room *const r = tmpSel.getRoom(id)) {
        if (r->getId() == id) {
            const Coordinate &c = r->getPosition();
            emit center(c.x, c.y); // connects to MapWindow
        }
        emit log("FindRooms", inputItem->toolTip(0));
    }
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
