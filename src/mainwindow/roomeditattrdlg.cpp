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

#include <cstddef>
#include <QMessageLogContext>
#include <QString>
#include <QtGui>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "../display/mapcanvas.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../global/utils.h"
#include "../mapdata/ExitFieldVariant.h"
#include "../mapdata/customaction.h"
#include "../mapdata/enums.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/mmapper2room.h"
#include "../mapdata/roomselection.h"
#include "../mapfrontend/mapaction.h"

template<typename T>
void fixMissing(T &array, const char *name)
{
    for (QListWidgetItem *&x : array) {
        if (x != nullptr)
            continue;
        const auto ordinal = static_cast<int>(&x - array.data());
        x = new QListWidgetItem(QString::asprintf("%d", ordinal));
        qWarning() << "Missing " << name << " " << ordinal;
    }
}

template<typename T>
void installWidgets(T &array,
                    const char *name,
                    QListWidget &widget,
                    const QFlags<Qt::ItemFlag> &flags)
{
    fixMissing(array, name);
    widget.clear();
    for (QListWidgetItem *x : array) {
        x->setFlags(flags);
        widget.addItem(x);
    }
}

/* TODO: merge this with human-readable names used in parser output */
static QString getName(const RoomMobFlag flag)
{
#define CASE2(UPPER, desc) \
    do { \
    case RoomMobFlag::UPPER: \
        return desc; \
    } while (false)
    switch (flag) {
        CASE2(RENT, "Rent place");
        CASE2(SHOP, "Generic shop");
        CASE2(WEAPON_SHOP, "Weapon shop");
        CASE2(ARMOUR_SHOP, "Armour shop");
        CASE2(FOOD_SHOP, "Food shop");
        CASE2(PET_SHOP, "Pet shop");
        CASE2(GUILD, "Generic guild");
        CASE2(SCOUT_GUILD, "Scout guild");
        CASE2(MAGE_GUILD, "Mage guild");
        CASE2(CLERIC_GUILD, "Cleric guild");
        CASE2(WARRIOR_GUILD, "Warrior guild");
        CASE2(RANGER_GUILD, "Ranger guild");
        CASE2(SMOB, "SMOB");
        CASE2(QUEST, "Quest mob");
        CASE2(ANY, "Any mob");
    }
    return QString::asprintf("(RoomMobFlag)%d", static_cast<int>(flag));
#undef CASE2
}

/* TODO: merge this with human-readable names used in parser output */
static QString getName(const RoomLoadFlag flag)
{
#define CASE2(UPPER, desc) \
    do { \
    case RoomLoadFlag::UPPER: \
        return desc; \
    } while (false)
    switch (flag) {
        CASE2(TREASURE, "Treasure");
        CASE2(ARMOUR, "Equipment");
        CASE2(WEAPON, "Weapon");
        CASE2(WATER, "Water");
        CASE2(FOOD, "Food");
        CASE2(HERB, "Herb");
        CASE2(KEY, "Key");
        CASE2(MULE, "Mule");
        CASE2(HORSE, "Horse");
        CASE2(PACK_HORSE, "Pack horse");
        CASE2(TRAINED_HORSE, "Trained horse");
        CASE2(ROHIRRIM, "Rohirrim");
        CASE2(WARG, "Warg");
        CASE2(BOAT, "Boat");
        CASE2(ATTENTION, "Attention");
        CASE2(TOWER, "Tower");
        CASE2(CLOCK, "Clock");
        CASE2(MAIL, "Mail");
        CASE2(STABLE, "Stable");
    }
    return QString::asprintf("(RoomLoadFlag)%d", static_cast<int>(flag));
#undef CASE2
}

/* TODO: merge this with human-readable names used in parser output */
static QString getName(const ExitFlag flag)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case ExitFlag::UPPER_CASE: \
        return friendly; \
    } while (false);
    switch (flag) {
        X_FOREACH_EXIT_FLAG(X_CASE)
    }
    return QString::asprintf("(ExitFlag)%d", static_cast<int>(flag));
#undef X_CASE
}

/* TODO: merge this with human-readable names used in parser output */
static QString getName(const DoorFlag flag)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case DoorFlag::UPPER_CASE: \
        return friendly; \
    } while (false);
    switch (flag) {
        X_FOREACH_DOOR_FLAG(X_CASE)
    }
    return QString::asprintf("(DoorFlag)%d", static_cast<int>(flag));
#undef X_CASE
}

RoomEditAttrDlg::RoomEditAttrDlg(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    roomDescriptionTextEdit->setLineWrapMode(QTextEdit::NoWrap);

    for (auto flag : ALL_MOB_FLAGS)
        mobListItems[flag] = new QListWidgetItem(getName(flag));
    installWidgets(mobListItems,
                   "mob room flags",
                   *mobFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsTristate);

    for (auto flag : ALL_LOAD_FLAGS)
        loadListItems[flag] = new QListWidgetItem(getName(flag));
    installWidgets(loadListItems,
                   "load list",
                   *loadFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsTristate);

    for (auto flag : ALL_EXIT_FLAGS)
        exitListItems[flag] = new QListWidgetItem(getName(flag));
    installWidgets(exitListItems,
                   "exit list",
                   *exitFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

    for (auto flag : ALL_DOOR_FLAGS)
        doorListItems[flag] = new QListWidgetItem(getName(flag));
    installWidgets(doorListItems,
                   "door list",
                   *doorFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

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
    const auto geometry = Config().readRoomEditAttrGeometry();
    restoreGeometry(geometry);
}

void RoomEditAttrDlg::writeSettings()
{
    Config().writeRoomEditAttrDlgGeometry(saveGeometry());
}

void RoomEditAttrDlg::connectAll()
{
    connect(neutralRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::neutralRadioButtonToggled);
    connect(goodRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::goodRadioButtonToggled);
    connect(evilRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::evilRadioButtonToggled);
    connect(alignUndefRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::alignUndefRadioButtonToggled);

    connect(noPortRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::noPortRadioButtonToggled);
    connect(portableRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::portableRadioButtonToggled);
    connect(portUndefRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::portUndefRadioButtonToggled);

    connect(noRideRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::noRideRadioButtonToggled);
    connect(ridableRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::ridableRadioButtonToggled);
    connect(rideUndefRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::rideUndefRadioButtonToggled);

    connect(litRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::litRadioButtonToggled);
    connect(darkRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::darkRadioButtonToggled);
    connect(lightUndefRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::lightUndefRadioButtonToggled);

    connect(sundeathRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::sundeathRadioButtonToggled);
    connect(noSundeathRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::noSundeathRadioButtonToggled);
    connect(sundeathUndefRadioButton,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::sundeathUndefRadioButtonToggled);

    connect(mobFlagsListWidget,
            &QListWidget::itemChanged,
            this,
            &RoomEditAttrDlg::mobFlagsListItemChanged);
    connect(loadFlagsListWidget,
            &QListWidget::itemChanged,
            this,
            &RoomEditAttrDlg::loadFlagsListItemChanged);

    connect(exitNButton, &QAbstractButton::toggled, this, &RoomEditAttrDlg::exitButtonToggled);
    connect(exitSButton, &QAbstractButton::toggled, this, &RoomEditAttrDlg::exitButtonToggled);
    connect(exitEButton, &QAbstractButton::toggled, this, &RoomEditAttrDlg::exitButtonToggled);
    connect(exitWButton, &QAbstractButton::toggled, this, &RoomEditAttrDlg::exitButtonToggled);
    connect(exitUButton, &QAbstractButton::toggled, this, &RoomEditAttrDlg::exitButtonToggled);
    connect(exitDButton, &QAbstractButton::toggled, this, &RoomEditAttrDlg::exitButtonToggled);

    connect(exitFlagsListWidget,
            &QListWidget::itemChanged,
            this,
            &RoomEditAttrDlg::exitFlagsListItemChanged);
    connect(doorFlagsListWidget,
            &QListWidget::itemChanged,
            this,
            &RoomEditAttrDlg::doorFlagsListItemChanged);

    connect(doorNameLineEdit,
            &QLineEdit::textChanged,
            this,
            &RoomEditAttrDlg::doorNameLineEditTextChanged);

    connect(toolButton00,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);
    connect(toolButton01,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);
    connect(toolButton02,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);
    connect(toolButton03,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);
    connect(toolButton04,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);
    connect(toolButton05,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);
    connect(toolButton06,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);
    connect(toolButton07,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);
    connect(toolButton08,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);
    connect(toolButton09,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);
    connect(toolButton10,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);
    connect(toolButton11,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);
    connect(toolButton12,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);
    connect(toolButton13,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);
    connect(toolButton14,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);
    connect(toolButton15,
            &QAbstractButton::toggled,
            this,
            &RoomEditAttrDlg::terrainToolButtonToggled);

    connect(roomNoteTextEdit, &QTextEdit::textChanged, this, &RoomEditAttrDlg::roomNoteChanged);

    connect(m_hiddenShortcut, &QShortcut::activated, this, &RoomEditAttrDlg::toggleHiddenDoor);
}

void RoomEditAttrDlg::disconnectAll()
{
    disconnect(neutralRadioButton,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::neutralRadioButtonToggled);
    disconnect(goodRadioButton,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::goodRadioButtonToggled);
    disconnect(evilRadioButton,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::evilRadioButtonToggled);
    disconnect(alignUndefRadioButton,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::alignUndefRadioButtonToggled);

    disconnect(noPortRadioButton,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::noPortRadioButtonToggled);
    disconnect(portableRadioButton,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::portableRadioButtonToggled);
    disconnect(portUndefRadioButton,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::portUndefRadioButtonToggled);

    disconnect(noRideRadioButton,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::noRideRadioButtonToggled);
    disconnect(ridableRadioButton,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::ridableRadioButtonToggled);
    disconnect(rideUndefRadioButton,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::rideUndefRadioButtonToggled);

    disconnect(litRadioButton,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::litRadioButtonToggled);
    disconnect(darkRadioButton,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::darkRadioButtonToggled);
    disconnect(lightUndefRadioButton,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::lightUndefRadioButtonToggled);

    disconnect(mobFlagsListWidget,
               &QListWidget::itemChanged,
               this,
               &RoomEditAttrDlg::mobFlagsListItemChanged);
    disconnect(loadFlagsListWidget,
               &QListWidget::itemChanged,
               this,
               &RoomEditAttrDlg::loadFlagsListItemChanged);

    disconnect(exitNButton, &QAbstractButton::toggled, this, &RoomEditAttrDlg::exitButtonToggled);
    disconnect(exitSButton, &QAbstractButton::toggled, this, &RoomEditAttrDlg::exitButtonToggled);
    disconnect(exitEButton, &QAbstractButton::toggled, this, &RoomEditAttrDlg::exitButtonToggled);
    disconnect(exitWButton, &QAbstractButton::toggled, this, &RoomEditAttrDlg::exitButtonToggled);
    disconnect(exitUButton, &QAbstractButton::toggled, this, &RoomEditAttrDlg::exitButtonToggled);
    disconnect(exitDButton, &QAbstractButton::toggled, this, &RoomEditAttrDlg::exitButtonToggled);

    disconnect(exitFlagsListWidget,
               &QListWidget::itemChanged,
               this,
               &RoomEditAttrDlg::exitFlagsListItemChanged);
    disconnect(doorFlagsListWidget,
               &QListWidget::itemChanged,
               this,
               &RoomEditAttrDlg::doorFlagsListItemChanged);

    disconnect(doorNameLineEdit,
               &QLineEdit::textChanged,
               this,
               &RoomEditAttrDlg::doorNameLineEditTextChanged);

    disconnect(toolButton00,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);
    disconnect(toolButton01,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);
    disconnect(toolButton02,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);
    disconnect(toolButton03,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);
    disconnect(toolButton04,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);
    disconnect(toolButton05,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);
    disconnect(toolButton06,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);
    disconnect(toolButton07,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);
    disconnect(toolButton08,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);
    disconnect(toolButton09,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);
    disconnect(toolButton10,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);
    disconnect(toolButton11,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);
    disconnect(toolButton12,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);
    disconnect(toolButton13,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);
    disconnect(toolButton14,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);
    disconnect(toolButton15,
               &QAbstractButton::toggled,
               this,
               &RoomEditAttrDlg::terrainToolButtonToggled);

    disconnect(roomNoteTextEdit, &QTextEdit::textChanged, this, &RoomEditAttrDlg::roomNoteChanged);

    disconnect(m_hiddenShortcut, &QShortcut::activated, this, &RoomEditAttrDlg::toggleHiddenDoor);
}

const Room *RoomEditAttrDlg::getSelectedRoom()
{
    if (m_roomSelection->empty()) {
        return nullptr;
    }
    if (m_roomSelection->size() == 1) {
        return (m_roomSelection->values().front());
    }
    return m_roomSelection->value(
        RoomId{roomListComboBox->itemData(roomListComboBox->currentIndex()).toUInt()});
}

ExitDirection RoomEditAttrDlg::getSelectedExit()
{
    if (exitNButton->isChecked()) {
        return ExitDirection::NORTH;
    }
    if (exitSButton->isChecked()) {
        return ExitDirection::SOUTH;
    }
    if (exitEButton->isChecked()) {
        return ExitDirection::EAST;
    }
    if (exitWButton->isChecked()) {
        return ExitDirection::WEST;
    }
    if (exitUButton->isChecked()) {
        return ExitDirection::UP;
    }
    if (exitDButton->isChecked()) {
        return ExitDirection::DOWN;
    }
    return ExitDirection::UNKNOWN;
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

    for (auto room : m_roomSelection->values()) {
        roomListComboBox->addItem(room->getName(), room->getId().asUint32());
    }

    connect(roomListComboBox,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(roomListCurrentIndexChanged(int)));
    connect(closeButton, &QAbstractButton::clicked, this, &RoomEditAttrDlg::closeClicked);
    connect(this, SIGNAL(mapChanged()), m_mapCanvas, SLOT(update()));
}

template<typename T, typename Flags>
void setCheckStates(T &array, const Flags &flags)
{
    for (size_t i = 0, len = array.size(); i < len; ++i) {
        auto flag = static_cast<typename T::index_type>(i);
        if (auto *x = array[flag]) {
            x->setCheckState(flags.contains(flag) ? Qt::CheckState::Checked
                                                  : Qt::CheckState::Unchecked);
        }
    }
}

template<typename T>
void setFlags(T &array, const QFlags<Qt::ItemFlag> &flags)
{
    for (auto &x : array) {
        x->setFlags(flags);
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

        for (auto x : loadListItems) {
            x->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsTristate);
            x->setCheckState(Qt::PartiallyChecked);
        }

        for (auto x : mobListItems) {
            x->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsTristate);
            x->setCheckState(Qt::PartiallyChecked);
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
        setCheckStates(exitListItems, e.getExitFlags());

        if (e.isDoor()) {
            doorNameLineEdit->setEnabled(true);
            doorFlagsListWidget->setEnabled(true);
            doorNameLineEdit->setText(e.getDoorName());
            setCheckStates(doorListItems, e.getDoorFlags());

        } else {
            doorNameLineEdit->clear();
            doorNameLineEdit->setEnabled(false);
            doorFlagsListWidget->setEnabled(false);
        }

        roomNoteTextEdit->clear();
        roomNoteTextEdit->setEnabled(false);

        setFlags(loadListItems, Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        setFlags(mobListItems, Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

        setCheckStates(mobListItems, r->getMobFlags());
        setCheckStates(loadListItems, r->getLoadFlags());

        roomDescriptionTextEdit->setEnabled(true);
        roomNoteTextEdit->setEnabled(true);

        roomDescriptionTextEdit->clear();
        roomDescriptionTextEdit->setFontItalic(false);
        {
            QString str = r->getStaticDescription();
            str = str.left(str.length() - 1);
            roomDescriptionTextEdit->append(str);
        }
        roomDescriptionTextEdit->setFontItalic(true);
        roomDescriptionTextEdit->append(r->getDynamicDescription());
        roomDescriptionTextEdit->scroll(-100, -100);

        roomNoteTextEdit->clear();
        roomNoteTextEdit->append(r->getNote());

        const auto get_terrain_pixmap = [](RoomTerrainType type) -> QString {
            switch (type) {
            case RoomTerrainType::ROAD:
                return QString(":/pixmaps/road7.png");
            default:
                return QString(":/pixmaps/terrain%1.png").arg(static_cast<int>(type));
            }
        };
        terrainLabel->setPixmap(get_terrain_pixmap(r->getTerrainType()));

        switch (r->getAlignType()) {
        case RoomAlignType::GOOD:
            goodRadioButton->setChecked(true);
            break;
        case RoomAlignType::NEUTRAL:
            neutralRadioButton->setChecked(true);
            break;
        case RoomAlignType::EVIL:
            evilRadioButton->setChecked(true);
            break;
        case RoomAlignType::UNDEFINED:
            alignUndefRadioButton->setChecked(true);
            break;
        }

        switch (r->getPortableType()) {
        case RoomPortableType::PORTABLE:
            portableRadioButton->setChecked(true);
            break;
        case RoomPortableType::NOT_PORTABLE:
            noPortRadioButton->setChecked(true);
            break;
        case RoomPortableType::UNDEFINED:
            portUndefRadioButton->setChecked(true);
            break;
        }

        switch (r->getRidableType()) {
        case RoomRidableType::RIDABLE:
            ridableRadioButton->setChecked(true);
            break;
        case RoomRidableType::NOT_RIDABLE:
            noRideRadioButton->setChecked(true);
            break;
        case RoomRidableType::UNDEFINED:
            rideUndefRadioButton->setChecked(true);
            break;
        }

        switch (r->getLightType()) {
        case RoomLightType::DARK:
            darkRadioButton->setChecked(true);
            break;
        case RoomLightType::LIT:
            litRadioButton->setChecked(true);
            break;
        case RoomLightType::UNDEFINED:
            lightUndefRadioButton->setChecked(true);
            break;
        }

        switch (r->getSundeathType()) {
        case RoomSundeathType::NO_SUNDEATH:
            noSundeathRadioButton->setChecked(true);
            break;
        case RoomSundeathType::SUNDEATH:
            sundeathRadioButton->setChecked(true);
            break;
        case RoomSundeathType::UNDEFINED:
            sundeathUndefRadioButton->setChecked(true);
            break;
        }

        switch (r->getTerrainType()) {
        case RoomTerrainType::UNDEFINED:
            toolButton00->setChecked(true);
            break;
        case RoomTerrainType::INDOORS:
            toolButton01->setChecked(true);
            break;
        case RoomTerrainType::CITY:
            toolButton02->setChecked(true);
            break;
        case RoomTerrainType::FIELD:
            toolButton03->setChecked(true);
            break;
        case RoomTerrainType::FOREST:
            toolButton04->setChecked(true);
            break;
        case RoomTerrainType::HILLS:
            toolButton05->setChecked(true);
            break;
        case RoomTerrainType::MOUNTAINS:
            toolButton06->setChecked(true);
            break;
        case RoomTerrainType::SHALLOW:
            toolButton07->setChecked(true);
            break;
        case RoomTerrainType::WATER:
            toolButton08->setChecked(true);
            break;
        case RoomTerrainType::RAPIDS:
            toolButton09->setChecked(true);
            break;
        case RoomTerrainType::UNDERWATER:
            toolButton10->setChecked(true);
            break;
        case RoomTerrainType::ROAD:
            toolButton11->setChecked(true);
            break;
        case RoomTerrainType::BRUSH:
            toolButton12->setChecked(true);
            break;
        case RoomTerrainType::TUNNEL:
            toolButton13->setChecked(true);
            break;
        case RoomTerrainType::CAVERN:
            toolButton14->setChecked(true);
            break;
        case RoomTerrainType::DEATHTRAP:
            toolButton15->setChecked(true);
            break;
        case RoomTerrainType::RANDOM:
            // REVISIT: add a button?
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomAlignType::NEUTRAL,
                                                                        RoomField::ALIGN_TYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomAlignType::NEUTRAL,
                                                                   RoomField::ALIGN_TYPE),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomAlignType::GOOD,
                                                                        RoomField::ALIGN_TYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomAlignType::GOOD,
                                                                   RoomField::ALIGN_TYPE),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomAlignType::EVIL,
                                                                        RoomField::ALIGN_TYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomAlignType::EVIL,
                                                                   RoomField::ALIGN_TYPE),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomAlignType::UNDEFINED,
                                                                        RoomField::ALIGN_TYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomAlignType::UNDEFINED,
                                                                   RoomField::ALIGN_TYPE),
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
        if (const Room *r = getSelectedRoom()) {
            m_mapData
                ->execute(new SingleRoomAction(new UpdateRoomField(RoomPortableType::NOT_PORTABLE,
                                                                   RoomField::PORTABLE_TYPE),
                                               r->getId()),
                          m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomPortableType::NOT_PORTABLE,
                                                                   RoomField::PORTABLE_TYPE),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomPortableType::PORTABLE,
                                                                        RoomField::PORTABLE_TYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomPortableType::PORTABLE,
                                                                   RoomField::PORTABLE_TYPE),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomPortableType::UNDEFINED,
                                                                        RoomField::PORTABLE_TYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomPortableType::UNDEFINED,
                                                                   RoomField::PORTABLE_TYPE),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomRidableType::NOT_RIDABLE,
                                                                        RoomField::RIDABLE_TYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomRidableType::NOT_RIDABLE,
                                                                   RoomField::RIDABLE_TYPE),
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
        if (const Room *r = getSelectedRoom()) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomRidableType::RIDABLE,
                                                                        RoomField::RIDABLE_TYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomRidableType::RIDABLE,
                                                                   RoomField::RIDABLE_TYPE),
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
        if (const Room *r = getSelectedRoom()) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomRidableType::UNDEFINED,
                                                                        RoomField::RIDABLE_TYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomRidableType::UNDEFINED,
                                                                   RoomField::RIDABLE_TYPE),
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
        if (const Room *r = getSelectedRoom()) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomLightType::LIT,
                                                                        RoomField::LIGHT_TYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomLightType::LIT,
                                                                   RoomField::LIGHT_TYPE),
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
        if (const Room *r = getSelectedRoom()) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomLightType::DARK,
                                                                        RoomField::LIGHT_TYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomLightType::DARK,
                                                                   RoomField::LIGHT_TYPE),
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
        if (const Room *r = getSelectedRoom()) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomLightType::UNDEFINED,
                                                                        RoomField::LIGHT_TYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomLightType::UNDEFINED,
                                                                   RoomField::LIGHT_TYPE),
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
        if (const Room *r = getSelectedRoom()) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomSundeathType::SUNDEATH,
                                                                        RoomField::SUNDEATH_TYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomSundeathType::SUNDEATH,
                                                                   RoomField::SUNDEATH_TYPE),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomSundeathType::NO_SUNDEATH,
                                                                        RoomField::SUNDEATH_TYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomSundeathType::NO_SUNDEATH,
                                                                   RoomField::SUNDEATH_TYPE),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomSundeathType::UNDEFINED,
                                                                        RoomField::SUNDEATH_TYPE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new UpdateRoomField(RoomSundeathType::UNDEFINED,
                                                                   RoomField::SUNDEATH_TYPE),
                                               m_roomSelection),
                               m_roomSelection);
        }

        emit mapChanged();
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::mobFlagsListItemChanged(QListWidgetItem *const item)
{
    deref(item);
    RoomMobFlag flag{};
    if (!mobListItems.findIndexOf(item, flag)) {
        qWarning() << "oops" << __FILE__ << ":" << __LINE__;
        return;
    }

    const auto flags = RoomMobFlags{flag};
    const Room *r = getSelectedRoom();

    switch (item->checkState()) {
    case Qt::Unchecked:
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags(flags.asUint32(),
                                                                        RoomField::MOB_FLAGS,
                                                                        FlagModifyMode::UNSET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new ModifyRoomFlags(flags.asUint32(),
                                                                   RoomField::MOB_FLAGS,
                                                                   FlagModifyMode::UNSET),
                                               m_roomSelection),
                               m_roomSelection);
        }
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags(flags.asUint32(),
                                                                        RoomField::MOB_FLAGS,
                                                                        FlagModifyMode::SET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new ModifyRoomFlags(flags.asUint32(),
                                                                   RoomField::MOB_FLAGS,
                                                                   FlagModifyMode::SET),
                                               m_roomSelection),
                               m_roomSelection);
        }
        break;
    }

    emit mapChanged();
}

void RoomEditAttrDlg::loadFlagsListItemChanged(QListWidgetItem *const item)
{
    deref(item);
    RoomLoadFlag flag{};
    if (!loadListItems.findIndexOf(item, flag)) {
        qWarning() << "oops: " << __FILE__ << ":" << __LINE__;
        return;
    }

    const auto flags = RoomLoadFlags{flag};
    const Room *const r = getSelectedRoom();

    switch (item->checkState()) {
    case Qt::Unchecked:
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags(flags.asUint32(),
                                                                        RoomField::LOAD_FLAGS,
                                                                        FlagModifyMode::UNSET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new ModifyRoomFlags(flags.asUint32(),
                                                                   RoomField::LOAD_FLAGS,
                                                                   FlagModifyMode::UNSET),
                                               m_roomSelection),
                               m_roomSelection);
        }
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags(flags.asUint32(),
                                                                        RoomField::LOAD_FLAGS,
                                                                        FlagModifyMode::SET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new ModifyRoomFlags(flags.asUint32(),
                                                                   RoomField::LOAD_FLAGS,
                                                                   FlagModifyMode::SET),
                                               m_roomSelection),
                               m_roomSelection);
        }
        break;
    }

    emit mapChanged();
}

void RoomEditAttrDlg::exitFlagsListItemChanged(QListWidgetItem *const item)
{
    deref(item);
    ExitFlag flag{};
    if (!exitListItems.findIndexOf(item, flag)) {
        qWarning() << "oops: " << __FILE__ << ":" << __LINE__;
        return;
    }

    const Room *const r = getSelectedRoom();
    const auto flags = ExitFlags{flag};
    auto dir = getSelectedExit();
    switch (item->checkState()) {
    case Qt::Unchecked: {
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyExitFlags(flags,
                                                                        dir,
                                                                        FlagModifyMode::UNSET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new ModifyExitFlags(flags, dir, FlagModifyMode::UNSET),
                                               m_roomSelection),
                               m_roomSelection);
        }
    } break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyExitFlags(flags,
                                                                        dir,
                                                                        FlagModifyMode::SET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupAction(new ModifyExitFlags(flags, dir, FlagModifyMode::SET),
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
    const Room *const r = getSelectedRoom();

    const auto doorName = static_cast<DoorName>(doorNameLineEdit->text());
    m_mapData->execute(new SingleRoomAction(new UpdateExitField(doorName, getSelectedExit()),
                                            r->getId()),
                       m_roomSelection);
}

void RoomEditAttrDlg::doorFlagsListItemChanged(QListWidgetItem *const item)
{
    deref(item);
    DoorFlag flag{};
    if (!doorListItems.findIndexOf(item, flag)) {
        qWarning() << "oops: " << __FILE__ << ":" << __LINE__;
        return;
    }

    const Room *const r = getSelectedRoom();
    deref(r);

    const auto flags = DoorFlags{flag};
    const auto dir = getSelectedExit();
    switch (item->checkState()) {
    case Qt::Unchecked:
        m_mapData->execute(new SingleRoomAction(new ModifyExitFlags(flags,
                                                                    dir,
                                                                    FlagModifyMode::UNSET),
                                                r->getId()),
                           m_roomSelection);
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        m_mapData->execute(new SingleRoomAction(new ModifyExitFlags(flags, dir, FlagModifyMode::SET),
                                                r->getId()),
                           m_roomSelection);
        break;
    }

    emit mapChanged();
}

void RoomEditAttrDlg::toggleHiddenDoor()
{
    if (doorFlagsListWidget->isEnabled()) {
        doorListItems[DoorFlag::HIDDEN]->setCheckState(
            doorListItems[DoorFlag::HIDDEN]->checkState() == Qt::Unchecked ? Qt::Checked
                                                                           : Qt::Unchecked);
    }
}

//terrain tab
void RoomEditAttrDlg::terrainToolButtonToggled(bool val)
{
    if (!val)
        return;

    const Room *const r = getSelectedRoom();

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

    /* WARNING: there are more than 16 room terrain types */
    const auto rtt = static_cast<RoomTerrainType>(index);

    terrainLabel->setPixmap(QPixmap(QString(":/pixmaps/terrain%1.png").arg(index)));

    if (r != nullptr) {
        m_mapData->execute(new SingleRoomAction(new UpdateRoomField(rtt, RoomField::TERRAIN_TYPE),
                                                r->getId()),
                           m_roomSelection);
    } else {
        m_mapData->execute(new GroupAction(new UpdateRoomField(rtt, RoomField::TERRAIN_TYPE),
                                           m_roomSelection),
                           m_roomSelection);
    }

    emit mapChanged();
}

//note tab
void RoomEditAttrDlg::roomNoteChanged()
{
    const Room *r = getSelectedRoom();

    const QString note = roomNoteTextEdit->document()->toPlainText();
    if (r != nullptr) {
        m_mapData->execute(new SingleRoomAction(new UpdateRoomField(note, RoomField::NOTE),
                                                r->getId()),
                           m_roomSelection);
    } else {
        m_mapData->execute(new GroupAction(new UpdateRoomField(note, RoomField::NOTE),
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
