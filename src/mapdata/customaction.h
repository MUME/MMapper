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

#ifndef CUSTOMACTION_H
#define CUSTOMACTION_H

#include <list>
#include <QVariant>
#include <QtCore>
#include <QtGlobal>

#include "../expandoracommon/coordinate.h"
#include "../global/roomid.h"
#include "../mapfrontend/mapaction.h"
#include "ExitDirection.h"
#include "ExitFieldVariant.h"
#include "mmapper2exit.h"
#include "mmapper2room.h"

class DoorFlags;
class ExitFlags;
class MapData;
class MapFrontend;
class ParseEvent;
class Room;
class RoomSelection;

enum class FlagModifyMode { SET, UNSET, TOGGLE };

using AddOneWayExit = AddExit;

class AddTwoWayExit final : public AddOneWayExit
{
public:
    explicit AddTwoWayExit(RoomId room1Id,
                           RoomId room2Id,
                           ExitDirection room1Dir,
                           ExitDirection in_room2Dir = ExitDirection::UNKNOWN)
        : AddOneWayExit(room1Id, room2Id, room1Dir)
        , room2Dir(in_room2Dir)
    {}

protected:
    virtual void exec() override;

    ExitDirection room2Dir = ExitDirection::UNKNOWN;
};

using RemoveOneWayExit = RemoveExit;

class RemoveTwoWayExit final : public RemoveOneWayExit
{
public:
    explicit RemoveTwoWayExit(RoomId room1Id,
                              RoomId room2Id,
                              ExitDirection room1Dir,
                              ExitDirection in_room2Dir = ExitDirection::UNKNOWN)
        : RemoveOneWayExit(room1Id, room2Id, room1Dir)
        , room2Dir(in_room2Dir)
    {}

protected:
    virtual void exec() override;

    ExitDirection room2Dir = ExitDirection::UNKNOWN;
};

class GroupMapAction final : virtual public MapAction
{
public:
    explicit GroupMapAction(AbstractAction *ex, const RoomSelection *selection);

    void schedule(MapFrontend *in) override { executor->setFrontend(in); }

    virtual ~GroupMapAction() { delete executor; }

protected:
    virtual void exec() override;

    virtual const RoomIdSet &getAffectedRooms() override;

private:
    std::list<RoomId> selectedRooms{};
    AbstractAction *executor = nullptr;
};

class MoveRelative final : public AbstractAction
{
public:
    explicit MoveRelative(const Coordinate &move);

    virtual void preExec(RoomId id) override;

    virtual void exec(RoomId id) override;

protected:
    Coordinate move;
};

class MergeRelative final : public Remove
{
public:
    explicit MergeRelative(const Coordinate &move);

    virtual void preExec(RoomId id) override;

    virtual void exec(RoomId id) override;

    virtual void insertAffected(RoomId id, RoomIdSet &affected) override;

protected:
    Coordinate move{};
};

class ConnectToNeighbours final : public AbstractAction
{
public:
    virtual void insertAffected(RoomId id, RoomIdSet &affected) override;

    virtual void exec(RoomId id) override;

private:
    void connectRooms(Room *center, Coordinate &otherPos, ExitDirection dir, RoomId cid);
};

class DisconnectFromNeighbours final : public ExitsAffecter
{
public:
    virtual void exec(RoomId id) override;
};

class ModifyRoomFlags final : public AbstractAction
{
public:
    explicit ModifyRoomFlags(uint flags, RoomField fieldNum, FlagModifyMode);

    virtual void exec(RoomId id) override;

protected:
    const uint flags = 0u;
    const RoomField fieldNum = RoomField::NAME;
    const FlagModifyMode mode{};
};

// Currently only used for DoorName, but it should work for any type.
class UpdateExitField final : public AbstractAction
{
public:
    explicit UpdateExitField(const ExitFieldVariant &update, ExitDirection dir);
    explicit UpdateExitField(const DoorName &update, ExitDirection dir);

    virtual void exec(RoomId id) override;

protected:
    const ExitFieldVariant update;
    const ExitDirection dir = ExitDirection::UNKNOWN;
};

// Despite its name, this is also used to modify an exit's DoorFlags.
// This has never been called with DoorName, so that part is not implemented.
class ModifyExitFlags final : public AbstractAction
{
public:
    explicit ModifyExitFlags(ExitFieldVariant flags, ExitDirection dir, FlagModifyMode);
    explicit ModifyExitFlags(ExitFlags flags, ExitDirection dir, FlagModifyMode);
    explicit ModifyExitFlags(DoorFlags flags, ExitDirection dir, FlagModifyMode);

    virtual void exec(RoomId id) override;

protected:
    const ExitFieldVariant var;
    const FlagModifyMode mode{};
    const ExitDirection dir = ExitDirection::UNKNOWN;
};

#endif
