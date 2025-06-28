// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Kalev Lember <kalev@smartlink.ee> (Kalev)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "findroomsdlg.h"

#include "../configuration/configuration.h"
#include "../global/parserutils.h"
#include "../map/ExitDirection.h"
#include "../map/coordinate.h"
#include "../map/roomid.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/roomfilter.h"
#include "../mapdata/roomselection.h"

#include <memory>

#include <QString>
#include <QtGui>
#include <QtWidgets>

FindRoomsDlg::FindRoomsDlg(MapData &md, QWidget *const parent)
    : QDialog(parent)
    , m_mapData{md}
{
    setupUi(this);

    adjustResultTable();

    m_showSelectedRoom = std::make_unique<QShortcut>(QKeySequence(tr("Space", "Select result item")),
                                                     resultTable);
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
    connect(m_showSelectedRoom.get(),
            &QShortcut::activated,
            this,
            &FindRoomsDlg::slot_showSelectedRoom);
    connect(resultTable, &QTreeWidget::itemSelectionChanged, this, [this]() {
        const bool enabled = !resultTable->selectedItems().isEmpty();
        selectButton->setEnabled(enabled);
        editButton->setEnabled(enabled);
    });
    connect(selectButton, &QAbstractButton::clicked, this, [this]() {
        auto map = m_mapData.getCurrentMap();
        RoomIdSet tmpSet;
        glm::vec2 sum{0.f, 0.f};
        for (const auto &selectedItem : resultTable->selectedItems()) {
            const auto id = ExternalRoomId{selectedItem->text(0).toUInt()};
            if (const auto room = map.findRoomHandle(id)) {
                sum += room.getPosition().to_vec2();
                tmpSet.insert(room.getId());
            }
        }

        if (!tmpSet.empty()) {
            // note: half-room offset to the room center
            const glm::vec2 offset{0.5f, 0.5f};
            const auto worldPos = sum / static_cast<float>(tmpSet.size()) + offset;
            emit sig_center(worldPos);
        }

        auto shared = RoomSelection::createSelection(std::move(tmpSet));
        emit sig_newRoomSelection(SigRoomSelection{std::move(shared)});
    });
    connect(editButton, &QAbstractButton::clicked, this, [this]() {
        const auto &map = m_mapData;
        const auto &currentMap = map.getCurrentMap();
        RoomIdSet set;
        for (const auto &selectedItem : resultTable->selectedItems()) {
            const auto extid = ExternalRoomId{selectedItem->text(0).toUInt()};
            if (const auto room = currentMap.findRoomHandle(extid)) {
                set.insert(room.getId());
            }
        }
        auto tmpSel = RoomSelection::createSelection(std::move(set));
        emit sig_newRoomSelection(SigRoomSelection{std::move(tmpSel)});
        emit sig_editSelection();
    });

    setFocus();
    label->setFocusProxy(lineEdit);
    lineEdit->setFocus();

    readSettings();
}

FindRoomsDlg::~FindRoomsDlg()
{
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

    const std::string text = mmqt::toStdStringUtf8(lineEdit->text());

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
    } else if (areaRadioButton->isChecked()) {
        kind = PatternKindsEnum::AREA;
    }

    try {
        RoomFilter filter(text, cs, regex, kind);
        const Map &map = m_mapData.getCurrentMap();
        map.getRooms().for_each([&](const auto roomId) {
            const auto &room = map.getRoomHandle(roomId);
            if (!filter.filter(room.getRaw())) {
                return;
            }

            QString id = QString("%1").arg(room.getIdExternal().asUint32());
            QString roomName = room.getName().toQString();
            QString toolTip = constructToolTip(room);

            item = new QTreeWidgetItem(resultTable);
            item->setText(0, id);
            item->setText(1, roomName);
            item->setToolTip(0, toolTip);
            item->setToolTip(1, toolTip);
        });
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

QString FindRoomsDlg::constructToolTip(const RoomHandle &room)
{
    // Tooltip doesn't support ANSI, and there's no way to add formatted text.
    return mmqt::previewRoom(room, mmqt::StripAnsiEnum::Yes, mmqt::PreviewStyleEnum::ForDisplay);
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

    const auto &map = m_mapData.getCurrentMap();
    const auto extId = ExternalRoomId{inputItem->text(0).toUInt()};
    if (const auto r = map.findRoomHandle(extId)) {
        // REVISIT: should this block be a stand-alone function?
        {
            assert(r.getIdExternal() == extId);
            const Coordinate &c = r.getPosition();
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
