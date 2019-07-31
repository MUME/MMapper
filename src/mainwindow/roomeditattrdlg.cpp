// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "roomeditattrdlg.h"

#include <cstddef>
#include <map>
#include <QMessageLogContext>
#include <QString>
#include <QVariant>
#include <QtGui>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "../display/Filenames.h"
#include "../display/mapcanvas.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../global/utils.h"
#include "../mapdata/ExitFieldVariant.h"
#include "../mapdata/RoomFieldVariant.h"
#include "../mapdata/customaction.h"
#include "../mapdata/enums.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/mmapper2room.h"
#include "../mapdata/roomselection.h"
#include "../mapfrontend/mapaction.h"

template<typename T>
void fixMissing(T &array, const char *name)
{
    for (auto &x : array) { // reference to pointer so we can add missing elements
        if (x != nullptr)
            continue;
        const auto ordinal = static_cast<int>(&x - array.data());
        x = new RoomListWidgetItem(QString::asprintf("%d", ordinal));
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
    for (RoomListWidgetItem *const x : array) {
        x->setFlags(flags);
        widget.addItem(checked_static_upcast<QListWidgetItem *>(x));
    }
}

/* TODO: merge this with human-readable names used in parser output */
static QString getName(const RoomMobFlagEnum flag)
{
#define CASE2(UPPER, desc) \
    do { \
    case RoomMobFlagEnum::UPPER: \
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
        CASE2(AGGRESSIVE_MOB, "Aggressive mob");
        CASE2(QUEST_MOB, "Quest mob");
        CASE2(PASSIVE_MOB, "Passive mob");
        CASE2(ELITE_MOB, "Elite mob");
        CASE2(SUPER_MOB, "Super mob");
    }
    return QString::asprintf("(RoomMobFlagEnum)%d", static_cast<int>(flag));
#undef CASE2
}

/* TODO: merge this with human-readable names used in parser output */
static QString getName(const RoomLoadFlagEnum flag)
{
#define CASE2(UPPER, desc) \
    do { \
    case RoomLoadFlagEnum::UPPER: \
        return desc; \
    } while (false)
    switch (flag) {
        CASE2(TREASURE, "Treasure");
        CASE2(ARMOUR, "Armour");
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
        CASE2(WHITE_WORD, "White word");
        CASE2(DARK_WORD, "Dark word");
        CASE2(EQUIPMENT, "Equipment");
        CASE2(COACH, "Coach");
        CASE2(FERRY, "Ferry");
    }
    return QString::asprintf("(RoomLoadFlagEnum)%d", static_cast<int>(flag));
#undef CASE2
}

/* TODO: merge this with human-readable names used in parser output */
static QString getName(const ExitFlagEnum flag)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case ExitFlagEnum::UPPER_CASE: \
        return friendly; \
    } while (false);
    switch (flag) {
        X_FOREACH_EXIT_FLAG(X_CASE)
    }
    return QString::asprintf("(ExitFlagEnum)%d", static_cast<int>(flag));
#undef X_CASE
}

/* TODO: merge this with human-readable names used in parser output */
static QString getName(const DoorFlagEnum flag)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case DoorFlagEnum::UPPER_CASE: \
        return friendly; \
    } while (false);
    switch (flag) {
        X_FOREACH_DOOR_FLAG(X_CASE)
    }
    return QString::asprintf("(DoorFlagEnum)%d", static_cast<int>(flag));
#undef X_CASE
}

template<typename T>
static QIcon getIcon(T flag)
{
    const QString filename = getPixmapFilename(flag);
    try {
        QIcon result(filename);
        if (result.isNull())
            throw std::runtime_error(
                QString("failed to load icon '%1'").arg(filename).toStdString());
        return result;
    } catch (...) {
        qWarning() << "Oops: Unable to create icon:" << filename;
        throw;
    }
}

static int getPriority(const RoomMobFlagEnum flag)
{
#define X_POS(UPPER, pos) \
    do { \
        if (flag == RoomMobFlagEnum::UPPER) \
            return NUM_ROOM_MOB_FLAGS * -1 + pos; \
    } while (false)
    X_POS(PASSIVE_MOB, 0);
    X_POS(AGGRESSIVE_MOB, 1);
    X_POS(ELITE_MOB, 2);
    X_POS(SUPER_MOB, 3);
    X_POS(QUEST_MOB, 4);
#undef X_POS
    return static_cast<int>(flag);
}

static int getPriority(const RoomLoadFlagEnum flag)
{
#define X_POS(UPPER, pos) \
    do { \
        if (flag == RoomLoadFlagEnum::UPPER) \
            return NUM_ROOM_LOAD_FLAGS * -1 + pos; \
    } while (false)
    X_POS(TREASURE, 0);
    X_POS(ARMOUR, 1);
    X_POS(WEAPON, 2);
    X_POS(EQUIPMENT, 3);
#undef X_POS
    return static_cast<int>(flag);
}

RoomListWidgetItem::RoomListWidgetItem(const QString &text, const int priority)
    : QListWidgetItem(text)
{
    setData(Qt::UserRole, priority);
}

RoomListWidgetItem::RoomListWidgetItem(const QIcon &icon, const QString &text, const int priority)
    : QListWidgetItem(icon, text)
{
    setData(Qt::UserRole, priority);
}

bool RoomListWidgetItem::operator<(const QListWidgetItem &other) const
{
    // Sort on user role for priority as opposed to text
    return data(Qt::UserRole).toInt() < other.data(Qt::UserRole).toInt();
}

RoomEditAttrDlg::RoomEditAttrDlg(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    roomDescriptionTextEdit->setLineWrapMode(QTextEdit::NoWrap);

    for (const auto flag : ALL_MOB_FLAGS)
        mobListItems[flag] = new RoomListWidgetItem(getIcon(flag), getName(flag), getPriority(flag));
    installWidgets(mobListItems,
                   "mob room flags",
                   *mobFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsTristate);

    for (const auto flag : ALL_LOAD_FLAGS)
        loadListItems[flag] = new RoomListWidgetItem(getIcon(flag),
                                                     getName(flag),
                                                     getPriority(flag));
    installWidgets(loadListItems,
                   "load list",
                   *loadFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsTristate);

    for (const auto flag : ALL_EXIT_FLAGS)
        exitListItems[flag] = new RoomListWidgetItem(getName(flag));
    installWidgets(exitListItems,
                   "exit list",
                   *exitFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

    for (const auto flag : ALL_DOOR_FLAGS)
        doorListItems[flag] = new RoomListWidgetItem(getName(flag));
    installWidgets(doorListItems,
                   "door list",
                   *doorFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

    m_hiddenShortcut = new QShortcut(QKeySequence(tr("Ctrl+H", "Room edit > hidden flag")), this);

    updatedCheckBox->setCheckable(false);
    updatedCheckBox->setText("Room has not been online updated yet!!!");

    readSettings();

    connect(closeButton, &QAbstractButton::clicked, this, &RoomEditAttrDlg::closeClicked);
}

RoomEditAttrDlg::~RoomEditAttrDlg()
{
    writeSettings();
}

void RoomEditAttrDlg::readSettings()
{
    restoreGeometry(getConfig().roomEditDialog.geometry);
}

void RoomEditAttrDlg::writeSettings()
{
    setConfig().roomEditDialog.geometry = saveGeometry();
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

    connect(roomListComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &RoomEditAttrDlg::roomListCurrentIndexChanged);

    connect(updatedCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyRoomUpToDate(checked), r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new ModifyRoomUpToDate(checked), m_roomSelection),
                               m_roomSelection);
        }
        if (checked) {
            updatedCheckBox->setText("Room has been forced updated.");
        } else {
            updatedCheckBox->setText("Room has been forced outdated.");
        }
        emit mapChanged();
    });
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

    disconnect(roomListComboBox,
               QOverload<int>::of(&QComboBox::currentIndexChanged),
               this,
               &RoomEditAttrDlg::roomListCurrentIndexChanged);
}

const Room *RoomEditAttrDlg::getSelectedRoom()
{
    if (m_roomSelection == nullptr || m_roomSelection->empty()) {
        return nullptr;
    }
    if (m_roomSelection->size() == 1) {
        return m_roomSelection->first();
    }
    return m_roomSelection->value(
        RoomId{roomListComboBox->itemData(roomListComboBox->currentIndex()).toUInt()});
}

ExitDirEnum RoomEditAttrDlg::getSelectedExit()
{
    if (exitNButton->isChecked()) {
        return ExitDirEnum::NORTH;
    }
    if (exitSButton->isChecked()) {
        return ExitDirEnum::SOUTH;
    }
    if (exitEButton->isChecked()) {
        return ExitDirEnum::EAST;
    }
    if (exitWButton->isChecked()) {
        return ExitDirEnum::WEST;
    }
    if (exitUButton->isChecked()) {
        return ExitDirEnum::UP;
    }
    if (exitDButton->isChecked()) {
        return ExitDirEnum::DOWN;
    }
    return ExitDirEnum::UNKNOWN;
}

void RoomEditAttrDlg::roomListCurrentIndexChanged(int /*unused*/)
{
    updateDialog(getSelectedRoom());
}

void RoomEditAttrDlg::setRoomSelection(const SharedRoomSelection &rs,
                                       MapData *const md,
                                       MapCanvas *const mc)
{
    m_roomSelection = rs;
    m_mapData = md;
    m_mapCanvas = mc;

    roomListComboBox->clear();

    if (rs == nullptr)
        return;
    else if (rs->size() == 1) {
        tabWidget->setCurrentWidget(attributesTab);
        const auto room = m_roomSelection->first();
        roomListComboBox->addItem(room->getName(), room->getId().asUint32());
        updateDialog(room);
    } else {
        tabWidget->setCurrentWidget(selectionTab);
        roomListComboBox->addItem("All", 0);
        for (const Room *room : *m_roomSelection) {
            roomListComboBox->addItem(room->getName(), room->getId().asUint32());
        }
        updateDialog(nullptr);
    }

    connect(this,
            &RoomEditAttrDlg::mapChanged,
            m_mapCanvas,
            static_cast<void (QWidget::*)(void)>(&QWidget::update));
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
    struct NODISCARD DisconnectReconnectAntiPattern final
    {
        RoomEditAttrDlg &self;
        explicit DisconnectReconnectAntiPattern(RoomEditAttrDlg &self)
            : self{self}
        {
            self.disconnectAll();
        }
        ~DisconnectReconnectAntiPattern() { self.connectAll(); }
    } antiPattern{*this};

    if (r == nullptr) {
        roomDescriptionTextEdit->clear();
        roomDescriptionTextEdit->setEnabled(false);

        updatedCheckBox->setCheckable(false);
        updatedCheckBox->setText("");

        roomNoteTextEdit->clear();
        roomNoteTextEdit->setEnabled(false);

        terrainLabel->setPixmap(QPixmap(getPixmapFilename(RoomTerrainEnum::UNDEFINED)));

        exitsFrame->setEnabled(false);

        // REVISIT: Check state of all entries and set it if they all match that state
        rideUndefRadioButton->setChecked(true);
        alignUndefRadioButton->setChecked(true);
        portUndefRadioButton->setChecked(true);
        lightUndefRadioButton->setChecked(true);
        sundeathUndefRadioButton->setChecked(true);

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

        updatedCheckBox->setCheckable(true);
        updatedCheckBox->setChecked(r->isUpToDate());
        if (r->isUpToDate()) {
            updatedCheckBox->setText("Room has been online updated.");
        } else {
            updatedCheckBox->setText("Room has not been online updated yet!!!");
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

        const auto get_terrain_pixmap = [](RoomTerrainEnum type) -> QString {
            if (type == RoomTerrainEnum::ROAD)
                return getPixmapFilename(TaggedRoad{
                    RoadIndexMaskEnum::NORTH | RoadIndexMaskEnum::EAST | RoadIndexMaskEnum::SOUTH});
            else
                return getPixmapFilename(type);
        };
        terrainLabel->setPixmap(get_terrain_pixmap(r->getTerrainType()));

        switch (r->getAlignType()) {
        case RoomAlignEnum::GOOD:
            goodRadioButton->setChecked(true);
            break;
        case RoomAlignEnum::NEUTRAL:
            neutralRadioButton->setChecked(true);
            break;
        case RoomAlignEnum::EVIL:
            evilRadioButton->setChecked(true);
            break;
        case RoomAlignEnum::UNDEFINED:
            alignUndefRadioButton->setChecked(true);
            break;
        }

        switch (r->getPortableType()) {
        case RoomPortableEnum::PORTABLE:
            portableRadioButton->setChecked(true);
            break;
        case RoomPortableEnum::NOT_PORTABLE:
            noPortRadioButton->setChecked(true);
            break;
        case RoomPortableEnum::UNDEFINED:
            portUndefRadioButton->setChecked(true);
            break;
        }

        switch (r->getRidableType()) {
        case RoomRidableEnum::RIDABLE:
            ridableRadioButton->setChecked(true);
            break;
        case RoomRidableEnum::NOT_RIDABLE:
            noRideRadioButton->setChecked(true);
            break;
        case RoomRidableEnum::UNDEFINED:
            rideUndefRadioButton->setChecked(true);
            break;
        }

        switch (r->getLightType()) {
        case RoomLightEnum::DARK:
            darkRadioButton->setChecked(true);
            break;
        case RoomLightEnum::LIT:
            litRadioButton->setChecked(true);
            break;
        case RoomLightEnum::UNDEFINED:
            lightUndefRadioButton->setChecked(true);
            break;
        }

        switch (r->getSundeathType()) {
        case RoomSundeathEnum::NO_SUNDEATH:
            noSundeathRadioButton->setChecked(true);
            break;
        case RoomSundeathEnum::SUNDEATH:
            sundeathRadioButton->setChecked(true);
            break;
        case RoomSundeathEnum::UNDEFINED:
            sundeathUndefRadioButton->setChecked(true);
            break;
        }

        switch (r->getTerrainType()) {
        case RoomTerrainEnum::UNDEFINED:
            toolButton00->setChecked(true);
            break;
        case RoomTerrainEnum::INDOORS:
            toolButton01->setChecked(true);
            break;
        case RoomTerrainEnum::CITY:
            toolButton02->setChecked(true);
            break;
        case RoomTerrainEnum::FIELD:
            toolButton03->setChecked(true);
            break;
        case RoomTerrainEnum::FOREST:
            toolButton04->setChecked(true);
            break;
        case RoomTerrainEnum::HILLS:
            toolButton05->setChecked(true);
            break;
        case RoomTerrainEnum::MOUNTAINS:
            toolButton06->setChecked(true);
            break;
        case RoomTerrainEnum::SHALLOW:
            toolButton07->setChecked(true);
            break;
        case RoomTerrainEnum::WATER:
            toolButton08->setChecked(true);
            break;
        case RoomTerrainEnum::RAPIDS:
            toolButton09->setChecked(true);
            break;
        case RoomTerrainEnum::UNDERWATER:
            toolButton10->setChecked(true);
            break;
        case RoomTerrainEnum::ROAD:
            toolButton11->setChecked(true);
            break;
        case RoomTerrainEnum::BRUSH:
            toolButton12->setChecked(true);
            break;
        case RoomTerrainEnum::TUNNEL:
            toolButton13->setChecked(true);
            break;
        case RoomTerrainEnum::CAVERN:
            toolButton14->setChecked(true);
            break;
        case RoomTerrainEnum::DEATHTRAP:
            toolButton15->setChecked(true);
            break;
        }
    }
}

// attributes page
void RoomEditAttrDlg::exitButtonToggled(bool /*unused*/)
{
    updateDialog(getSelectedRoom());
}

void RoomEditAttrDlg::neutralRadioButtonToggled(bool val)
{
    if (val) {
        const Room *r = getSelectedRoom();
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomAlignEnum::NEUTRAL),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(RoomAlignEnum::NEUTRAL),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomAlignEnum::GOOD),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(RoomAlignEnum::GOOD),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomAlignEnum::EVIL),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(RoomAlignEnum::EVIL),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomAlignEnum::UNDEFINED),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(RoomAlignEnum::UNDEFINED),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(
                                                        RoomPortableEnum::NOT_PORTABLE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(
                                                      RoomPortableEnum::NOT_PORTABLE),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomPortableEnum::PORTABLE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(RoomPortableEnum::PORTABLE),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomPortableEnum::UNDEFINED),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(RoomPortableEnum::UNDEFINED),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(
                                                        RoomRidableEnum::NOT_RIDABLE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(RoomRidableEnum::NOT_RIDABLE),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomRidableEnum::RIDABLE),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(RoomRidableEnum::RIDABLE),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomRidableEnum::UNDEFINED),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(RoomRidableEnum::UNDEFINED),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomLightEnum::LIT),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(RoomLightEnum::LIT),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomLightEnum::DARK),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(RoomLightEnum::DARK),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomLightEnum::UNDEFINED),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(RoomLightEnum::UNDEFINED),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomSundeathEnum::SUNDEATH),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(RoomSundeathEnum::SUNDEATH),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(
                                                        RoomSundeathEnum::NO_SUNDEATH),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(RoomSundeathEnum::NO_SUNDEATH),
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
            m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RoomSundeathEnum::UNDEFINED),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new UpdateRoomField(RoomSundeathEnum::UNDEFINED),
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
    const auto optFlag = mobListItems.findIndexOf(
        checked_dynamic_downcast<RoomListWidgetItem *>(item));
    if (!optFlag) {
        qWarning() << "oops" << __FILE__ << ":" << __LINE__;
        return;
    }

    const RoomMobFlagEnum flag = optFlag.value();
    const auto flags = RoomMobFlags{flag};
    const Room *const r = getSelectedRoom();

    switch (item->checkState()) {
    case Qt::Unchecked:
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags(flags,
                                                                        FlagModifyModeEnum::UNSET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new ModifyRoomFlags(flags,
                                                                      FlagModifyModeEnum::UNSET),
                                                  m_roomSelection),
                               m_roomSelection);
        }
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags(flags,
                                                                        FlagModifyModeEnum::SET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new ModifyRoomFlags(flags,
                                                                      FlagModifyModeEnum::SET),
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

    const auto optFlag = loadListItems.findIndexOf(
        checked_dynamic_downcast<RoomListWidgetItem *>(item));
    if (!optFlag) {
        qWarning() << "oops: " << __FILE__ << ":" << __LINE__;
        return;
    }

    const RoomLoadFlagEnum flag = optFlag.value();
    const auto flags = RoomLoadFlags{flag};
    const Room *const r = getSelectedRoom();

    switch (item->checkState()) {
    case Qt::Unchecked:
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags(flags,
                                                                        FlagModifyModeEnum::UNSET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new ModifyRoomFlags(flags,
                                                                      FlagModifyModeEnum::UNSET),
                                                  m_roomSelection),
                               m_roomSelection);
        }
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags(flags,
                                                                        FlagModifyModeEnum::SET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new ModifyRoomFlags(flags,
                                                                      FlagModifyModeEnum::SET),
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

    const auto optFlag = exitListItems.findIndexOf(
        checked_dynamic_downcast<RoomListWidgetItem *>(item));
    if (!optFlag) {
        qWarning() << "oops: " << __FILE__ << ":" << __LINE__;
        return;
    }

    const ExitFlagEnum flag = optFlag.value();
    const Room *const r = getSelectedRoom();
    const auto flags = ExitFlags{flag};
    auto dir = getSelectedExit();
    switch (item->checkState()) {
    case Qt::Unchecked: {
        if (r != nullptr) {
            m_mapData->execute(new SingleRoomAction(new ModifyExitFlags(flags,
                                                                        dir,
                                                                        FlagModifyModeEnum::UNSET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new ModifyExitFlags(flags,
                                                                      dir,
                                                                      FlagModifyModeEnum::UNSET),
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
                                                                        FlagModifyModeEnum::SET),
                                                    r->getId()),
                               m_roomSelection);
        } else {
            m_mapData->execute(new GroupMapAction(new ModifyExitFlags(flags,
                                                                      dir,
                                                                      FlagModifyModeEnum::SET),
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

    const auto optFlag = doorListItems.findIndexOf(
        checked_dynamic_downcast<RoomListWidgetItem *>(item));
    if (!optFlag) {
        qWarning() << "oops: " << __FILE__ << ":" << __LINE__;
        return;
    }

    const DoorFlagEnum flag = optFlag.value();
    const Room *const r = getSelectedRoom();
    deref(r);

    const auto flags = DoorFlags{flag};
    const auto dir = getSelectedExit();
    switch (item->checkState()) {
    case Qt::Unchecked:
        m_mapData->execute(new SingleRoomAction(new ModifyExitFlags(flags,
                                                                    dir,
                                                                    FlagModifyModeEnum::UNSET),
                                                r->getId()),
                           m_roomSelection);
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        m_mapData->execute(new SingleRoomAction(new ModifyExitFlags(flags,
                                                                    dir,
                                                                    FlagModifyModeEnum::SET),
                                                r->getId()),
                           m_roomSelection);
        break;
    }

    emit mapChanged();
}

void RoomEditAttrDlg::toggleHiddenDoor()
{
    if (doorFlagsListWidget->isEnabled()) {
        doorListItems[DoorFlagEnum::HIDDEN]->setCheckState(
            doorListItems[DoorFlagEnum::HIDDEN]->checkState() == Qt::Unchecked ? Qt::Checked
                                                                               : Qt::Unchecked);
    }
}

// terrain tab
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
    const auto rtt = static_cast<RoomTerrainEnum>(index);

    terrainLabel->setPixmap(QPixmap(getPixmapFilename(rtt)));

    if (r != nullptr) {
        m_mapData->execute(new SingleRoomAction(new UpdateRoomField(rtt), r->getId()),
                           m_roomSelection);
    } else {
        m_mapData->execute(new GroupMapAction(new UpdateRoomField(rtt), m_roomSelection),
                           m_roomSelection);
    }

    emit mapChanged();
}

// note tab
void RoomEditAttrDlg::roomNoteChanged()
{
    const Room *r = getSelectedRoom();

    const RoomNote note = roomNoteTextEdit->document()->toPlainText();
    if (r != nullptr) {
        m_mapData->execute(new SingleRoomAction(new UpdateRoomField(note), r->getId()),
                           m_roomSelection);
    } else {
        m_mapData->execute(new GroupMapAction(new UpdateRoomField(note), m_roomSelection),
                           m_roomSelection);
    }

    emit mapChanged();
}

// all tabs
void RoomEditAttrDlg::closeClicked()
{
    accept();
}
