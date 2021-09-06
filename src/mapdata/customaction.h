#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <list>
#include <memory>

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

using AddOneWayExit = AddExit;

class NODISCARD AddTwoWayExit final : public AddOneWayExit
{
public:
    explicit AddTwoWayExit(RoomId room1Id,
                           RoomId room2Id,
                           ExitDirEnum room1Dir,
                           ExitDirEnum in_room2Dir)
        : AddOneWayExit(room1Id, room2Id, room1Dir)
        , room2Dir(in_room2Dir)
    {}

protected:
    void exec() override;

    ExitDirEnum room2Dir = ExitDirEnum::UNKNOWN;
};

using RemoveOneWayExit = RemoveExit;

class NODISCARD RemoveTwoWayExit final : public RemoveOneWayExit
{
public:
    explicit RemoveTwoWayExit(RoomId room1Id,
                              RoomId room2Id,
                              ExitDirEnum room1Dir,
                              ExitDirEnum in_room2Dir)
        : RemoveOneWayExit(room1Id, room2Id, room1Dir)
        , room2Dir(in_room2Dir)
    {}

protected:
    void exec() override;

    ExitDirEnum room2Dir = ExitDirEnum::UNKNOWN;
};

class NODISCARD GroupMapAction final : public MapAction
{
public:
    explicit GroupMapAction(std::unique_ptr<AbstractAction> ex,
                            const SharedRoomSelection &selection);

    void schedule(MapFrontend *in) override { executor->setFrontend(in); }

protected:
    void exec() override;

    NODISCARD const RoomIdSet &getAffectedRooms() override;

private:
    std::list<RoomId> selectedRooms{};
    std::unique_ptr<AbstractAction> executor;
};

class NODISCARD MoveRelative final : public AbstractAction
{
public:
    explicit MoveRelative(const Coordinate &move);

    void preExec(RoomId id) override;

    void exec(RoomId id) override;

protected:
    Coordinate move;
};

class NODISCARD MergeRelative final : public Remove
{
public:
    explicit MergeRelative(const Coordinate &move);

    void preExec(RoomId id) override;

    void exec(RoomId id) override;

    void insertAffected(RoomId id, RoomIdSet &affected) override;

protected:
    Coordinate move;
};

class NODISCARD ConnectToNeighbours final : public AbstractAction
{
public:
    void insertAffected(RoomId id, RoomIdSet &affected) override;

    void exec(RoomId id) override;

private:
    void connectRooms(Room *center, Coordinate &otherPos, ExitDirEnum dir, RoomId cid);
};

class NODISCARD ModifyRoomFlags final : public AbstractAction
{
public:
    explicit ModifyRoomFlags(RoomFieldVariant, FlagModifyModeEnum);

#define NOP()
#define X_DECLARE_CONSTRUCTORS(UPPER_CASE, CamelCase, Type) \
    explicit ModifyRoomFlags(Type type, FlagModifyModeEnum in_mode) \
        : ModifyRoomFlags{RoomFieldVariant{type}, in_mode} \
    {}
    X_FOREACH_ROOM_FIELD(X_DECLARE_CONSTRUCTORS, NOP)
#undef X_DECLARE_CONSTRUCTORS
#undef NOP

    void exec(RoomId id) override;

protected:
    const RoomFieldVariant var;
    const FlagModifyModeEnum mode;
};

class NODISCARD ModifyRoomUpToDate final : public AbstractAction
{
public:
    explicit ModifyRoomUpToDate(bool checked);

    void exec(RoomId id) override;

protected:
    const bool checked = false;
};

// Despite its name, this is also used to modify an exit's DoorFlags and door names
class NODISCARD ModifyExitFlags final : public AbstractAction
{
public:
    explicit ModifyExitFlags(ExitFieldVariant, ExitDirEnum, FlagModifyModeEnum);

#define NOP()
#define X_DECLARE_CONSTRUCTORS(UPPER_CASE, Type) \
    explicit ModifyExitFlags(Type type, ExitDirEnum dir, FlagModifyModeEnum in_mode) \
        : ModifyExitFlags{ExitFieldVariant{type}, dir, in_mode} \
    {}
    X_FOREACH_EXIT_FIELD(X_DECLARE_CONSTRUCTORS, NOP)
#undef X_DECLARE_CONSTRUCTORS
#undef NOP

    void exec(RoomId id) override;

protected:
    const ExitFieldVariant var;
    const FlagModifyModeEnum mode;
    const ExitDirEnum dir = ExitDirEnum::UNKNOWN;
};
