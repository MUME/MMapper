/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
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

#include "roomeditattrdlg.h"
#include "customaction.h"
#include "defs.h"
#include "mapcanvas.h"
#include "mapdata.h"
#include "mmapper2room.h"
#include "room.h"
#include "roomselection.h"

#include <QSettings>
#include <QShortcut>

#include <cmath>

RoomEditAttrDlg::RoomEditAttrDlg(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    roomDescriptionTextEdit->setLineWrapMode(QTextEdit::NoWrap);

    mobListItems[0] = new QListWidgetItem("Rent place");
    mobListItems[1] = new QListWidgetItem("Generic shop");
    mobListItems[2] = new QListWidgetItem("Weapon shop");
    mobListItems[3] = new QListWidgetItem("Armour shop");
    mobListItems[4] = new QListWidgetItem("Food shop");
    mobListItems[5] = new QListWidgetItem("Pet shop");
    mobListItems[6] = new QListWidgetItem("Generic guild");
    mobListItems[7] = new QListWidgetItem("Scout guild");
    mobListItems[8] = new QListWidgetItem("Mage guild");
    mobListItems[9] = new QListWidgetItem("Cleric guild");
    mobListItems[10] = new QListWidgetItem("Warrior guild");
    mobListItems[11] = new QListWidgetItem("Ranger guild");
    mobListItems[12] = new QListWidgetItem("SMOB");
    mobListItems[13] = new QListWidgetItem("Quest mob");
    mobListItems[14] = new QListWidgetItem("Any mob");
    mobListItems[15] = nullptr;
    mobListItems[16] = nullptr;
    mobListItems[17] = nullptr;
    mobListItems[18] = nullptr;
    mobListItems[19] = nullptr;
    mobListItems[20] = nullptr;
    mobListItems[21] = nullptr;
    mobListItems[22] = nullptr;
    mobListItems[23] = nullptr;
    mobListItems[24] = nullptr;
    mobListItems[25] = nullptr;
    mobListItems[26] = nullptr;
    mobListItems[27] = nullptr;
    mobListItems[28] = nullptr;
    mobListItems[29] = nullptr;
    mobListItems[30] = nullptr;
    mobListItems[31] = nullptr;
    mobFlagsListWidget->clear();
    for (int i = 0; i < 15; i++) {
        mobListItems[i]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsTristate);
        mobFlagsListWidget->addItem(mobListItems[i]);
    }

    loadListItems[0] = new QListWidgetItem("Treasure");
    loadListItems[1] = new QListWidgetItem("Equipment");
    loadListItems[2] = new QListWidgetItem("Weapon");
    loadListItems[3] = new QListWidgetItem("Water");
    loadListItems[4] = new QListWidgetItem("Food");
    loadListItems[5] = new QListWidgetItem("Herb");
    loadListItems[6] = new QListWidgetItem("Key");
    loadListItems[7] = new QListWidgetItem("Mule");
    loadListItems[8] = new QListWidgetItem("Horse");
    loadListItems[9] = new QListWidgetItem("Pack horse");
    loadListItems[10] = new QListWidgetItem("Trained horse");
    loadListItems[11] = new QListWidgetItem("Rohirrim");
    loadListItems[12] = new QListWidgetItem("Warg");
    loadListItems[13] = new QListWidgetItem("Boat");
    loadListItems[14] = new QListWidgetItem("Attention");
    loadListItems[15] = new QListWidgetItem("Tower");
    loadListItems[16] = new QListWidgetItem("Clock");
    loadListItems[17] = new QListWidgetItem("Mail");
    loadListItems[18] = new QListWidgetItem("Stable");
    loadListItems[19] = nullptr;
    loadListItems[20] = nullptr;
    loadListItems[21] = nullptr;
    loadListItems[22] = nullptr;
    loadListItems[23] = nullptr;
    loadListItems[24] = nullptr;
    loadListItems[25] = nullptr;
    loadListItems[26] = nullptr;
    loadListItems[27] = nullptr;
    loadListItems[28] = nullptr;
    loadListItems[29] = nullptr;
    loadListItems[30] = nullptr;
    loadListItems[31] = nullptr;
    loadFlagsListWidget->clear();
    for (int i = 0; i < 19; i++) {
        loadListItems[i]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsTristate);
        loadFlagsListWidget->addItem(loadListItems[i]);
    }

    exitListItems[0] = new QListWidgetItem("Exit");
    exitListItems[1] = new QListWidgetItem("Door");
    exitListItems[2] = new QListWidgetItem("Road");
    exitListItems[3] = new QListWidgetItem("Climb");
    exitListItems[4] = new QListWidgetItem("Random");
    exitListItems[5] = new QListWidgetItem("Special");
    exitListItems[6] = new QListWidgetItem("No match");
    exitListItems[7] = new QListWidgetItem("Water flow");
    exitListItems[8] = new QListWidgetItem("No flee");
    exitListItems[9] = new QListWidgetItem("Damage");
    exitListItems[10] = new QListWidgetItem("Fall");
    exitListItems[11] = new QListWidgetItem("Guarded");
    exitListItems[12] = nullptr;
    exitListItems[13] = nullptr;
    exitListItems[14] = nullptr;
    exitListItems[15] = nullptr;
    exitFlagsListWidget->clear();
    for (int i = 0; i < 12; i++) {
        exitListItems[i]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        exitFlagsListWidget->addItem(exitListItems[i]);
    }

    doorListItems[0] = new QListWidgetItem("Hidden");
    doorListItems[1] = new QListWidgetItem("Need key");
    doorListItems[2] = new QListWidgetItem("No block");
    doorListItems[3] = new QListWidgetItem("No break");
    doorListItems[4] = new QListWidgetItem("No pick");
    doorListItems[5] = new QListWidgetItem("Delayed");
    doorListItems[6] = new QListWidgetItem("Callable");
    doorListItems[7] = new QListWidgetItem("Knockable");
    doorListItems[8] = new QListWidgetItem("Magic");
    doorListItems[9] = new QListWidgetItem("Action-controlled");
    doorListItems[10] = nullptr;
    doorListItems[11] = nullptr;
    doorListItems[12] = nullptr;
    doorListItems[13] = nullptr;
    doorListItems[14] = nullptr;
    doorListItems[15] = nullptr;

    doorFlagsListWidget->clear();
    for (int i = 0; i < 10; i++) {
        doorListItems[i]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        doorFlagsListWidget->addItem(doorListItems[i]);
    }

    m_hiddenShortcut = new QShortcut(QKeySequence(tr("Ctrl+H", "Room edit > hidden flag")), this);

    updatedLabel->setText("Room has not been online updated yet!!!");

    readSettings();
}

RoomEditAttrDlg::~RoomEditAttrDlg()
{
    writeSettings();
}

void RoomEditAttrDlg::readSettings()
{
    QSettings settings("Caligor soft", "MMapper2");
    settings.beginGroup("RoomEditAttrDlg");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    settings.endGroup();
    move(pos);
}

void RoomEditAttrDlg::writeSettings()
{
    QSettings settings("Caligor soft", "MMapper2");
    settings.beginGroup("RoomEditAttrDlg");
    settings.setValue("pos", pos());
    settings.endGroup();
}

void RoomEditAttrDlg::connectAll()
{
    connect(neutralRadioButton, SIGNAL(toggled(bool)), this, SLOT(neutralRadioButtonToggled(bool)));
    connect(goodRadioButton, SIGNAL(toggled(bool)), this, SLOT(goodRadioButtonToggled(bool)));
    connect(evilRadioButton, SIGNAL(toggled(bool)), this, SLOT(evilRadioButtonToggled(bool)));
    connect(alignUndefRadioButton,
            SIGNAL(toggled(bool)),
            this,
            SLOT(alignUndefRadioButtonToggled(bool)));

    connect(noPortRadioButton, SIGNAL(toggled(bool)), this, SLOT(noPortRadioButtonToggled(bool)));
    connect(portableRadioButton,
            SIGNAL(toggled(bool)),
            this,
            SLOT(portableRadioButtonToggled(bool)));
    connect(portUndefRadioButton,
            SIGNAL(toggled(bool)),
            this,
            SLOT(portUndefRadioButtonToggled(bool)));

    connect(noRideRadioButton, SIGNAL(toggled(bool)), this, SLOT(noRideRadioButtonToggled(bool)));
    connect(ridableRadioButton, SIGNAL(toggled(bool)), this, SLOT(ridableRadioButtonToggled(bool)));
    connect(rideUndefRadioButton,
            SIGNAL(toggled(bool)),
            this,
            SLOT(rideUndefRadioButtonToggled(bool)));

    connect(litRadioButton, SIGNAL(toggled(bool)), this, SLOT(litRadioButtonToggled(bool)));
    connect(darkRadioButton, SIGNAL(toggled(bool)), this, SLOT(darkRadioButtonToggled(bool)));
    connect(lightUndefRadioButton,
            SIGNAL(toggled(bool)),
            this,
            SLOT(lightUndefRadioButtonToggled(bool)));

    connect(sundeathRadioButton,
            SIGNAL(toggled(bool)),
            this,
            SLOT(sundeathRadioButtonToggled(bool)));
    connect(noSundeathRadioButton,
            SIGNAL(toggled(bool)),
            this,
            SLOT(noSundeathRadioButtonToggled(bool)));
    connect(sundeathUndefRadioButton,
            SIGNAL(toggled(bool)),
            this,
            SLOT(sundeathUndefRadioButtonToggled(bool)));

    connect(mobFlagsListWidget,
            SIGNAL(itemChanged(QListWidgetItem *)),
            this,
            SLOT(mobFlagsListItemChanged(QListWidgetItem *)));
    connect(loadFlagsListWidget,
            SIGNAL(itemChanged(QListWidgetItem *)),
            this,
            SLOT(loadFlagsListItemChanged(QListWidgetItem *)));

    connect(exitNButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));
    connect(exitSButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));
    connect(exitEButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));
    connect(exitWButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));
    connect(exitUButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));
    connect(exitDButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));

    connect(exitFlagsListWidget,
            SIGNAL(itemChanged(QListWidgetItem *)),
            this,
            SLOT(exitFlagsListItemChanged(QListWidgetItem *)));
    connect(doorFlagsListWidget,
            SIGNAL(itemChanged(QListWidgetItem *)),
            this,
            SLOT(doorFlagsListItemChanged(QListWidgetItem *)));

    connect(doorNameLineEdit,
            SIGNAL(textChanged(QString)),
            this,
            SLOT(doorNameLineEditTextChanged(QString)));

    connect(toolButton00, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    connect(toolButton01, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    connect(toolButton02, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    connect(toolButton03, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    connect(toolButton04, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    connect(toolButton05, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    connect(toolButton06, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    connect(toolButton07, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    connect(toolButton08, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    connect(toolButton09, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    connect(toolButton10, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    connect(toolButton11, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    connect(toolButton12, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    connect(toolButton13, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    connect(toolButton14, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    connect(toolButton15, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));

    connect(roomNoteTextEdit, SIGNAL(textChanged()), this, SLOT(roomNoteChanged()));

    connect(m_hiddenShortcut, SIGNAL(activated()), this, SLOT(toggleHiddenDoor()));
}

void RoomEditAttrDlg::disconnectAll()
{
    disconnect(neutralRadioButton,
               SIGNAL(toggled(bool)),
               this,
               SLOT(neutralRadioButtonToggled(bool)));
    disconnect(goodRadioButton, SIGNAL(toggled(bool)), this, SLOT(goodRadioButtonToggled(bool)));
    disconnect(evilRadioButton, SIGNAL(toggled(bool)), this, SLOT(evilRadioButtonToggled(bool)));
    disconnect(alignUndefRadioButton,
               SIGNAL(toggled(bool)),
               this,
               SLOT(alignUndefRadioButtonToggled(bool)));

    disconnect(noPortRadioButton, SIGNAL(toggled(bool)), this, SLOT(noPortRadioButtonToggled(bool)));
    disconnect(portableRadioButton,
               SIGNAL(toggled(bool)),
               this,
               SLOT(portableRadioButtonToggled(bool)));
    disconnect(portUndefRadioButton,
               SIGNAL(toggled(bool)),
               this,
               SLOT(portUndefRadioButtonToggled(bool)));

    disconnect(noRideRadioButton, SIGNAL(toggled(bool)), this, SLOT(noRideRadioButtonToggled(bool)));
    disconnect(ridableRadioButton,
               SIGNAL(toggled(bool)),
               this,
               SLOT(ridableRadioButtonToggled(bool)));
    disconnect(rideUndefRadioButton,
               SIGNAL(toggled(bool)),
               this,
               SLOT(rideUndefRadioButtonToggled(bool)));

    disconnect(litRadioButton, SIGNAL(toggled(bool)), this, SLOT(litRadioButtonToggled(bool)));
    disconnect(darkRadioButton, SIGNAL(toggled(bool)), this, SLOT(darkRadioButtonToggled(bool)));
    disconnect(lightUndefRadioButton,
               SIGNAL(toggled(bool)),
               this,
               SLOT(lightUndefRadioButtonToggled(bool)));

    disconnect(mobFlagsListWidget,
               SIGNAL(itemChanged(QListWidgetItem *)),
               this,
               SLOT(mobFlagsListItemChanged(QListWidgetItem *)));
    disconnect(loadFlagsListWidget,
               SIGNAL(itemChanged(QListWidgetItem *)),
               this,
               SLOT(loadFlagsListItemChanged(QListWidgetItem *)));

    disconnect(exitNButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));
    disconnect(exitSButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));
    disconnect(exitEButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));
    disconnect(exitWButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));
    disconnect(exitUButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));
    disconnect(exitDButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));

    disconnect(exitFlagsListWidget,
               SIGNAL(itemChanged(QListWidgetItem *)),
               this,
               SLOT(exitFlagsListItemChanged(QListWidgetItem *)));
    disconnect(doorFlagsListWidget,
               SIGNAL(itemChanged(QListWidgetItem *)),
               this,
               SLOT(doorFlagsListItemChanged(QListWidgetItem *)));

    disconnect(doorNameLineEdit,
               SIGNAL(textChanged(QString)),
               this,
               SLOT(doorNameLineEditTextChanged(QString)));

    disconnect(toolButton00, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    disconnect(toolButton01, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    disconnect(toolButton02, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    disconnect(toolButton03, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    disconnect(toolButton04, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    disconnect(toolButton05, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    disconnect(toolButton06, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    disconnect(toolButton07, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    disconnect(toolButton08, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    disconnect(toolButton09, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    disconnect(toolButton10, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    disconnect(toolButton11, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    disconnect(toolButton12, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    disconnect(toolButton13, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    disconnect(toolButton14, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
    disconnect(toolButton15, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));

    disconnect(roomNoteTextEdit, SIGNAL(textChanged()), this, SLOT(roomNoteChanged()));

    disconnect(m_hiddenShortcut, SIGNAL(activated()), this, SLOT(toggleHiddenDoor()));
}

const Room *RoomEditAttrDlg::getSelectedRoom()
{
    if (m_roomSelection->empty()) {
        return nullptr;
    }
    if (m_roomSelection->size() == 1) {
        return (m_roomSelection->values().front());
    }
    return (m_roomSelection->value(
        (roomListComboBox->itemData(roomListComboBox->currentIndex()).toInt())));
}

uint RoomEditAttrDlg::getSelectedExit()
{
    if (exitNButton->isChecked()) {
        return 0;
    }
    if (exitSButton->isChecked()) {
        return 1;
    }
    if (exitEButton->isChecked()) {
        return 2;
    }
    if (exitWButton->isChecked()) {
        return 3;
    }
    if (exitUButton->isChecked()) {
        return 4;
    }
    if (exitDButton->isChecked()) {
        return 5;
    }
    return 6;
}

void RoomEditAttrDlg::roomListCurrentIndexChanged(int /*unused*/)
{
    disconnectAll();
    alignUndefRadioButton->setChecked(true);
    portUndefRadioButton->setChecked(true);
    lightUndefRadioButton->setChecked(true);
    connectAll();

    updateDialog(getSelectedRoom());
}

void RoomEditAttrDlg::setRoomSelection(const RoomSelection *rs, MapData *md, MapCanvas *mc)
{
    m_roomSelection = rs;
    m_mapData = md;
    m_mapCanvas = mc;

    roomListComboBox->clear();

    if (rs->size() > 1) {
        tabWidget->setCurrentWidget(selectionTab);
        roomListComboBox->addItem("All", 0);
        updateDialog(nullptr);

        disconnectAll();
        alignUndefRadioButton->setChecked(true);
        portUndefRadioButton->setChecked(true);
        lightUndefRadioButton->setChecked(true);
        connectAll();
    } else if (rs->size() == 1) {
        tabWidget->setCurrentWidget(attributesTab);
        updateDialog(m_roomSelection->values().front());
    }

    RoomSelection::const_iterator i = m_roomSelection->constBegin();
    while (i != m_roomSelection->constEnd()) {
        roomListComboBox->addItem(Mmapper2Room::getName(i.value()), i.value()->getId());
        ++i;
    }

    connect(roomListComboBox,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(roomListCurrentIndexChanged(int)));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(closeClicked()));
    connect(this, SIGNAL(mapChanged()), m_mapCanvas, SLOT(update()));
}

template<int N, typename Flags, typename Flag>
void setCheckState(QListWidgetItem *(&list)[N], int index, const Flags &flags, Flag flag)
{
    static_assert(N > 0, "");
    assert(index < N);
    assert(static_cast<int>(flag) == (1 << index));
    if (auto *x = list[index]) {
        x->setCheckState(ISSET(flags, flag) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    }
}

void RoomEditAttrDlg::updateDialog(const Room *r)
{
    disconnectAll();

    if (r == nullptr) {
        roomDescriptionTextEdit->clear();
        roomDescriptionTextEdit->setEnabled(false);

        updatedLabel->setText("");

        roomNoteTextEdit->clear();
        roomNoteTextEdit->setEnabled(false);

        terrainLabel->setPixmap(QPixmap(QString(":/pixmaps/terrain%1.png").arg(0)));

        exitsFrame->setEnabled(false);

        int index = 0;
        while (loadListItems[index] != nullptr) {
            loadListItems[index]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled
                                           | Qt::ItemIsTristate);
            loadListItems[index]->setCheckState(Qt::PartiallyChecked);
            index++;
        }

        index = 0;
        while (mobListItems[index] != nullptr) {
            mobListItems[index]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled
                                          | Qt::ItemIsTristate);
            mobListItems[index]->setCheckState(Qt::PartiallyChecked);
            index++;
        }
    } else {
        roomDescriptionTextEdit->clear();
        roomDescriptionTextEdit->setEnabled(true);

        if (r->isUpToDate()) {
            updatedLabel->setText("Room has been online updated.");
        } else {
            updatedLabel->setText("Room has not been online updated yet!!!");
        }

        exitsFrame->setEnabled(true);

        const Exit &e = r->exit(getSelectedExit());
        ExitFlags ef = Mmapper2Exit::getFlags(e);

        setCheckState(exitListItems, 0, ef, EF_EXIT);

        if (ISSET(ef, EF_DOOR)) {
            doorNameLineEdit->setEnabled(true);
            doorFlagsListWidget->setEnabled(true);

            exitListItems[1]->setCheckState(Qt::Checked);
            doorNameLineEdit->setText(Mmapper2Exit::getDoorName(e));

            DoorFlags df = Mmapper2Exit::getDoorFlags(e);

            setCheckState(doorListItems, 0, df, DF_HIDDEN);
            setCheckState(doorListItems, 1, df, DF_NEEDKEY);
            setCheckState(doorListItems, 2, df, DF_NOBLOCK);
            setCheckState(doorListItems, 3, df, DF_NOBREAK);
            setCheckState(doorListItems, 4, df, DF_NOPICK);
            setCheckState(doorListItems, 5, df, DF_DELAYED);
            setCheckState(doorListItems, 6, df, DF_CALLABLE);
            setCheckState(doorListItems, 7, df, DF_KNOCKABLE);
            setCheckState(doorListItems, 8, df, DF_MAGIC);
            setCheckState(doorListItems, 9, df, DF_ACTION);
        } else {
            doorNameLineEdit->clear();
            doorNameLineEdit->setEnabled(false);
            doorFlagsListWidget->setEnabled(false);

            exitListItems[1]->setCheckState(Qt::Unchecked);
        }

        setCheckState(exitListItems, 2, ef, EF_ROAD);
        setCheckState(exitListItems, 3, ef, EF_CLIMB);
        setCheckState(exitListItems, 4, ef, EF_RANDOM);
        setCheckState(exitListItems, 5, ef, EF_SPECIAL);
        setCheckState(exitListItems, 6, ef, EF_NO_MATCH);
        setCheckState(exitListItems, 7, ef, EF_FLOW);
        setCheckState(exitListItems, 8, ef, EF_NO_FLEE);
        setCheckState(exitListItems, 9, ef, EF_DAMAGE);
        setCheckState(exitListItems, 10, ef, EF_FALL);
        setCheckState(exitListItems, 11, ef, EF_GUARDED);

        roomNoteTextEdit->clear();
        roomNoteTextEdit->setEnabled(false);

        int index = 0;
        while (loadListItems[index] != nullptr) {
            loadListItems[index]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            index++;
        }

        index = 0;
        while (mobListItems[index] != nullptr) {
            mobListItems[index]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            index++;
        }

        {
            RoomMobFlags rmf = Mmapper2Room::getMobFlags(r);
            setCheckState(mobListItems, 0, rmf, RMF_RENT);
            setCheckState(mobListItems, 1, rmf, RMF_SHOP);
            setCheckState(mobListItems, 2, rmf, RMF_WEAPONSHOP);
            setCheckState(mobListItems, 3, rmf, RMF_ARMOURSHOP);
            setCheckState(mobListItems, 4, rmf, RMF_FOODSHOP);
            setCheckState(mobListItems, 5, rmf, RMF_PETSHOP);
            setCheckState(mobListItems, 6, rmf, RMF_GUILD);
            setCheckState(mobListItems, 7, rmf, RMF_SCOUTGUILD);
            setCheckState(mobListItems, 8, rmf, RMF_MAGEGUILD);
            setCheckState(mobListItems, 9, rmf, RMF_CLERICGUILD);
            setCheckState(mobListItems, 10, rmf, RMF_WARRIORGUILD);
            setCheckState(mobListItems, 11, rmf, RMF_RANGERGUILD);
            setCheckState(mobListItems, 12, rmf, RMF_SMOB);
            setCheckState(mobListItems, 13, rmf, RMF_QUEST);
            setCheckState(mobListItems, 14, rmf, RMF_ANY);
        }

        {
            RoomLoadFlags rlf = Mmapper2Room::getLoadFlags(r);
            setCheckState(loadListItems, 0, rlf, RLF_TREASURE);
            setCheckState(loadListItems, 1, rlf, RLF_ARMOUR);
            setCheckState(loadListItems, 2, rlf, RLF_WEAPON);
            setCheckState(loadListItems, 3, rlf, RLF_WATER);
            setCheckState(loadListItems, 4, rlf, RLF_FOOD);
            setCheckState(loadListItems, 5, rlf, RLF_HERB);
            setCheckState(loadListItems, 6, rlf, RLF_KEY);
            setCheckState(loadListItems, 7, rlf, RLF_MULE);
            setCheckState(loadListItems, 8, rlf, RLF_HORSE);
            setCheckState(loadListItems, 9, rlf, RLF_PACKHORSE);
            setCheckState(loadListItems, 10, rlf, RLF_TRAINEDHORSE);
            setCheckState(loadListItems, 11, rlf, RLF_ROHIRRIM);
            setCheckState(loadListItems, 12, rlf, RLF_WARG);
            setCheckState(loadListItems, 13, rlf, RLF_BOAT);
            setCheckState(loadListItems, 14, rlf, RLF_ATTENTION);
            setCheckState(loadListItems, 15, rlf, RLF_TOWER);
            setCheckState(loadListItems, 16, rlf, RLF_CLOCK);
            setCheckState(loadListItems, 17, rlf, RLF_MAIL);
            setCheckState(loadListItems, 18, rlf, RLF_STABLE);
        }

        roomDescriptionTextEdit->setEnabled(true);
        roomNoteTextEdit->setEnabled(true);

        roomDescriptionTextEdit->clear();
        roomDescriptionTextEdit->setFontItalic(false);
        QString str = Mmapper2Room::getDescription(r);
        str = str.left(str.length() - 1);
        roomDescriptionTextEdit->append(str);
        roomDescriptionTextEdit->setFontItalic(true);
        roomDescriptionTextEdit->append(Mmapper2Room::getDynamicDescription(r));
        roomDescriptionTextEdit->scroll(-100, -100);

        roomNoteTextEdit->clear();
        roomNoteTextEdit->append(Mmapper2Room::getNote(r));

        terrainLabel->setPixmap(
            QPixmap(QString(":/pixmaps/terrain%1.png").arg(Mmapper2Room::getTerrainType(r))));

        switch (Mmapper2Room::getAlignType(r)) {
        case RAT_GOOD:
            goodRadioButton->setChecked(true);
            break;
        case RAT_NEUTRAL:
            neutralRadioButton->setChecked(true);
            break;
        case RAT_EVIL:
            evilRadioButton->setChecked(true);
            break;
        case RAT_UNDEFINED:
            alignUndefRadioButton->setChecked(true);
            break;
        }

        switch (Mmapper2Room::getPortableType(r)) {
        case RPT_PORTABLE:
            portableRadioButton->setChecked(true);
            break;
        case RPT_NOTPORTABLE:
            noPortRadioButton->setChecked(true);
            break;
        case RPT_UNDEFINED:
            portUndefRadioButton->setChecked(true);
            break;
        }

        switch (Mmapper2Room::getRidableType(r)) {
        case RRT_RIDABLE:
            ridableRadioButton->setChecked(true);
            break;
        case RRT_NOTRIDABLE:
            noRideRadioButton->setChecked(true);
            break;
        case RRT_UNDEFINED:
            rideUndefRadioButton->setChecked(true);
            break;
        }

        switch (Mmapper2Room::getLightType(r)) {
        case RLT_DARK:
            darkRadioButton->setChecked(true);
            break;
        case RLT_LIT:
            litRadioButton->setChecked(true);
            break;
        case RLT_UNDEFINED:
            lightUndefRadioButton->setChecked(true);
            break;
        }

        switch (Mmapper2Room::getSundeathType(r)) {
        case RST_NOSUNDEATH:
            noSundeathRadioButton->setChecked(true);
            break;
        case RST_SUNDEATH:
            sundeathRadioButton->setChecked(true);
            break;
        case RST_UNDEFINED:
            sundeathUndefRadioButton->setChecked(true);
            break;
        }
    }

    connectAll();
}

//attributes page
void RoomEditAttrDlg::exitButtonToggled(bool /*unused*/)
{
    updateDialog(getSelectedRoom());
}

void RoomEditAttrDlg::neutralRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RAT_NEUTRAL, R_ALIGNTYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RAT_NEUTRAL, R_ALIGNTYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::goodRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RAT_GOOD, R_ALIGNTYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RAT_GOOD, R_ALIGNTYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::evilRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RAT_EVIL, R_ALIGNTYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RAT_EVIL, R_ALIGNTYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::alignUndefRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RAT_UNDEFINED, R_ALIGNTYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RAT_UNDEFINED, R_ALIGNTYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::noPortRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RPT_NOTPORTABLE,
                                                                        R_PORTABLETYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RPT_NOTPORTABLE, R_PORTABLETYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::portableRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RPT_PORTABLE,
                                                                        R_PORTABLETYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RPT_PORTABLE, R_PORTABLETYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::portUndefRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RPT_UNDEFINED,
                                                                        R_PORTABLETYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RPT_UNDEFINED, R_PORTABLETYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::noRideRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RRT_NOTRIDABLE,
                                                                        R_RIDABLETYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RRT_NOTRIDABLE, R_RIDABLETYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::ridableRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RRT_RIDABLE, R_RIDABLETYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RRT_RIDABLE, R_RIDABLETYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::rideUndefRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RRT_UNDEFINED,
                                                                        R_RIDABLETYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RRT_UNDEFINED, R_RIDABLETYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::litRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RLT_LIT, R_LIGHTTYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RLT_LIT, R_LIGHTTYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::darkRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RLT_DARK, R_LIGHTTYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RLT_DARK, R_LIGHTTYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::lightUndefRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RLT_UNDEFINED, R_LIGHTTYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RLT_UNDEFINED, R_LIGHTTYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::sundeathRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RST_SUNDEATH,
                                                                        R_SUNDEATHTYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RST_SUNDEATH, R_SUNDEATHTYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::noSundeathRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RST_NOSUNDEATH,
                                                                        R_SUNDEATHTYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RST_NOSUNDEATH, R_SUNDEATHTYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::sundeathUndefRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RST_UNDEFINED,
                                                                        R_SUNDEATHTYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RST_UNDEFINED, R_SUNDEATHTYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::mobFlagsListItemChanged(QListWidgetItem *item)
{
    int index = 0;
    while (item != mobListItems[index]) {
        index++;
    }

    const Room *r = getSelectedRoom();

    switch (item->checkState()) {
    case Qt::Unchecked:
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags(static_cast<uint>(
                                                                            pow(2.0, index)),
                                                                        R_MOBFLAGS,
                                                                        FMM_UNSET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new ModifyRoomFlags(static_cast<uint>(
                                                                       pow(2.0, index)),
                                                                   R_MOBFLAGS,
                                                                   FMM_UNSET),
                                               m_roomSelection),
                               m_roomSelection);
        }
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags(static_cast<uint>(
                                                                            pow(2.0, index)),
                                                                        R_MOBFLAGS,
                                                                        FMM_SET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new ModifyRoomFlags(static_cast<uint>(
                                                                       pow(2.0, index)),
                                                                   R_MOBFLAGS,
                                                                   FMM_SET),
                                               m_roomSelection),
                               m_roomSelection);
        }
        break;
    }

    emit mapChanged();
}

void RoomEditAttrDlg::loadFlagsListItemChanged(QListWidgetItem *item)
{
    int index = 0;
    while (item != loadListItems[index]) {
        index++;
    }

    const Room *r = getSelectedRoom();

    switch (item->checkState()) {
    case Qt::Unchecked:
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags(static_cast<uint>(
                                                                            pow(2.0, index)),
                                                                        R_LOADFLAGS,
                                                                        FMM_UNSET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new ModifyRoomFlags(static_cast<uint>(
                                                                       pow(2.0, index)),
                                                                   R_LOADFLAGS,
                                                                   FMM_UNSET),
                                               m_roomSelection),
                               m_roomSelection);
        }
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags(static_cast<uint>(
                                                                            pow(2.0, index)),
                                                                        R_LOADFLAGS,
                                                                        FMM_SET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new ModifyRoomFlags(static_cast<uint>(
                                                                       pow(2.0, index)),
                                                                   R_LOADFLAGS,
                                                                   FMM_SET),
                                               m_roomSelection),
                               m_roomSelection);
        }
        break;
    }

    emit mapChanged();
}

void RoomEditAttrDlg::exitFlagsListItemChanged(QListWidgetItem *item)
{
    int index = 0;
    while (item != exitListItems[index]) {
        index++;
    }

    const Room *r = getSelectedRoom();

    switch (item->checkState()) {
    case Qt::Unchecked:
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyExitFlags(static_cast<uint>(
                                                                            pow(2.0, index)),
                                                                        getSelectedExit(),
                                                                        E_FLAGS,
                                                                        FMM_UNSET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new ModifyExitFlags(static_cast<uint>(
                                                                       pow(2.0, index)),
                                                                   getSelectedExit(),
                                                                   E_FLAGS,
                                                                   FMM_UNSET),
                                               m_roomSelection),
                               m_roomSelection);
        }
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyExitFlags(static_cast<uint>(
                                                                            pow(2.0, index)),
                                                                        getSelectedExit(),
                                                                        E_FLAGS,
                                                                        FMM_SET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new ModifyExitFlags(static_cast<uint>(
                                                                       pow(2.0, index)),
                                                                   getSelectedExit(),
                                                                   E_FLAGS,
                                                                   FMM_SET),
                                               m_roomSelection),
                               m_roomSelection);
        }
        break;
    }

    updateDialog(getSelectedRoom());
    emit mapChanged();
}

void RoomEditAttrDlg::doorNameLineEditTextChanged(const QString /*unused*/ &)
{
    const Room *r = getSelectedRoom();

    m_mapData->execute(new SingleRoomAction(new UpdateExitField(doorNameLineEdit->text(),
                                                                getSelectedExit(),
                                                                E_DOORNAME),
                                            r->getId()),
                       m_roomSelection);
}

void RoomEditAttrDlg::doorFlagsListItemChanged(QListWidgetItem *item)
{
    int index = 0;
    while (item != doorListItems[index]) {
        index++;
    }

    const Room *r = getSelectedRoom();

    switch (item->checkState()) {
    case Qt::Unchecked:
        m_mapData->execute(new SingleRoomAction(new ModifyExitFlags(static_cast<uint>(
                                                                        pow(2.0, index)),
                                                                    getSelectedExit(),
                                                                    E_DOORFLAGS,
                                                                    FMM_UNSET),
                                                r->getId()),
                           m_roomSelection);
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        m_mapData->execute(new SingleRoomAction(new ModifyExitFlags(static_cast<uint>(
                                                                        pow(2.0, index)),
                                                                    getSelectedExit(),
                                                                    E_DOORFLAGS,
                                                                    FMM_SET),
                                                r->getId()),
                           m_roomSelection);
        break;
    }

    emit mapChanged();
}

void RoomEditAttrDlg::toggleHiddenDoor()
{
    if (doorFlagsListWidget->isEnabled()) {
        doorListItems[0]->setCheckState(
            doorListItems[0]->checkState() == Qt::Unchecked ? Qt::Checked : Qt::Unchecked);
    }
}

//terrain tab
void RoomEditAttrDlg::terrainToolButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();

        uint index = 0;
        if (toolButton00->isChecked()) {
            index = 0;
        }
        if (toolButton01->isChecked()) {
            index = 1;
        }
        if (toolButton02->isChecked()) {
            index = 2;
        }
        if (toolButton03->isChecked()) {
            index = 3;
        }
        if (toolButton04->isChecked()) {
            index = 4;
        }
        if (toolButton05->isChecked()) {
            index = 5;
        }
        if (toolButton06->isChecked()) {
            index = 6;
        }
        if (toolButton07->isChecked()) {
            index = 7;
        }
        if (toolButton08->isChecked()) {
            index = 8;
        }
        if (toolButton09->isChecked()) {
            index = 9;
        }
        if (toolButton10->isChecked()) {
            index = 10;
        }
        if (toolButton11->isChecked()) {
            index = 11;
        }
        if (toolButton12->isChecked()) {
            index = 12;
        }
        if (toolButton13->isChecked()) {
            index = 13;
        }
        if (toolButton14->isChecked()) {
            index = 14;
        }
        if (toolButton15->isChecked()) {
            index = 15;
        }

        terrainLabel->setPixmap(QPixmap(QString(":/pixmaps/terrain%1.png").arg(index)));

        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(index, R_TERRAINTYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(index, R_TERRAINTYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
    }
}

//note tab
void RoomEditAttrDlg::roomNoteChanged()
{
    const Room *r = getSelectedRoom();

    if (r != nullptr) {
        m_mapData->execute(new SingleRoomAction(new UpdateRoomField(roomNoteTextEdit->document()
                                                                        ->toPlainText(),
                                                                    R_NOTE),
                                                r->getId()),
                           m_roomSelection);
    } else {
        m_mapData->execute(new GroupAction(new UpdateRoomField(roomNoteTextEdit->document()
                                                                   ->toPlainText(),
                                                               R_NOTE),
                                           m_roomSelection),
                           m_roomSelection);
    }

    emit mapChanged();
}

//all tabs
void RoomEditAttrDlg::closeClicked()
{
    accept();
}
