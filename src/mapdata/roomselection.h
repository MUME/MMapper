#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <memory>
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

public:
    explicit RoomSelection(MapData &mapData);
    explicit RoomSelection(MapData &mapData, const Coordinate &c);
    explicit RoomSelection(MapData &mapData, const Coordinate &min, const Coordinate &max);
    ~RoomSelection() override;

public:
    const Room *getFirstRoom() const noexcept(false);
    RoomId getFirstRoomId() const noexcept(false);

public:
    const Room *getRoom(RoomId targetId);
    const Room *getRoom(const Coordinate &coord);
    void unselect(RoomId targetId);

public:
    bool isMovable(const Coordinate &offset) const;

public:
    void genericSearch(const RoomFilter &f);

public:
    static SharedRoomSelection createSelection(MapData &mapData);
    static SharedRoomSelection createSelection(MapData &mapData, const Coordinate &c);
    static SharedRoomSelection createSelection(MapData &mapData,
                                               const Coordinate &min,
                                               const Coordinate &max);

public:
    DELETE_CTORS_AND_ASSIGN_OPS(RoomSelection);
};
