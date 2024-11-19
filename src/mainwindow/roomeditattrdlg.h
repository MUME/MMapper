#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Connections.h"
#include "../global/EnumIndexedArray.h"
#include "../map/DoorFlags.h"
#include "../map/ExitDirection.h"
#include "../map/ExitFieldVariant.h"
#include "../map/ExitFlags.h"
#include "../map/mmapper2room.h"
#include "../mapdata/roomselection.h"
#include "ui_roomeditattrdlg.h"

#include <memory>
#include <vector>

#include <QDialog>
#include <QString>
#include <QtCore>
#include <QtWidgets/QListWidgetItem>

class AbstractAction;
class MapCanvas;
class MapData;
class QListWidgetItem;
class QObject;
class QShortcut;
class QWidget;
class Room;

class NODISCARD RoomListWidgetItem final : public QListWidgetItem
{
public:
    explicit RoomListWidgetItem(const QString &text, int priority = 0);
    explicit RoomListWidgetItem(const QIcon &icon, const QString &text, int priority = 0);

    bool operator<(const QListWidgetItem &other) const override;
};

class NODISCARD_QOBJECT RoomEditAttrDlg final : public QDialog, private Ui::RoomEditAttrDlg
{
    Q_OBJECT

private:
    mmqt::Connections m_connections;

    using UniqueRoomListWidgetItem = std::unique_ptr<RoomListWidgetItem>;
    EnumIndexedArray<UniqueRoomListWidgetItem, RoomLoadFlagEnum> m_loadListItems;
    EnumIndexedArray<UniqueRoomListWidgetItem, RoomMobFlagEnum> m_mobListItems;
    EnumIndexedArray<UniqueRoomListWidgetItem, ExitFlagEnum> m_exitListItems;
    EnumIndexedArray<UniqueRoomListWidgetItem, DoorFlagEnum> m_doorListItems;
    EnumIndexedArray<QToolButton *, RoomTerrainEnum> roomTerrainButtons;

    SharedRoomSelection m_roomSelection;
    MapData *m_mapData = nullptr;
    MapCanvas *m_mapCanvas = nullptr;
    std::unique_ptr<QShortcut> m_hiddenShortcut;

private:
    void requestUpdate() { emit sig_requestUpdate(); }

signals:
    void sig_requestUpdate();

public slots:
    void setRoomSelection(const SharedRoomSelection &, MapData *, MapCanvas *);

    // selection page
    void roomListCurrentIndexChanged(int);

    // attributes page
    void neutralRadioButtonToggled(bool);
    void goodRadioButtonToggled(bool);
    void evilRadioButtonToggled(bool);
    void alignUndefRadioButtonToggled(bool);

    void noPortRadioButtonToggled(bool);
    void portableRadioButtonToggled(bool);
    void portUndefRadioButtonToggled(bool);

    void noRideRadioButtonToggled(bool);
    void ridableRadioButtonToggled(bool);
    void rideUndefRadioButtonToggled(bool);

    void litRadioButtonToggled(bool);
    void darkRadioButtonToggled(bool);
    void lightUndefRadioButtonToggled(bool);

    void noSundeathRadioButtonToggled(bool);
    void sundeathRadioButtonToggled(bool);
    void sundeathUndefRadioButtonToggled(bool);

    void mobFlagsListItemChanged(QListWidgetItem *);
    void loadFlagsListItemChanged(QListWidgetItem *);

    void exitButtonToggled(bool);

    void exitFlagsListItemChanged(QListWidgetItem *);

    void doorNameLineEditTextChanged();
    void doorFlagsListItemChanged(QListWidgetItem *);

    void toggleHiddenDoor();

    // terrain tab
    void terrainToolButtonToggled(bool);

    // note tab
    void roomNoteChanged();

    // all tabs
    void closeClicked();

public:
    explicit RoomEditAttrDlg(QWidget *parent);
    ~RoomEditAttrDlg() final;

    void readSettings();
    void writeSettings();

private:
    void connectAll();
    void disconnectAll();

    NODISCARD const Room *getSelectedRoom();
    NODISCARD ExitDirEnum getSelectedExit();
    void updateDialog(const Room *r);

private:
    void updateCommon(std::unique_ptr<AbstractAction> moved_action, bool onlyExecuteAction = false);
    void updateRoomAlign(RoomAlignEnum value);
    void updateRoomPortable(RoomPortableEnum value);
    void updateRoomRideable(RoomRidableEnum value);
    void updateRoomLight(RoomLightEnum value);
    void updateRoomSundeath(RoomSundeathEnum value);

private:
    NODISCARD QRadioButton *getAlignRadioButton(RoomAlignEnum value) const;
    NODISCARD QRadioButton *getPortableRadioButton(RoomPortableEnum value) const;
    NODISCARD QRadioButton *getRideableRadioButton(RoomRidableEnum value) const;
    NODISCARD QRadioButton *getLightRadioButton(RoomLightEnum value) const;
    NODISCARD QRadioButton *getSundeathRadioButton(RoomSundeathEnum value) const;
    NODISCARD QToolButton *getTerrainToolButton(RoomTerrainEnum value) const;
};
