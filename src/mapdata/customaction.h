#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <list>
#include <memory>
#include <QVariant>
#include <QtCore>
#include <QtGlobal>

#include "../expandoracommon/coordinate.h"
#include "../global/roomid.h"
#include "../mapfrontend/mapaction.h"
#include "ExitDirection.h"
#include "ExitFieldVariant.h"
#include "RoomFieldVariant.h"
#include "mmapper2exit.h"
#include "mmapper2room.h"
#include "roomselection.h"

class DoorFlags;
class ExitFlags;
class MapData;
class MapFrontend;
class ParseEvent;
class Room;

enum class FlagModifyModeEnum { SET, UNSET, TOGGLE };

using AddOneWayExit = AddExit;

class AddTwoWayExit final : public AddOneWayExit
{
public:
    explicit AddTwoWayExit(RoomId room1Id,
                           RoomId room2Id,
                           ExitDirEnum room1Dir,
                           ExitDirEnum in_room2Dir = ExitDirEnum::UNKNOWN)
        : AddOneWayExit(room1Id, room2Id, room1Dir)
        , room2Dir(in_room2Dir)
    {}

protected:
    virtual void exec() override;

    ExitDirEnum room2Dir = ExitDirEnum::UNKNOWN;
};

using RemoveOneWayExit = RemoveExit;

class RemoveTwoWayExit final : public RemoveOneWayExit
{
public:
    explicit RemoveTwoWayExit(RoomId room1Id,
                              RoomId room2Id,
                              ExitDirEnum room1Dir,
                              ExitDirEnum in_room2Dir = ExitDirEnum::UNKNOWN)
        : RemoveOneWayExit(room1Id, room2Id, room1Dir)
        , room2Dir(in_room2Dir)
    {}

protected:
    virtual void exec() override;

    ExitDirEnum room2Dir = ExitDirEnum::UNKNOWN;
};

class GroupMapAction final : virtual public MapAction
{
public:
    explicit GroupMapAction(std::unique_ptr<AbstractAction> ex,
                            const SharedRoomSelection &selection);

    void schedule(MapFrontend *in) override { executor->setFrontend(in); }

protected:
    virtual void exec() override;

    virtual const RoomIdSet &getAffectedRooms() override;

private:
    std::list<RoomId> selectedRooms{};
    std::unique_ptr<AbstractAction> executor;
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
    Coordinate move;
};

class ConnectToNeighbours final : public AbstractAction
{
public:
    virtual void insertAffected(RoomId id, RoomIdSet &affected) override;

    virtual void exec(RoomId id) override;

private:
    void connectRooms(Room *center, Coordinate &otherPos, ExitDirEnum dir, RoomId cid);
};

class ModifyRoomFlags final : public AbstractAction
{
public:
    explicit ModifyRoomFlags(RoomFieldVariant flags, FlagModifyModeEnum);
    explicit ModifyRoomFlags(RoomMobFlags flags, FlagModifyModeEnum);
    explicit ModifyRoomFlags(RoomLoadFlags flags, FlagModifyModeEnum);

    virtual void exec(RoomId id) override;

protected:
    const RoomFieldVariant var;
    const FlagModifyModeEnum mode;
};

class ModifyRoomUpToDate final : public AbstractAction
{
public:
    explicit ModifyRoomUpToDate(bool checked);

    virtual void exec(RoomId id) override;

protected:
    const bool checked = false;
};

// Currently only used for DoorName, but it should work for any type.
class UpdateExitField final : public AbstractAction
{
public:
    explicit UpdateExitField(const DoorName &update, ExitDirEnum dir);

    virtual void exec(RoomId id) override;

protected:
    const ExitFieldVariant update;
    const ExitDirEnum dir = ExitDirEnum::UNKNOWN;
};

// Despite its name, this is also used to modify an exit's DoorFlags.
// This has never been called with DoorName, so that part is not implemented.
class ModifyExitFlags final : public AbstractAction
{
public:
    explicit ModifyExitFlags(ExitFieldVariant flags, ExitDirEnum dir, FlagModifyModeEnum);
    explicit ModifyExitFlags(ExitFlags flags, ExitDirEnum dir, FlagModifyModeEnum);
    explicit ModifyExitFlags(DoorFlags flags, ExitDirEnum dir, FlagModifyModeEnum);

    virtual void exec(RoomId id) override;

protected:
    const ExitFieldVariant var;
    const FlagModifyModeEnum mode;
    const ExitDirEnum dir = ExitDirEnum::UNKNOWN;
};
