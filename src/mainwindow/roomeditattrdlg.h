#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <memory>
#include <vector>
#include <QDialog>
#include <QString>
#include <QtCore>
#include <QtWidgets/QListWidgetItem>

#include "../global/EnumIndexedArray.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/ExitFlags.h"
#include "../mapdata/mmapper2exit.h"
#include "../mapdata/mmapper2room.h"
#include "../mapdata/roomselection.h"
#include "ui_roomeditattrdlg.h"

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

class RoomEditAttrDlg final : public QDialog, private Ui::RoomEditAttrDlg
{
    Q_OBJECT

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
    explicit RoomEditAttrDlg(QWidget *parent = nullptr);
    ~RoomEditAttrDlg() final;

    void readSettings();
    void writeSettings();

private:
    struct NODISCARD Connections final
    {
    private:
        std::vector<QMetaObject::Connection> m_connections;

    public:
        Connections &operator+=(QMetaObject::Connection c)
        {
            m_connections.emplace_back(std::move(c));
            return *this;
        }
        void disconnectAll(QObject &o)
        {
            for (const auto &c : m_connections)
                o.disconnect(c);
            m_connections.clear();
        }
    };
    Connections m_connections;
    void connectAll();
    void disconnectAll();

    const Room *getSelectedRoom();
    ExitDirEnum getSelectedExit();
    void updateDialog(const Room *r);

private:
    void updateCommon(std::unique_ptr<AbstractAction> moved_action, bool onlyExecuteAction = false);
    void updateRoomAlign(RoomAlignEnum value);
    void updateRoomPortable(RoomPortableEnum value);
    void updateRoomRideable(RoomRidableEnum value);
    void updateRoomLight(RoomLightEnum value);
    void updateRoomSundeath(RoomSundeathEnum value);

private:
    QRadioButton *getAlignRadioButton(RoomAlignEnum value) const;
    QRadioButton *getPortableRadioButton(RoomPortableEnum value) const;
    QRadioButton *getRideableRadioButton(RoomRidableEnum value) const;
    QRadioButton *getLightRadioButton(RoomLightEnum value) const;
    QRadioButton *getSundeathRadioButton(RoomSundeathEnum value) const;
    QToolButton *getTerrainToolButton(RoomTerrainEnum value) const;

private:
    EnumIndexedArray<RoomListWidgetItem *, RoomLoadFlagEnum> loadListItems;
    EnumIndexedArray<RoomListWidgetItem *, RoomMobFlagEnum> mobListItems;
    EnumIndexedArray<RoomListWidgetItem *, ExitFlagEnum> exitListItems;
    EnumIndexedArray<RoomListWidgetItem *, DoorFlagEnum> doorListItems;
    EnumIndexedArray<QToolButton *, RoomTerrainEnum> roomTerrainButtons;

    SharedRoomSelection m_roomSelection;
    MapData *m_mapData = nullptr;
    MapCanvas *m_mapCanvas = nullptr;
    std::unique_ptr<QShortcut> m_hiddenShortcut;
};
