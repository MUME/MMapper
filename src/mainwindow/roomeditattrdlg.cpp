// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "roomeditattrdlg.h"

#include <cstddef>
#include <map>
#include <memory>
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
#include "../global/SignalBlocker.h"
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
void fixMissing(T &array, const char *const name)
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
                    const char *const name,
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
NODISCARD static QString getName(const RoomMobFlagEnum flag)
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
        CASE2(MILKABLE, "Milkable mob");
    }
    return QString::asprintf("(RoomMobFlagEnum)%d", static_cast<int>(flag));
#undef CASE2
}

/* TODO: merge this with human-readable names used in parser output */
NODISCARD static QString getName(const RoomLoadFlagEnum flag)
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
NODISCARD static QString getName(const ExitFlagEnum flag)
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
NODISCARD static QString getName(const DoorFlagEnum flag)
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
NODISCARD static QIcon getIcon(T flag)
{
    const QString filename = getPixmapFilename(flag);
    try {
        QIcon result(filename);
        if (result.isNull())
            throw std::runtime_error(
                ::toStdStringUtf8(QString("failed to load icon '%1'").arg(filename)));
        return result;
    } catch (...) {
        qWarning() << "Oops: Unable to create icon:" << filename;
        throw;
    }
}

NODISCARD static int getPriority(const RoomMobFlagEnum flag)
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

NODISCARD static int getPriority(const RoomLoadFlagEnum flag)
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
#define NUM_ELEMENTS(arr) (decltype(arr)::SIZE)
    static_assert(NUM_ELEMENTS(loadListItems) <= 32u);
    static_assert(NUM_ELEMENTS(mobListItems) <= 32u);
    static_assert(NUM_ELEMENTS(exitListItems) <= 16u);
    static_assert(NUM_ELEMENTS(doorListItems) <= 16u);
    static_assert(NUM_ELEMENTS(roomTerrainButtons) == NUM_ROOM_TERRAIN_TYPES);
    static_assert(NUM_ROOM_TERRAIN_TYPES == 16);
#undef NUM_ELEMENTS

    setupUi(this);

    // NOTE: Another option would be to just initialize them all directly here,
    // and then get rid of have getTerrainToolButton() index into the array,
    // or get rid of the function entirely.
    for (size_t i = 0; i < NUM_ROOM_TERRAIN_TYPES; ++i) {
        const auto rtt = static_cast<RoomTerrainEnum>(i);
        roomTerrainButtons[rtt] = getTerrainToolButton(rtt);
    }

    roomDescriptionTextEdit->setLineWrapMode(QTextEdit::NoWrap);

    for (const RoomMobFlagEnum flag : ALL_MOB_FLAGS)
        mobListItems[flag] = new RoomListWidgetItem(getIcon(flag), getName(flag), getPriority(flag));
    installWidgets(mobListItems,
                   "mob room flags",
                   *mobFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsAutoTristate);

    for (const RoomLoadFlagEnum flag : ALL_LOAD_FLAGS)
        loadListItems[flag] = new RoomListWidgetItem(getIcon(flag),
                                                     getName(flag),
                                                     getPriority(flag));
    installWidgets(loadListItems,
                   "load list",
                   *loadFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsAutoTristate);

    for (const ExitFlagEnum flag : ALL_EXIT_FLAGS)
        exitListItems[flag] = new RoomListWidgetItem(getName(flag));
    installWidgets(exitListItems,
                   "exit list",
                   *exitFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

    for (const DoorFlagEnum flag : ALL_DOOR_FLAGS)
        doorListItems[flag] = new RoomListWidgetItem(getName(flag));
    installWidgets(doorListItems,
                   "door list",
                   *doorFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

    m_hiddenShortcut = std::make_unique<QShortcut>(QKeySequence(
                                                       tr("Ctrl+H", "Room edit > hidden flag")),
                                                   this);

    updatedCheckBox->setCheckable(false);
    updatedCheckBox->setText("Room has not been online updated yet!!!");

    readSettings();

    connect(closeButton, &QAbstractButton::clicked, this, &RoomEditAttrDlg::closeClicked);
}

RoomEditAttrDlg::~RoomEditAttrDlg()
{
    writeSettings();

    for (auto &x : mobListItems)
        delete std::exchange(x, nullptr);
    for (auto &x : loadListItems)
        delete std::exchange(x, nullptr);
    for (auto &x : exitListItems)
        delete std::exchange(x, nullptr);
    for (auto &x : doorListItems)
        delete std::exchange(x, nullptr);
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
    // TODO: If we're going to insist on using the connect/disconnect antipattern,
    // then let's at least turn these into X-macros.
    m_connections += connect(neutralRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::neutralRadioButtonToggled);
    m_connections += connect(goodRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::goodRadioButtonToggled);
    m_connections += connect(evilRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::evilRadioButtonToggled);
    m_connections += connect(alignUndefRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::alignUndefRadioButtonToggled);

    m_connections += connect(noPortRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::noPortRadioButtonToggled);
    m_connections += connect(portableRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::portableRadioButtonToggled);
    m_connections += connect(portUndefRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::portUndefRadioButtonToggled);

    m_connections += connect(noRideRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::noRideRadioButtonToggled);
    m_connections += connect(ridableRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::ridableRadioButtonToggled);
    m_connections += connect(rideUndefRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::rideUndefRadioButtonToggled);

    m_connections += connect(litRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::litRadioButtonToggled);
    m_connections += connect(darkRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::darkRadioButtonToggled);
    m_connections += connect(lightUndefRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::lightUndefRadioButtonToggled);

    m_connections += connect(sundeathRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::sundeathRadioButtonToggled);
    m_connections += connect(noSundeathRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::noSundeathRadioButtonToggled);
    m_connections += connect(sundeathUndefRadioButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::sundeathUndefRadioButtonToggled);

    m_connections += connect(mobFlagsListWidget,
                             &QListWidget::itemChanged,
                             this,
                             &RoomEditAttrDlg::mobFlagsListItemChanged);
    m_connections += connect(loadFlagsListWidget,
                             &QListWidget::itemChanged,
                             this,
                             &RoomEditAttrDlg::loadFlagsListItemChanged);

    m_connections += connect(exitNButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::exitButtonToggled);
    m_connections += connect(exitSButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::exitButtonToggled);
    m_connections += connect(exitEButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::exitButtonToggled);
    m_connections += connect(exitWButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::exitButtonToggled);
    m_connections += connect(exitUButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::exitButtonToggled);
    m_connections += connect(exitDButton,
                             &QAbstractButton::toggled,
                             this,
                             &RoomEditAttrDlg::exitButtonToggled);

    m_connections += connect(exitFlagsListWidget,
                             &QListWidget::itemChanged,
                             this,
                             &RoomEditAttrDlg::exitFlagsListItemChanged);
    m_connections += connect(doorFlagsListWidget,
                             &QListWidget::itemChanged,
                             this,
                             &RoomEditAttrDlg::doorFlagsListItemChanged);

    m_connections += connect(doorNameLineEdit,
                             &QLineEdit::editingFinished,
                             this,
                             &RoomEditAttrDlg::doorNameLineEditTextChanged);

    for (QToolButton *const toolButton : roomTerrainButtons)
        m_connections += connect(toolButton,
                                 &QAbstractButton::toggled,
                                 this,
                                 &RoomEditAttrDlg::terrainToolButtonToggled);

    m_connections += connect(roomNoteTextEdit,
                             &QTextEdit::textChanged,
                             this,
                             &RoomEditAttrDlg::roomNoteChanged);

    m_connections += connect(m_hiddenShortcut.get(),
                             &QShortcut::activated,
                             this,
                             &RoomEditAttrDlg::toggleHiddenDoor);

    m_connections += connect(roomListComboBox,
                             QOverload<int>::of(&QComboBox::currentIndexChanged),
                             this,
                             &RoomEditAttrDlg::roomListCurrentIndexChanged);

    m_connections += connect(updatedCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        updateCommon(std::make_unique<ModifyRoomUpToDate>(checked));
        if (checked) {
            updatedCheckBox->setText("Room has been forced updated.");
        } else {
            updatedCheckBox->setText("Room has been forced outdated.");
        }
    });
}

void RoomEditAttrDlg::disconnectAll()
{
    m_connections.disconnectAll(*this);
}

const Room *RoomEditAttrDlg::getSelectedRoom()
{
    if (m_roomSelection == nullptr || m_roomSelection->empty()) {
        return nullptr;
    }
    if (m_roomSelection->size() == 1) {
        return m_roomSelection->getFirstRoom();
    }
    auto it = m_roomSelection->find(
        RoomId{roomListComboBox->itemData(roomListComboBox->currentIndex()).toUInt()});
    if (it == m_roomSelection->end())
        return nullptr;
    return it->second;
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
        const auto room = m_roomSelection->getFirstRoom();
        roomListComboBox->addItem(room->getName().toQString(), room->getId().asUint32());
        updateDialog(room);
    } else {
        tabWidget->setCurrentWidget(selectionTab);
        roomListComboBox->addItem("All", 0);
        for (const auto &[rid, room] : *m_roomSelection) {
            roomListComboBox->addItem(room->getName().toQString(), room->getId().asUint32());
        }
        updateDialog(nullptr);
    }

    connect(this, &RoomEditAttrDlg::sig_requestUpdate, m_mapCanvas, &MapCanvas::slot_requestUpdate);
}

template<typename T, typename Flags>
void setCheckStates(T &array, const Flags &flags)
{
    for (size_t i = 0, len = array.size(); i < len; ++i) {
        const auto flag = static_cast<typename T::index_type>(i);
        if (auto *const x = array[flag]) {
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
        SignalBlocker signalBlocker;
        explicit DisconnectReconnectAntiPattern(RoomEditAttrDlg &self)
            : self{self}
            , signalBlocker{self}
        {
            self.disconnectAll();
        }
        ~DisconnectReconnectAntiPattern() { self.connectAll(); }
    } antiPattern{*this};

    if (r == nullptr) {
        roomDescriptionTextEdit->clear();
        roomDescriptionTextEdit->setEnabled(false);

        updatedCheckBox->setCheckable(true);
        updatedCheckBox->setText("Online update status has not been changed.");

        roomNoteTextEdit->clear();
        roomNoteTextEdit->setEnabled(false);

        terrainLabel->setPixmap(QPixmap(getPixmapFilename(RoomTerrainEnum::UNDEFINED)));

        exitsFrame->setEnabled(false);

        rideGroupBox->setChecked(false);
        alignGroupBox->setChecked(false);
        teleportGroupBox->setChecked(false);
        lightGroupBox->setChecked(false);
        sunGroupBox->setChecked(false);

        for (const auto &x : loadListItems) {
            x->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsAutoTristate);
            x->setCheckState(Qt::PartiallyChecked);
        }

        for (const auto &x : mobListItems) {
            x->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsAutoTristate);
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
            doorNameLineEdit->setText(e.getDoorName().toQString());
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
            QString str = r->getDescription().toQString();
            // note: older rooms may not have a trailing newline
            // REVISIT: what if they have \r\n?
            if (str.endsWith("\n"))
                str = str.left(str.length() - 1);
            roomDescriptionTextEdit->append(str);
        }
        roomDescriptionTextEdit->setFontItalic(true);
        roomDescriptionTextEdit->append(r->getContents().toQString());

        roomNoteTextEdit->clear();
        roomNoteTextEdit->append(r->getNote().toQString());

        const auto get_terrain_pixmap = [](RoomTerrainEnum type) -> QString {
            if (type == RoomTerrainEnum::ROAD)
                return getPixmapFilename(TaggedRoad{
                    RoadIndexMaskEnum::NORTH | RoadIndexMaskEnum::EAST | RoadIndexMaskEnum::SOUTH});
            else
                return getPixmapFilename(type);
        };
        terrainLabel->setPixmap(get_terrain_pixmap(r->getTerrainType()));

        if (auto *const button = getAlignRadioButton(r->getAlignType())) {
            button->setChecked(true);
        }
        if (auto *const button = getPortableRadioButton(r->getPortableType())) {
            button->setChecked(true);
        }
        if (auto *const button = getRideableRadioButton(r->getRidableType())) {
            button->setChecked(true);
        }
        if (auto *const button = getLightRadioButton(r->getLightType())) {
            button->setChecked(true);
        }
        if (auto *const button = getSundeathRadioButton(r->getSundeathType())) {
            button->setChecked(true);
        }
        if (auto button = getTerrainToolButton(r->getTerrainType())) {
            button->setChecked(true);
        }
    }
}

QRadioButton *RoomEditAttrDlg::getAlignRadioButton(const RoomAlignEnum value) const
{
    switch (value) {
    case RoomAlignEnum::GOOD:
        return goodRadioButton;
    case RoomAlignEnum::NEUTRAL:
        return neutralRadioButton;
    case RoomAlignEnum::EVIL:
        return evilRadioButton;
    case RoomAlignEnum::UNDEFINED:
        return alignUndefRadioButton;
    }
    return nullptr;
}

QRadioButton *RoomEditAttrDlg::getPortableRadioButton(const RoomPortableEnum value) const
{
    switch (value) {
    case RoomPortableEnum::PORTABLE:
        return portableRadioButton;
    case RoomPortableEnum::NOT_PORTABLE:
        return noPortRadioButton;
    case RoomPortableEnum::UNDEFINED:
        return portUndefRadioButton;
    }
    return nullptr;
}
QRadioButton *RoomEditAttrDlg::getRideableRadioButton(const RoomRidableEnum value) const
{
    switch (value) {
    case RoomRidableEnum::RIDABLE:
        return ridableRadioButton;
    case RoomRidableEnum::NOT_RIDABLE:
        return noRideRadioButton;
    case RoomRidableEnum::UNDEFINED:
        return rideUndefRadioButton;
    }
    return nullptr;
}
QRadioButton *RoomEditAttrDlg::getLightRadioButton(const RoomLightEnum value) const
{
    switch (value) {
    case RoomLightEnum::DARK:
        return darkRadioButton;
    case RoomLightEnum::LIT:
        return litRadioButton;
    case RoomLightEnum::UNDEFINED:
        return lightUndefRadioButton;
    }
    return nullptr;
}
QRadioButton *RoomEditAttrDlg::getSundeathRadioButton(const RoomSundeathEnum value) const
{
    switch (value) {
    case RoomSundeathEnum::NO_SUNDEATH:
        return noSundeathRadioButton;
    case RoomSundeathEnum::SUNDEATH:
        return sundeathRadioButton;
    case RoomSundeathEnum::UNDEFINED:
        return sundeathUndefRadioButton;
    }
    return nullptr;
}

QToolButton *RoomEditAttrDlg::getTerrainToolButton(const RoomTerrainEnum value) const
{
    switch (value) {
    case RoomTerrainEnum::UNDEFINED:
        return toolButton00;
    case RoomTerrainEnum::INDOORS:
        return toolButton01;
    case RoomTerrainEnum::CITY:
        return toolButton02;
    case RoomTerrainEnum::FIELD:
        return toolButton03;
    case RoomTerrainEnum::FOREST:
        return toolButton04;
    case RoomTerrainEnum::HILLS:
        return toolButton05;
    case RoomTerrainEnum::MOUNTAINS:
        return toolButton06;
    case RoomTerrainEnum::SHALLOW:
        return toolButton07;
    case RoomTerrainEnum::WATER:
        return toolButton08;
    case RoomTerrainEnum::RAPIDS:
        return toolButton09;
    case RoomTerrainEnum::UNDERWATER:
        return toolButton10;
    case RoomTerrainEnum::ROAD:
        return toolButton11;
    case RoomTerrainEnum::BRUSH:
        return toolButton12;
    case RoomTerrainEnum::TUNNEL:
        return toolButton13;
    case RoomTerrainEnum::CAVERN:
        return toolButton14;
    case RoomTerrainEnum::DEATHTRAP:
        return toolButton15;
    }

    return nullptr;
}

// attributes page
void RoomEditAttrDlg::exitButtonToggled(bool /*unused*/)
{
    updateDialog(getSelectedRoom());
}

void RoomEditAttrDlg::updateCommon(std::unique_ptr<AbstractAction> moved_action,
                                   bool onlyExecuteAction)
{
    if (const Room *const r = getSelectedRoom()) {
        m_mapData->execute(std::make_unique<SingleRoomAction>(std::exchange(moved_action, {}),
                                                              r->getId()),
                           m_roomSelection);
    } else {
        m_mapData->execute(std::make_unique<GroupMapAction>(std::exchange(moved_action, {}),
                                                            m_roomSelection),
                           m_roomSelection);
    }

    if (!onlyExecuteAction) {
        updateDialog(getSelectedRoom());
        emit sig_requestUpdate();
    }
}

void RoomEditAttrDlg::updateRoomAlign(const RoomAlignEnum value)
{
    updateCommon(std::make_unique<ModifyRoomFlags>(value, FlagModifyModeEnum::SET));
}

void RoomEditAttrDlg::neutralRadioButtonToggled(const bool val)
{
    if (val) {
        updateRoomAlign(RoomAlignEnum::NEUTRAL);
    }
}

void RoomEditAttrDlg::goodRadioButtonToggled(const bool val)
{
    if (val) {
        updateRoomAlign(RoomAlignEnum::GOOD);
    }
}

void RoomEditAttrDlg::evilRadioButtonToggled(const bool val)
{
    if (val) {
        updateRoomAlign(RoomAlignEnum::EVIL);
    }
}

void RoomEditAttrDlg::alignUndefRadioButtonToggled(const bool val)
{
    if (val) {
        updateRoomAlign(RoomAlignEnum::UNDEFINED);
    }
}

void RoomEditAttrDlg::updateRoomPortable(const RoomPortableEnum value)
{
    updateCommon(std::make_unique<ModifyRoomFlags>(value, FlagModifyModeEnum::SET));
}

void RoomEditAttrDlg::noPortRadioButtonToggled(bool val)
{
    if (val) {
        updateRoomPortable(RoomPortableEnum::NOT_PORTABLE);
    }
}

void RoomEditAttrDlg::portableRadioButtonToggled(const bool val)
{
    if (val) {
        updateRoomPortable(RoomPortableEnum::PORTABLE);
    }
}

void RoomEditAttrDlg::portUndefRadioButtonToggled(const bool val)
{
    if (val) {
        updateRoomPortable(RoomPortableEnum::UNDEFINED);
    }
}

void RoomEditAttrDlg::updateRoomRideable(const RoomRidableEnum value)
{
    updateCommon(std::make_unique<ModifyRoomFlags>(value, FlagModifyModeEnum::SET));
}

void RoomEditAttrDlg::noRideRadioButtonToggled(const bool val)
{
    if (val) {
        updateRoomRideable(RoomRidableEnum::NOT_RIDABLE);
    }
}

void RoomEditAttrDlg::ridableRadioButtonToggled(const bool val)
{
    if (val) {
        updateRoomRideable(RoomRidableEnum::RIDABLE);
    }
}

void RoomEditAttrDlg::rideUndefRadioButtonToggled(const bool val)
{
    if (val) {
        updateRoomRideable(RoomRidableEnum::UNDEFINED);
    }
}

void RoomEditAttrDlg::updateRoomLight(const RoomLightEnum value)
{
    updateCommon(std::make_unique<ModifyRoomFlags>(value, FlagModifyModeEnum::SET));
}

void RoomEditAttrDlg::litRadioButtonToggled(const bool val)
{
    if (val) {
        updateRoomLight(RoomLightEnum::LIT);
    }
}

void RoomEditAttrDlg::darkRadioButtonToggled(const bool val)
{
    if (val) {
        updateRoomLight(RoomLightEnum::DARK);
    }
}

void RoomEditAttrDlg::lightUndefRadioButtonToggled(const bool val)
{
    if (val) {
        updateRoomLight(RoomLightEnum::UNDEFINED);
    }
}

void RoomEditAttrDlg::updateRoomSundeath(const RoomSundeathEnum value)
{
    updateCommon(std::make_unique<ModifyRoomFlags>(value, FlagModifyModeEnum::SET));
}

void RoomEditAttrDlg::sundeathRadioButtonToggled(const bool val)
{
    if (val) {
        updateRoomSundeath(RoomSundeathEnum::SUNDEATH);
    }
}

void RoomEditAttrDlg::noSundeathRadioButtonToggled(const bool val)
{
    if (val) {
        updateRoomSundeath(RoomSundeathEnum::NO_SUNDEATH);
    }
}

void RoomEditAttrDlg::sundeathUndefRadioButtonToggled(const bool val)
{
    if (val) {
        updateRoomSundeath(RoomSundeathEnum::UNDEFINED);
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

    const RoomMobFlags flags{optFlag.value()};
    switch (item->checkState()) {
    case Qt::Unchecked:
        updateCommon(std::make_unique<ModifyRoomFlags>(flags, FlagModifyModeEnum::UNSET));
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        updateCommon(std::make_unique<ModifyRoomFlags>(flags, FlagModifyModeEnum::SET));
        break;
    }
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

    const RoomLoadFlags flags{optFlag.value()};

    switch (item->checkState()) {
    case Qt::Unchecked:
        updateCommon(std::make_unique<ModifyRoomFlags>(flags, FlagModifyModeEnum::UNSET));
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        updateCommon(std::make_unique<ModifyRoomFlags>(flags, FlagModifyModeEnum::SET));
        break;
    }
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

    const ExitFlags flags{optFlag.value()};
    auto dir = getSelectedExit();
    switch (item->checkState()) {
    case Qt::Unchecked:
        if (flags.isExit()) {
            // Remove connections when the exit is removed
            const auto &from = getSelectedRoom()->getId();
            const auto &e = getSelectedRoom()->exit(dir);
            for (const auto &to : e.outClone()) {
                m_mapData->execute(std::make_unique<RemoveOneWayExit>(from, to, dir),
                                   m_roomSelection);
            }
        }
        updateCommon(std::make_unique<ModifyExitFlags>(flags, dir, FlagModifyModeEnum::UNSET));
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        updateCommon(std::make_unique<ModifyExitFlags>(flags, dir, FlagModifyModeEnum::SET));
        break;
    }

    updateDialog(getSelectedRoom());
}

void RoomEditAttrDlg::doorNameLineEditTextChanged()
{
    const Room *const r = getSelectedRoom();

    m_mapData->execute(std::make_unique<SingleRoomAction>(
                           std::make_unique<ModifyExitFlags>(DoorName{doorNameLineEdit->text()},
                                                             getSelectedExit(),
                                                             FlagModifyModeEnum::SET),
                           r->getId()),
                       m_roomSelection);
    emit sig_requestUpdate();
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

    const DoorFlags flags{optFlag.value()};
    const auto dir = getSelectedExit();

    const auto modifyExit = [this, flags, dir](const FlagModifyModeEnum mode) {
        m_mapData->execute(
            std::make_unique<SingleRoomAction>(std::make_unique<ModifyExitFlags>(flags, dir, mode),
                                               deref(getSelectedRoom()).getId()),
            m_roomSelection);
        emit sig_requestUpdate();
    };

    switch (item->checkState()) {
    case Qt::Unchecked:
        modifyExit(FlagModifyModeEnum::UNSET);
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        modifyExit(FlagModifyModeEnum::SET);
        break;
    }
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

    const RoomTerrainEnum rtt = [this]() -> RoomTerrainEnum {
        // returns the first one that's checked, or UNDEFINED.
        for (uint i = 0; i < NUM_ROOM_TERRAIN_TYPES; ++i) {
            const auto rtt = static_cast<RoomTerrainEnum>(i);
            if (const QToolButton *const ptr = roomTerrainButtons[rtt])
                if (ptr->isChecked())
                    return rtt;
        }

        // oops
        return RoomTerrainEnum::UNDEFINED;
    }();
    terrainLabel->setPixmap(QPixmap(getPixmapFilename(rtt)));
    updateCommon(std::make_unique<ModifyRoomFlags>(rtt, FlagModifyModeEnum::SET));
}

// note tab
void RoomEditAttrDlg::roomNoteChanged()
{
    // TODO: Change this so you can't leave the tab without clicking "Update" or "Cancel",
    // and then only modify the note on update.
    const RoomNote note{roomNoteTextEdit->document()->toPlainText()};
    updateCommon(std::make_unique<ModifyRoomFlags>(note, FlagModifyModeEnum::SET), true);
}

// all tabs
void RoomEditAttrDlg::closeClicked()
{
    accept();
}
