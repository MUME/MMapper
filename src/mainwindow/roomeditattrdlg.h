#pragma once
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

class MapCanvas;
class MapData;
class QListWidgetItem;
class QObject;
class QShortcut;
class QWidget;
class Room;

class RoomListWidgetItem final : public QListWidgetItem
{
public:
    explicit RoomListWidgetItem(const QString &text, int priority = 0);
    explicit RoomListWidgetItem(const QIcon &icon, const QString &text, int priority = 0);

    bool operator<(const QListWidgetItem &other) const override;
};

class RoomEditAttrDlg final : public QDialog, private Ui::RoomEditAttrDlg
{
    Q_OBJECT

signals:
    void mapChanged();

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

    void doorNameLineEditTextChanged(const QString &);
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
    ~RoomEditAttrDlg();

    void readSettings();
    void writeSettings();

private:
    void connectAll();
    void disconnectAll();

    const Room *getSelectedRoom();
    ExitDirection getSelectedExit();
    void updateDialog(const Room *r);

    EnumIndexedArray<RoomListWidgetItem *, RoomLoadFlag> loadListItems{};
    EnumIndexedArray<RoomListWidgetItem *, RoomMobFlag> mobListItems{};
    EnumIndexedArray<RoomListWidgetItem *, ExitFlag> exitListItems{};
    EnumIndexedArray<RoomListWidgetItem *, DoorFlag> doorListItems{};

#define NUM_ELEMENTS(arr) (decltype(arr)::SIZE)
    static_assert(NUM_ELEMENTS(loadListItems) <= 32u);
    static_assert(NUM_ELEMENTS(mobListItems) <= 32u);
    static_assert(NUM_ELEMENTS(exitListItems) <= 16u);
    static_assert(NUM_ELEMENTS(doorListItems) <= 16u);
#undef NUM_ELEMENTS

    SharedRoomSelection m_roomSelection;
    MapData *m_mapData = nullptr;
    MapCanvas *m_mapCanvas = nullptr;
    QShortcut *m_hiddenShortcut = nullptr;
};
