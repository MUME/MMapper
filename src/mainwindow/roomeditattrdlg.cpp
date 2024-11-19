// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "roomeditattrdlg.h"

#include "../client/displaywidget.h"
#include "../configuration/configuration.h"
#include "../display/Filenames.h"
#include "../display/mapcanvas.h"
#include "../global/AnsiOstream.h"
#include "../global/Consts.h"
#include "../global/PrintUtils.h"
#include "../global/SignalBlocker.h"
#include "../global/utils.h"
#include "../map/Diff.h"
#include "../map/ExitFieldVariant.h"
#include "../map/RoomFieldVariant.h"
#include "../map/enums.h"
#include "../map/exit.h"
#include "../map/mmapper2room.h"
#include "../map/room.h"
#include "../map/roomid.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/roomselection.h"

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <sstream>

#include <QMessageLogContext>
#include <QString>
#include <QVariant>
#include <QtGui>
#include <QtWidgets>

namespace { // anonymous

volatile bool auto_apply_note_on_close = false;

using UniqueRoomListWidgetItem = std::unique_ptr<RoomListWidgetItem>;

template<typename T>
void fixMissing(T &array, const char *const name)
{
    // reference to pointer so we can add missing elements
    for (UniqueRoomListWidgetItem &x : array) {
        if (x != nullptr) {
            continue;
        }
        const auto ordinal = static_cast<int>(&x - array.data());
        x = std::make_unique<RoomListWidgetItem>(QString::asprintf("%d", ordinal));
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
    for (UniqueRoomListWidgetItem &x : array) {
        deref(x).setFlags(flags);
        widget.addItem(checked_static_upcast<QListWidgetItem *>(x.get()));
    }
}

template<typename T>
NODISCARD QIcon getIcon(T flag)
{
    const QString filename = getPixmapFilename(flag);
    try {
        QIcon result(filename);
        if (result.isNull()) {
            throw std::runtime_error(
                mmqt::toStdStringUtf8(QString("failed to load icon '%1'").arg(filename)));
        }
        return result;
    } catch (...) {
        qWarning() << "Oops: Unable to create icon:" << filename;
        throw;
    }
}

NODISCARD int getPriority(const RoomMobFlagEnum flag)
{
#define X_POS(UPPER, pos) \
    do { \
        if (flag == RoomMobFlagEnum::UPPER) { \
            return (pos) - (NUM_ROOM_MOB_FLAGS); \
        } \
    } while (false)
    X_POS(PASSIVE_MOB, 0);
    X_POS(AGGRESSIVE_MOB, 1);
    X_POS(ELITE_MOB, 2);
    X_POS(SUPER_MOB, 3);
    X_POS(RATTLESNAKE, 4);
    X_POS(QUEST_MOB, 5);
#undef X_POS
    return static_cast<int>(flag);
}

NODISCARD int getPriority(const RoomLoadFlagEnum flag)
{
#define X_POS(UPPER, pos) \
    do { \
        if (flag == RoomLoadFlagEnum::UPPER) { \
            return (pos) - (NUM_ROOM_LOAD_FLAGS); \
        } \
    } while (false)
    X_POS(TREASURE, 0);
    X_POS(ARMOUR, 1);
    X_POS(WEAPON, 2);
    X_POS(EQUIPMENT, 3);
#undef X_POS
    return static_cast<int>(flag);
}

template<typename T, typename Flags>
void setCheckStates(T &array, const Flags flags)
{
    for (size_t i = 0, len = array.size(); i < len; ++i) {
        const auto flag = static_cast<typename T::index_type>(i);
        if (UniqueRoomListWidgetItem &x = array[flag]) {
            deref(x).setCheckState(flags.contains(flag) ? Qt::CheckState::Checked
                                                        : Qt::CheckState::Unchecked);
        }
    }
}

template<typename T>
void setFlags(T &array, const QFlags<Qt::ItemFlag> flags)
{
    for (UniqueRoomListWidgetItem &x : array) {
        deref(x).setFlags(flags);
    }
}

// NOTE: the multi-line strings are all normalized to contain a trailing newline
// if they contain any text, and appending text includes an implicit newline,
// so we have to remove the trailing newline.
template<typename T>
void add_boxed_string(QTextEdit *const textEdit, const T &boxed)
{
    std::string_view sv = boxed.getStdStringViewUtf8();
    trim_newline_inplace(sv);
    deref(textEdit).append(mmqt::toQStringUtf8(sv));
}

} // namespace

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
    static_assert(NUM_ELEMENTS(m_loadListItems) <= 32u);
    static_assert(NUM_ELEMENTS(m_mobListItems) <= 32u);
    static_assert(NUM_ELEMENTS(m_exitListItems) <= 16u);
    static_assert(NUM_ELEMENTS(m_doorListItems) <= 16u);
    static_assert(NUM_ELEMENTS(m_roomTerrainButtons) == NUM_ROOM_TERRAIN_TYPES);
    static_assert(NUM_ROOM_TERRAIN_TYPES == 16);
#undef NUM_ELEMENTS

    setupUi(this);

    // NOTE: Another option would be to just initialize them all directly here,
    // and then get rid of have getTerrainToolButton() index into the array,
    // or get rid of the function entirely.
    for (size_t i = 0; i < NUM_ROOM_TERRAIN_TYPES; ++i) {
        const auto rtt = static_cast<RoomTerrainEnum>(i);
        m_roomTerrainButtons[rtt] = getTerrainToolButton(rtt);
    }

    roomDescriptionTextEdit->setLineWrapMode(QTextEdit::NoWrap);

    for (const RoomMobFlagEnum flag : ALL_MOB_FLAGS) {
        m_mobListItems[flag] = std::make_unique<RoomListWidgetItem>(getIcon(flag),
                                                                    getName(flag),
                                                                    getPriority(flag));
    }

    installWidgets(m_mobListItems,
                   "mob room flags",
                   *mobFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsAutoTristate);

    for (const RoomLoadFlagEnum flag : ALL_LOAD_FLAGS) {
        m_loadListItems[flag] = std::make_unique<RoomListWidgetItem>(getIcon(flag),
                                                                     getName(flag),
                                                                     getPriority(flag));
    }

    installWidgets(m_loadListItems,
                   "load list",
                   *loadFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsAutoTristate);

    for (const ExitFlagEnum flag : ALL_EXIT_FLAGS) {
        m_exitListItems[flag] = std::make_unique<RoomListWidgetItem>(
            mmqt::toQStringUtf8(getName(flag)));
    }

    installWidgets(m_exitListItems,
                   "exit list",
                   *exitFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

    if (auto &ex = m_exitListItems[ExitFlagEnum::EXIT]) {
        ex->setFlags(ex->flags() & ~(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled));
    }

    for (const DoorFlagEnum flag : ALL_DOOR_FLAGS) {
        m_doorListItems[flag] = std::make_unique<RoomListWidgetItem>(
            mmqt::toQStringUtf8(getName(flag)));
    }

    installWidgets(m_doorListItems,
                   "door list",
                   *doorFlagsListWidget,
                   Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

    m_hiddenShortcut = std::make_unique<QShortcut>(QKeySequence(
                                                       tr("Ctrl+H", "Room edit > hidden flag")),
                                                   this);

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

    for (QToolButton *const toolButton : m_roomTerrainButtons) {
        std::ignore = deref(toolButton);
        m_connections += connect(toolButton,
                                 &QAbstractButton::toggled,
                                 this,
                                 &RoomEditAttrDlg::terrainToolButtonToggled);
    }

    m_connections += connect(roomNoteTextEdit, &QTextEdit::textChanged, this, [this]() {
        // This doesn't actually check if you've modified it back to the original text.
        // (Workaround: press the "Revert" button.)
        setRoomNoteDirty(true);
    });

    m_connections += connect(tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        auto to = tabWidget->widget(index);
        if (to == noteTab) {
            m_noteSelected = true;
        } else if (m_noteSelected) {
            if (!m_noteDirty) {
                m_noteSelected = false;
            } else {
                tabWidget->setCurrentWidget(noteTab);
                assert(tabWidget->currentWidget() == noteTab);
                assert(m_noteSelected);
            }
        }
    });

    m_connections += connect(this, &QDialog::finished, this, [this](int /*result*/) {
        if (!m_noteSelected) {
            return;
        }

        if (m_noteDirty) {
            // this shouldn't happen for "Close" and "X", but it can still happen by hitting ESCAPE.
            if (auto_apply_note_on_close) {
                roomNoteChanged();
            } else {
                const QString title = "[mmapper] warning: ignored note";
                const QString qnote = deref(deref(roomNoteTextEdit).document()).toPlainText();

                // title is cut off, and stack-overflow solutions don't work.
                // we need a generic message box class that actually works.
                // let's hope nobody actually has to see this message.
                QMessageBox box(this);
                box.setWindowTitle(title);
                box.setText(qnote);
                box.exec();
            }
        }

        m_noteSelected = false;
        setRoomNoteDirty(false);
    });

    m_connections += connect(roomNoteApplyButton, &QPushButton::clicked, this, [this]() {
        roomNoteChanged();
        {
            // lie about room note being selected for the duration of this call
            m_noteSelected = false;
            updateDialog(this->getSelectedRoom());
            m_noteSelected = true;
        }
    });

    m_connections += connect(roomNoteClearButton, &QPushButton::clicked, this, [this]() {
        auto &rnte = deref(roomNoteTextEdit);
        rnte.clear();
        setRoomNoteDirty(false);
        if (auto r = this->getSelectedRoom()) {
            if (!r->getNote().empty()) {
                setRoomNoteDirty(true);
            }
        }
    });

    m_connections += connect(roomNoteRevertButton, &QPushButton::clicked, this, [this]() {
        auto &rnte = deref(roomNoteTextEdit);
        rnte.clear();
        if (auto r = this->getSelectedRoom()) {
            add_boxed_string(roomNoteTextEdit, r->getNote());
        }
        setRoomNoteDirty(false);
    });

    m_connections += connect(m_hiddenShortcut.get(),
                             &QShortcut::activated,
                             this,
                             &RoomEditAttrDlg::toggleHiddenDoor);

    m_connections += connect(roomListComboBox,
                             QOverload<int>::of(&QComboBox::currentIndexChanged),
                             this,
                             &RoomEditAttrDlg::roomListCurrentIndexChanged);
}

void RoomEditAttrDlg::disconnectAll()
{
    m_connections.disconnectAll();
}

std::optional<RoomHandle> RoomEditAttrDlg::getSelectedRoom()
{
    if (m_roomSelection == nullptr || m_roomSelection->empty()) {
        return std::nullopt;
    }
    if (m_roomSelection->size() == 1) {
        return m_mapData->getCurrentMap().tryGetRoomHandle(m_roomSelection->getFirstRoomId());
    }
    const auto target = RoomId{
        roomListComboBox->itemData(roomListComboBox->currentIndex()).toUInt()};
    if (m_roomSelection->contains(target)) {
        return m_mapData->getCurrentMap().tryGetRoomHandle(target);
    }
    return std::nullopt;
}

ExitDirEnum RoomEditAttrDlg::getSelectedExit()
{
    EnumIndexedArray<QPushButton *, ExitDirEnum, NUM_EXITS_NESWUD> buttons;
    buttons[ExitDirEnum::NORTH] = exitNButton;
    buttons[ExitDirEnum::SOUTH] = exitSButton;
    buttons[ExitDirEnum::EAST] = exitEButton;
    buttons[ExitDirEnum::WEST] = exitWButton;
    buttons[ExitDirEnum::UP] = exitUButton;
    buttons[ExitDirEnum::DOWN] = exitDButton;

    for (auto *const button : buttons) {
        QPalette pal;
        button->setAutoFillBackground(false);
        button->setPalette(pal);
    }

    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        auto *const button = buttons[dir];
        if (!button->isChecked()) {
            continue;
        }

        QColor bg = Qt::black;
        QPalette pal;
        pal.setColor(QPalette::Button, bg.rgb());
        button->setPalette(pal);
        button->setAutoFillBackground(true);
        return dir;
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

    if (rs == nullptr) {
        return;
    }

    tabWidget->setCurrentWidget(attributesTab);

    auto &mapData = deref(m_mapData);
    const auto &map = mapData.getCurrentMap();
    auto addToCombo = [this, &map](RoomId id) -> RoomHandle {
        RoomHandle room = map.getRoomHandle(id);
        const QString shown = QString("Room %1: %2")
                                  .arg(room.getIdExternal().asUint32())
                                  .arg(room.getName().toQString());
        // REVISIT: Should this be ExternalRoomId?
        roomListComboBox->addItem(shown, room.getId().asUint32());
        return room;
    };

    auto &sel = deref(rs);
    sel.removeMissing(mapData);
    if (sel.size() == 1) {
        const RoomHandle &room = addToCombo(sel.getFirstRoomId());
        updateDialog(room);
    } else {
        // REVISIT: Does the zero here mean that RoomId{0} won't work properly?
        // Should we change this to INVALID_ROOMID.value()?
        roomListComboBox->addItem("All", 0);
        for (const RoomId id : sel) {
            MAYBE_UNUSED const auto room = //
                addToCombo(id);
        }
        updateDialog(std::nullopt);
    }

    connect(this, &RoomEditAttrDlg::sig_requestUpdate, m_mapCanvas, &MapCanvas::slot_requestUpdate);
}

void RoomEditAttrDlg::updateDialog(const std::optional<RoomHandle> &r)
{
    class NODISCARD DisconnectReconnectAntiPattern final
    {
    private:
        RoomEditAttrDlg &m_self;
        SignalBlocker m_signalBlocker;

    public:
        explicit DisconnectReconnectAntiPattern(RoomEditAttrDlg &self)
            : m_self{self}
            , m_signalBlocker{self}
        {
            m_self.disconnectAll();
        }
        ~DisconnectReconnectAntiPattern() { m_self.connectAll(); }
    } antiPattern{*this};

    constexpr auto CHECKABLE_AND_ENABLED = Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
    if (r == std::nullopt || !r->exists()) {
        roomDescriptionTextEdit->clear();
        roomDescriptionTextEdit->setEnabled(false);

        clearRoomNote();
        roomNoteTextEdit->setEnabled(false);

        roomStatTextEdit->clear();
        roomStatTextEdit->setEnabled(false);

        roomDiffTextEdit->clear();
        roomDiffTextEdit->setEnabled(false);

        terrainLabel->setPixmap(QPixmap(getPixmapFilename(RoomTerrainEnum::UNDEFINED)));

        exitsFrame->setEnabled(false);

        rideGroupBox->setChecked(false);
        alignGroupBox->setChecked(false);
        teleportGroupBox->setChecked(false);
        lightGroupBox->setChecked(false);
        sunGroupBox->setChecked(false);

        for (const auto &x : m_loadListItems) {
            x->setFlags(CHECKABLE_AND_ENABLED | Qt::ItemIsAutoTristate);
            x->setCheckState(Qt::PartiallyChecked);
        }

        for (const auto &x : m_mobListItems) {
            x->setFlags(CHECKABLE_AND_ENABLED | Qt::ItemIsAutoTristate);
            x->setCheckState(Qt::PartiallyChecked);
        }
    } else {
        roomDescriptionTextEdit->clear();
        roomDescriptionTextEdit->setEnabled(true);

        exitsFrame->setEnabled(true);

        const auto &e = r->getExit(getSelectedExit());
        setCheckStates(m_exitListItems, e.getExitFlags());

        if (e.exitIsDoor()) {
            doorNameLineEdit->setEnabled(true);
            doorFlagsListWidget->setEnabled(true);
            doorNameLineEdit->setText(e.getDoorName().toQString());
            setCheckStates(m_doorListItems, e.getDoorFlags());
        } else {
            doorNameLineEdit->clear();
            doorNameLineEdit->setEnabled(false);
            doorFlagsListWidget->setEnabled(false);
        }

        const bool shouldEnableDoorCheck = !e.exitIsDoor()
                                           || (e.getDoorFlags().empty() && e.getDoorName().empty());
        if (auto &ex = m_exitListItems[ExitFlagEnum::DOOR]) {
            if (shouldEnableDoorCheck) {
                ex->setFlags(ex->flags() | CHECKABLE_AND_ENABLED);
            } else {
                ex->setFlags(ex->flags() & ~CHECKABLE_AND_ENABLED);
            }
        }

        setFlags(m_loadListItems, CHECKABLE_AND_ENABLED);
        setFlags(m_mobListItems, CHECKABLE_AND_ENABLED);

        setCheckStates(m_mobListItems, r->getMobFlags());
        setCheckStates(m_loadListItems, r->getLoadFlags());

        roomDescriptionTextEdit->setEnabled(true);
        roomNoteTextEdit->setEnabled(true);

        setAnsiText(roomDescriptionTextEdit, previewRoom(*r));

        {
            assert(!m_noteSelected);
            clearRoomNote();
            add_boxed_string(roomNoteTextEdit, r->getNote());
            setRoomNoteDirty(false);
        }

        // can this ever be nullptr?
        if (roomStatTextEdit != nullptr) {
            const auto s = [r]() -> std::string {
                try {
                    std::ostringstream os;
                    {
                        AnsiOstream aos{os};
                        r->getMap().statRoom(aos, r->getId());
                    }
                    return std::move(os).str();
                } catch (const std::exception &ex) {
                    return std::string("Exception: ") + ex.what();
                }
            }();
            setAnsiText(roomStatTextEdit, s);
        }

        // can this ever be nullptr?
        if (roomDiffTextEdit != nullptr) {
            const auto s = [this, r]() -> std::string {
                auto saved = m_mapData->getSavedMap();
                auto current = m_mapData->getCurrentMap();

                const ExternalRoomId ext = r->getIdExternal();
                auto pOld = saved.findRoomHandle(ext);
                auto pNew = current.findRoomHandle(ext);
                if (!pOld) {
                    return "The room was created since the last save.";
                }
                if (!pNew) {
                    return "This should be impossible, but the room does not exist?";
                }

                try {
                    std::ostringstream os;
                    {
                        AnsiOstream aos{os};
                        OstreamDiffReporter odr{aos};
                        compare(odr, *pOld, *pNew);
                    }
                    auto str = std::move(os).str();
                    if (str.empty()) {
                        return "No changes since the last save.";
                    }
                    return str;
                } catch (const std::exception &ex) {
                    return std::string("Exception: ") + ex.what();
                }
            }();

            setAnsiText(roomDiffTextEdit, s);
        }

        const auto get_terrain_pixmap = [](RoomTerrainEnum type) -> QString {
            if (type == RoomTerrainEnum::ROAD) {
                return getPixmapFilename(TaggedRoad{
                    RoadIndexMaskEnum::NORTH | RoadIndexMaskEnum::EAST | RoadIndexMaskEnum::SOUTH});
            } else {
                return getPixmapFilename(type);
            }
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
        if (auto *const button = getTerrainToolButton(r->getTerrainType())) {
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

void RoomEditAttrDlg::updateCommon(const std::function<Change(const RawRoom &)> &getChange,
                                   const bool onlyExecuteAction)
{
    if (const auto &r = getSelectedRoom()) {
        m_mapData->applySingleChange(getChange(r->getRaw()));

    } else {
        m_mapData->applyChangesToList(*m_roomSelection, getChange);
    }

    // REVISIT: Why don't we want this to happen every time?
    if (!onlyExecuteAction) {
        updateDialog(getSelectedRoom());
        requestUpdate();
    }
}

void RoomEditAttrDlg::setFieldCommon(const RoomFieldVariant &var,
                                     const FlagModifyModeEnum mode,
                                     const bool onlyExecuteAction)
{
    updateCommon(
        [&var, mode](const RawRoom &room) -> Change {
            return Change{room_change_types::ModifyRoomFlags{room.getId(), var, mode}};
        },
        onlyExecuteAction);
}

void RoomEditAttrDlg::setSelectedRoomExitField(const ExitFieldVariant &var,
                                               const ExitDirEnum dir,
                                               const FlagModifyModeEnum mode)
{
    const auto id = getSelectedRoom()->getId();
    const bool changed = m_mapData->applySingleChange(
        Change{exit_change_types::ModifyExitFlags{id, dir, var, mode}});

    if (changed) {
        updateDialog(getSelectedRoom());
    }
}

void RoomEditAttrDlg::updateRoomAlign(const RoomAlignEnum value)
{
    setFieldCommon(RoomFieldVariant{value}, FlagModifyModeEnum::ASSIGN);
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
    setFieldCommon(RoomFieldVariant{value}, FlagModifyModeEnum::ASSIGN);
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
    setFieldCommon(RoomFieldVariant{value}, FlagModifyModeEnum::ASSIGN);
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
    setFieldCommon(RoomFieldVariant{value}, FlagModifyModeEnum::ASSIGN);
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
    setFieldCommon(RoomFieldVariant{value}, FlagModifyModeEnum::ASSIGN);
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
    std::ignore = deref(item);
    const auto optFlag = m_mobListItems.findIndexOf(
        checked_dynamic_downcast<RoomListWidgetItem *>(item));
    if (!optFlag) {
        qWarning() << "oops" << __FILE__ << ":" << __LINE__;
        return;
    }

    const RoomMobFlags flags{optFlag.value()};
    switch (item->checkState()) {
    case Qt::Unchecked:
        setFieldCommon(RoomFieldVariant{flags}, FlagModifyModeEnum::REMOVE);
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        setFieldCommon(RoomFieldVariant{flags}, FlagModifyModeEnum::INSERT);
        break;
    }
}

void RoomEditAttrDlg::loadFlagsListItemChanged(QListWidgetItem *const item)
{
    std::ignore = deref(item);

    const auto optFlag = m_loadListItems.findIndexOf(
        checked_dynamic_downcast<RoomListWidgetItem *>(item));
    if (!optFlag) {
        qWarning() << "oops: " << __FILE__ << ":" << __LINE__;
        return;
    }

    const RoomLoadFlags flags{optFlag.value()};

    switch (item->checkState()) {
    case Qt::Unchecked:
        setFieldCommon(RoomFieldVariant{flags}, FlagModifyModeEnum::REMOVE);
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        setFieldCommon(RoomFieldVariant{flags}, FlagModifyModeEnum::INSERT);
        break;
    }
}

void RoomEditAttrDlg::exitFlagsListItemChanged(QListWidgetItem *const item)
{
    std::ignore = deref(item);

    const auto optFlag = m_exitListItems.findIndexOf(
        checked_dynamic_downcast<RoomListWidgetItem *>(item));
    if (!optFlag) {
        qWarning() << "oops: " << __FILE__ << ":" << __LINE__;
        return;
    }

    const ExitFlags flags{optFlag.value()};
    const ExitDirEnum dir = getSelectedExit();

    const auto modifyExit = [this, flags, dir](const FlagModifyModeEnum mode) {
        this->setSelectedRoomExitField(ExitFieldVariant{flags}, dir, mode);
        requestUpdate();
    };

    switch (item->checkState()) {
    case Qt::Unchecked:
        modifyExit(FlagModifyModeEnum::REMOVE);
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        modifyExit(FlagModifyModeEnum::INSERT);
        break;
    }

    updateDialog(getSelectedRoom());
}

void RoomEditAttrDlg::doorNameLineEditTextChanged()
{
    const QString &doorName = doorNameLineEdit->text();
    this->setSelectedRoomExitField(ExitFieldVariant{mmqt::makeDoorName(doorName)},
                                   getSelectedExit(),
                                   FlagModifyModeEnum::ASSIGN);

    requestUpdate();
}

void RoomEditAttrDlg::doorFlagsListItemChanged(QListWidgetItem *const item)
{
    std::ignore = deref(item);

    const auto optFlag = m_doorListItems.findIndexOf(
        checked_dynamic_downcast<RoomListWidgetItem *>(item));
    if (!optFlag) {
        qWarning() << "oops: " << __FILE__ << ":" << __LINE__;
        return;
    }

    const DoorFlags flags{optFlag.value()};
    const auto dir = getSelectedExit();

    const auto modifyExit = [this, flags, dir](const FlagModifyModeEnum mode) {
        this->setSelectedRoomExitField(ExitFieldVariant{flags}, dir, mode);
        requestUpdate();
    };

    switch (item->checkState()) {
    case Qt::Unchecked:
        modifyExit(FlagModifyModeEnum::REMOVE);
        break;
    case Qt::PartiallyChecked:
        break;
    case Qt::Checked:
        modifyExit(FlagModifyModeEnum::INSERT);
        break;
    }

    updateDialog(getSelectedRoom());
}

// REVISIT: Remove this feature?
void RoomEditAttrDlg::toggleHiddenDoor()
{
    if (!doorFlagsListWidget->isEnabled()) {
        return;
    }

    auto &hidden = deref(m_doorListItems[DoorFlagEnum::HIDDEN]);
    hidden.setCheckState(hidden.checkState() == Qt::Unchecked ? Qt::Checked : Qt::Unchecked);
    updateDialog(getSelectedRoom());
}

// terrain tab
void RoomEditAttrDlg::terrainToolButtonToggled(bool val)
{
    if (!val) {
        return;
    }

    const RoomTerrainEnum rtt = [this]() -> RoomTerrainEnum {
        // returns the first one that's checked, or UNDEFINED.
        for (size_t i = 0; i < NUM_ROOM_TERRAIN_TYPES; ++i) {
            const auto tmp = static_cast<RoomTerrainEnum>(i);
            if (const QToolButton *const ptr = m_roomTerrainButtons[tmp]) {
                if (ptr->isChecked()) {
                    return tmp;
                }
            }
        }

        // oops
        return RoomTerrainEnum::UNDEFINED;
    }();

    terrainLabel->setPixmap(QPixmap(getPixmapFilename(rtt)));
    this->setFieldCommon(RoomFieldVariant{rtt}, FlagModifyModeEnum::ASSIGN);
}

// note tab
void RoomEditAttrDlg::roomNoteChanged()
{
    auto &rnte = deref(roomNoteTextEdit);
    const auto qnote = deref(rnte.document()).toPlainText();
    const auto note = mmqt::makeRoomNote(qnote);
    setFieldCommon(RoomFieldVariant{note}, FlagModifyModeEnum::ASSIGN, true);
    setRoomNoteDirty(false);
}

// all tabs
void RoomEditAttrDlg::closeClicked()
{
    if (m_noteSelected && m_noteDirty) {
        // ignore
        // Should we flash the window or create a popup?
    } else {
        accept();
    }
}

void RoomEditAttrDlg::closeEvent(QCloseEvent *const ev)
{
    if (m_noteSelected && m_noteDirty) {
        deref(ev).ignore();
        // Should we flash the window or create a popup?
        return;
    }
    QDialog::closeEvent(ev);
}

void RoomEditAttrDlg::clearRoomNote()
{
    if (m_noteSelected) {
        roomNoteChanged();
        m_noteSelected = false;
    }
    auto &rnte = deref(roomNoteTextEdit);
    rnte.clear();
    setRoomNoteDirty(false);
}

void RoomEditAttrDlg::setRoomNoteDirty(const bool dirty)
{
    const bool noteIsEmpty = deref(deref(roomNoteTextEdit).document()).isEmpty();

    m_noteDirty = dirty;
    deref(closeButton).setEnabled(!dirty);
    deref(roomNoteApplyButton).setEnabled(dirty);
    deref(roomNoteRevertButton).setEnabled(dirty);
    deref(roomNoteClearButton).setEnabled(!noteIsEmpty);
}
