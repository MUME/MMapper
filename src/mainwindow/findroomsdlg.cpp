// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Kalev Lember <kalev@smartlink.ee> (Kalev)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "findroomsdlg.h"

#include "../configuration/configuration.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/Charset.h"
#include "../global/parserutils.h"
#include "../global/roomid.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/roomfilter.h"
#include "../mapdata/roomselection.h"

#include <cstdint>

#include <QString>
#include <QtGui>
#include <QtWidgets>

FindRoomsDlg::FindRoomsDlg(MapData &md, QWidget *const parent)
    : QDialog(parent)
    , m_mapData{md}
{
    setupUi(this);

    adjustResultTable();

    m_showSelectedRoom = new QShortcut(QKeySequence(tr("Space", "Select result item")), resultTable);
    m_showSelectedRoom->setContext(Qt::WidgetShortcut);

    selectButton->setEnabled(false);
    editButton->setEnabled(false);

    connect(lineEdit, &QLineEdit::textChanged, this, &FindRoomsDlg::slot_enableFindButton);
    connect(findButton, &QAbstractButton::clicked, this, &FindRoomsDlg::slot_findClicked);
    connect(closeButton, &QAbstractButton::clicked, this, &QWidget::close);
    connect(resultTable,
            &QTreeWidget::itemDoubleClicked,
            this,
            &FindRoomsDlg::slot_itemDoubleClicked);
    connect(m_showSelectedRoom, &QShortcut::activated, this, &FindRoomsDlg::slot_showSelectedRoom);
    connect(resultTable, &QTreeWidget::itemSelectionChanged, this, [this]() {
        const bool enabled = !resultTable->selectedItems().isEmpty();
        selectButton->setEnabled(enabled);
        editButton->setEnabled(enabled);
    });
    connect(selectButton, &QAbstractButton::clicked, this, [this]() {
        const auto tmpSel = RoomSelection::createSelection(m_mapData);
        for (const auto &selectedItem : resultTable->selectedItems()) {
            const auto id = RoomId{selectedItem->text(0).toUInt()};
            tmpSel->getRoom(id);
        }
        if (!tmpSel->empty()) {
            glm::vec2 sum{0.f, 0.f};
            // FIXME: This is actually an anti-feature if the rooms are far apart,
            // because it drops you off in the middle of nowhere.
            for (const auto &[rid, room] : *tmpSel) {
                sum += room->getPosition().to_vec2();
            }
            // note: half-room offset to the room center is applied to the average,
            // rather than to each individual room.
            const auto worldPos = sum / static_cast<float>(tmpSel->size()) + glm::vec2{0.5f, 0.5f};
            emit sig_center(worldPos);
        }
        emit sig_newRoomSelection(SigRoomSelection{tmpSel});
    });
    connect(editButton, &QAbstractButton::clicked, this, [this]() {
        const auto tmpSel = RoomSelection::createSelection(m_mapData);
        for (const auto &selectedItem : resultTable->selectedItems()) {
            const auto id = RoomId{selectedItem->text(0).toUInt()};
            tmpSel->getRoom(id);
        }
        emit sig_newRoomSelection(SigRoomSelection{tmpSel});
        emit sig_editSelection();
    });

    setFocus();
    label->setFocusProxy(lineEdit);
    lineEdit->setFocus();

    readSettings();
}

FindRoomsDlg::~FindRoomsDlg()
{
    delete m_showSelectedRoom;
    resultTable->clear();
}

void FindRoomsDlg::readSettings()
{
    restoreGeometry(getConfig().findRoomsDialog.geometry);
}

void FindRoomsDlg::writeSettings()
{
    setConfig().findRoomsDialog.geometry = saveGeometry();
}

void FindRoomsDlg::slot_findClicked()
{
    const Qt::CaseSensitivity cs = caseCheckBox->isChecked() ? Qt::CaseSensitive
                                                             : Qt::CaseInsensitive;
    const bool regex = regexCheckBox->isChecked();
    std::string text = lineEdit->text().toLatin1().toStdString();
    // remove latin1
    text = ::latin1ToAsciiInPlace(text);
    resultTable->clear();
    roomsFoundLabel->clear();

    auto kind = PatternKindsEnum::ALL;
    if (nameRadioButton->isChecked()) {
        kind = PatternKindsEnum::NAME;
    } else if (descRadioButton->isChecked()) {
        kind = PatternKindsEnum::DESC;
    } else if (contentsRadioButton->isChecked()) {
        kind = PatternKindsEnum::CONTENTS;
    } else if (exitsRadioButton->isChecked()) {
        kind = PatternKindsEnum::EXITS;
    } else if (notesRadioButton->isChecked()) {
        kind = PatternKindsEnum::NOTE;
    } else if (flagsRadioButton->isChecked()) {
        kind = PatternKindsEnum::FLAGS;
    }

    try {
        auto tmpSel = RoomSelection(m_mapData);
        tmpSel.genericSearch(RoomFilter(text, cs, regex, kind));

        for (const auto &[rid, room] : tmpSel) {
            QString id;
            id.setNum(rid.asUint32());
            QString roomName = room->getName().toQString();
            QString toolTip = slot_constructToolTip(room);

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
                              QString::asprintf("An exception occurred: %s\n", ex.what()));
    }
    roomsFoundLabel->setText(tr("%1 room%2 found")
                                 .arg(resultTable->topLevelItemCount())
                                 .arg((resultTable->topLevelItemCount() == 1) ? "" : "s"));
}

// FIXME: This code is almost identical to the code in MapCanvas::mouseReleaseEvent. Refactor!
QString FindRoomsDlg::slot_constructToolTip(const Room *const r)
{
    return QString("Selected Room ID: %1\n%2").arg(r->getId().asUint32()).arg(r->toQString());
}

void FindRoomsDlg::slot_showSelectedRoom()
{
    slot_itemDoubleClicked(resultTable->currentItem());
}

void FindRoomsDlg::slot_itemDoubleClicked(QTreeWidgetItem *const inputItem)
{
    if (inputItem == nullptr) {
        return;
    }

    auto tmpSel = RoomSelection(m_mapData);
    const auto id = RoomId{inputItem->text(0).toUInt()};
    if (const Room *const r = tmpSel.getRoom(id)) {
        if (r->getId() == id) {
            const Coordinate &c = r->getPosition();
            const auto worldPos = c.to_vec2() + glm::vec2{0.5f, 0.5f};
            emit sig_center(worldPos); // connects to MapWindow
        }
        emit sig_log("FindRooms", inputItem->toolTip(0));
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

void FindRoomsDlg::slot_enableFindButton(const QString &text)
{
    findButton->setEnabled(!text.isEmpty());
}

void FindRoomsDlg::closeEvent(QCloseEvent *event)
{
    writeSettings();
    resultTable->clear();
    roomsFoundLabel->clear();
    lineEdit->setFocus();
    selectButton->setEnabled(false);
    editButton->setEnabled(false);
    event->accept();
}

void FindRoomsDlg::slot_on_lineEdit_textChanged()
{
    findButton->setEnabled(lineEdit->hasAcceptableInput());
}
