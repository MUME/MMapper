#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef ROOMSELECTION_H
#define ROOMSELECTION_H

#include <QMap>

#include "../expandoracommon/MmQtHandle.h"
#include "../expandoracommon/RoomRecipient.h"
#include "../global/NullPointerException.h"
#include "../global/RAII.h"
#include "../global/RuleOf5.h"
#include "../global/roomid.h"
#include "roomfilter.h"

class RoomSelection;
using SharedRoomSelection = std::shared_ptr<RoomSelection>;
using SigRoomSelection = MmQtHandle<RoomSelection>;

class Room;
class RoomAdmin;
class Coordinate;
class MapData;
class NODISCARD RoomSelection final : public QMap<RoomId, const Room *>, public RoomRecipient
{
public:
    void receiveRoom(RoomAdmin *admin, const Room *aRoom) override;

private:
    MapData &m_mapData;
    bool m_moved = false;

public:
    explicit RoomSelection(MapData &mapData);
    explicit RoomSelection(MapData &mapData, const Coordinate &c);
    explicit RoomSelection(MapData &mapData, const Coordinate &ulf, const Coordinate &lrb);
    ~RoomSelection() override;

public:
    const RoomSelection &operator*() const noexcept(false);

public:
    const Room *getFirstRoom() const noexcept(false);
    RoomId getFirstRoomId() const noexcept(false);

public:
    const Room *getRoom(const RoomId targetId);
    const Room *getRoom(const Coordinate &coord);
    void unselect(const RoomId targetId);

public:
    bool isMovable(const Coordinate &offset) const;

public:
    void genericSearch(const RoomFilter &f);

public:
    static SharedRoomSelection createSelection(MapData &mapData);
    static SharedRoomSelection createSelection(MapData &mapData, const Coordinate &c);
    static SharedRoomSelection createSelection(MapData &mapData,
                                               const Coordinate &ulf,
                                               const Coordinate &lrb);

public:
    RoomSelection(RoomSelection &&);
    DELETE_COPY_CTOR(RoomSelection);
    DELETE_MOVE_ASSIGN_OP(RoomSelection);
};

#endif // ROOMSELECTION_H
