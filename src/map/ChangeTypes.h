#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "DoorFlags.h"
#include "ExitDirection.h"
#include "ExitFieldVariant.h"
#include "ExitFlags.h"
#include "Map.h"
#include "RoomHandle.h"
#include "infomark.h"
#include "mmapper2room.h"
#include "roomid.h"

#include <utility>

#define XFOREACH_ChangeTypeEnum(X) \
    X(Add) \
    X(Remove)
#define XFOREACH_FlagChangeEnum(X) \
    X(Set) \
    X(Add) \
    X(Remove)
#define XFOREACH_PositionChangeEnum(X) \
    X(Exact) \
    X(Relative)
#define XFOREACH_UpdateTypeEnum(X) \
    X(New) \
    X(Force) \
    X(Update)
#define XFOREACH_WaysEnum(X) \
    X(OneWay) \
    X(TwoWay)
#define X_DECL(X) X,
enum class NODISCARD ChangeTypeEnum { XFOREACH_ChangeTypeEnum(X_DECL) };
enum class NODISCARD FlagChangeEnum { XFOREACH_FlagChangeEnum(X_DECL) };
enum class NODISCARD PositionChangeEnum { XFOREACH_PositionChangeEnum(X_DECL) };
enum class NODISCARD UpdateTypeEnum { XFOREACH_UpdateTypeEnum(X_DECL) };
enum class NODISCARD WaysEnum { XFOREACH_WaysEnum(X_DECL) };
#undef X_DECL

namespace world_change_types {

struct NODISCARD CompactRoomIds final
{
    ExternalRoomId firstId{0};
};

struct NODISCARD RemoveAllDoorNames final
{};

struct NODISCARD GenerateBaseMap final
{};

}; // namespace world_change_types

namespace room_change_types {

struct NODISCARD AddPermanentRoom final
{
    Coordinate position;
};

struct NODISCARD AddRoom2 final
{
    Coordinate position;
    ParseEvent event;
    explicit AddRoom2(const Coordinate &pos, ParseEvent ev)
        : position{pos}
        , event{std::move(ev)}
    {}
};

struct NODISCARD RemoveRoom final
{
    RoomId room = INVALID_ROOMID;
};

struct NODISCARD UndeleteRoom final
{
    ExternalRoomId room = INVALID_EXTERNAL_ROOMID;
    RawRoom raw;
};

struct NODISCARD MakePermanent final
{
    RoomId room = INVALID_ROOMID;
};

struct NODISCARD Update final
{
    RoomId room = INVALID_ROOMID;
    ParseEvent event;
    UpdateTypeEnum type = UpdateTypeEnum::Update;
    explicit Update(const RoomId id, ParseEvent ev, const UpdateTypeEnum in_type)
        : room{id}
        , event{std::move(ev)}
        , type{in_type}
    {}
};

struct NODISCARD SetServerId final
{
    RoomId room = INVALID_ROOMID;
    ServerRoomId server_id = INVALID_SERVER_ROOMID;
};

struct NODISCARD SetScaleFactor final
{
    RoomId room = INVALID_ROOMID;
    float scale = 1.0f;
};

struct NODISCARD MoveRelative final
{
    RoomId room = INVALID_ROOMID;
    Coordinate offset;
};

struct NODISCARD MoveRelative2 final
{
    RoomIdSet rooms;
    Coordinate offset;
};

struct NODISCARD MergeRelative final
{
    RoomId room = INVALID_ROOMID;
    Coordinate offset;
};

struct NODISCARD ModifyRoomFlags final
{
    RoomId room = INVALID_ROOMID;
    RoomFieldVariant field;
    FlagModifyModeEnum mode = FlagModifyModeEnum::ASSIGN;
    ModifyRoomFlags(const RoomId _room, RoomFieldVariant _field, const FlagModifyModeEnum _mode)
        : room{_room}
        , field{std::move(_field)}
        , mode{_mode}
    {}

#define X_NOP()
#define X_DECLARE_CONSTRUCTORS(UPPER_CASE, CamelCase, Type) \
    explicit ModifyRoomFlags(const RoomId id, Type type, const FlagModifyModeEnum in_mode) \
        : ModifyRoomFlags{id, RoomFieldVariant{std::move(type)}, in_mode} \
    {}
    XFOREACH_ROOM_FIELD(X_DECLARE_CONSTRUCTORS, X_NOP)
#undef X_DECLARE_CONSTRUCTORS
#undef X_NOP
};

// NOTE: The movement will occur with "best effort" only;
// no position change is guaranteed,
// but it will attempt to keep the requested z layer.
struct NODISCARD TryMoveCloseTo final
{
    RoomId room = INVALID_ROOMID;
    Coordinate desiredPosition;
};

} // namespace room_change_types

namespace exit_change_types {

// NOTE: Use NukeExit if you want to remove a connection entirely
struct NODISCARD ModifyExitConnection final
{
    ChangeTypeEnum type = ChangeTypeEnum::Add;
    RoomId room = INVALID_ROOMID;
    ExitDirEnum dir = ExitDirEnum::NONE;
    RoomId to = INVALID_ROOMID;
    WaysEnum ways = WaysEnum::OneWay;
};

struct NODISCARD ModifyExitFlags final
{
    RoomId room = INVALID_ROOMID;
    ExitDirEnum dir = ExitDirEnum::NONE;
    ExitFieldVariant field;
    FlagModifyModeEnum mode = FlagModifyModeEnum::ASSIGN;

    DEPRECATED_MSG("swap type and dir")
    ModifyExitFlags(RoomId room_,
                    ExitFieldVariant moved_field,
                    ExitDirEnum dir_,
                    FlagModifyModeEnum mode_)
        : room{room_}
        , dir{dir_}
        , field{std::move(moved_field)}
        , mode{mode_}
    {}

    ModifyExitFlags(RoomId room_,
                    ExitDirEnum dir_,
                    ExitFieldVariant moved_field,
                    FlagModifyModeEnum mode_)
        : room{room_}
        , dir{dir_}
        , field{std::move(moved_field)}
        , mode{mode_}
    {}

#define X_NOP()
#define X_DECLARE_CONSTRUCTORS(UPPER_CASE, Type) \
    DEPRECATED_MSG("swap type and dir") \
    explicit ModifyExitFlags(const RoomId id_, \
                             Type moved_type, \
                             const ExitDirEnum dir_, \
                             const FlagModifyModeEnum in_mode) \
        : ModifyExitFlags{id_, dir_, ExitFieldVariant{std::move(moved_type)}, in_mode} \
    {} \
    explicit ModifyExitFlags(const RoomId id_, \
                             const ExitDirEnum dir_, \
                             Type moved_type, \
                             FlagModifyModeEnum in_mode) \
        : ModifyExitFlags{id_, dir_, ExitFieldVariant{std::move(moved_type)}, in_mode} \
    {}
    XFOREACH_EXIT_FIELD(X_DECLARE_CONSTRUCTORS, X_NOP)
#undef X_DECLARE_CONSTRUCTORS
#undef X_NOP
};

struct NODISCARD NukeExit final
{
    RoomId room = INVALID_ROOMID;
    ExitDirEnum dir = ExitDirEnum::NONE;
    WaysEnum ways = WaysEnum::OneWay;
};

struct NODISCARD SetExitFlags final
{
    FlagChangeEnum type = FlagChangeEnum::Set;
    RoomId room = INVALID_ROOMID;
    ExitDirEnum dir = ExitDirEnum::NONE;
    ExitFlags flags;
};

struct NODISCARD SetDoorFlags final
{
    FlagChangeEnum type = FlagChangeEnum::Set;
    RoomId room = INVALID_ROOMID;
    ExitDirEnum dir = ExitDirEnum::NONE;
    DoorFlags flags;
};

struct NODISCARD SetDoorName final
{
    RoomId room = INVALID_ROOMID;
    ExitDirEnum dir = ExitDirEnum::NONE;
    DoorName name;
};

} // namespace exit_change_types

namespace infomark_change_types {

struct NODISCARD AddInfomark
{
    RawInfomark fields;
};

struct NODISCARD UpdateInfomark
{
    InfomarkId id = INVALID_INFOMARK_ID;
    RawInfomark fields;
};

struct NODISCARD RemoveInfomark
{
    InfomarkId id = INVALID_INFOMARK_ID;
};

} // namespace infomark_change_types

namespace types {
namespace world = world_change_types;
namespace rooms = room_change_types;
namespace exits = exit_change_types;
namespace infomarks = infomark_change_types;
} // namespace types

struct NODISCARD ConnectToNeighborsArgs final
{
    WaysEnum ways = WaysEnum::TwoWay;
};

#define XFOREACH_WORLD_CHANGE_TYPES(X, SEP) \
    X(world_change_types::CompactRoomIds) \
    SEP() \
    X(world_change_types::RemoveAllDoorNames) \
    SEP() \
    X(world_change_types::GenerateBaseMap)

#define XFOREACH_ROOM_CHANGE_TYPES(X, SEP) \
    X(room_change_types::AddPermanentRoom) \
    SEP() \
    X(room_change_types::AddRoom2) \
    SEP() \
    X(room_change_types::MakePermanent) \
    SEP() \
    X(room_change_types::MergeRelative) \
    SEP() \
    X(room_change_types::ModifyRoomFlags) \
    SEP() \
    X(room_change_types::MoveRelative) \
    SEP() \
    X(room_change_types::MoveRelative2) \
    SEP() \
    X(room_change_types::RemoveRoom) \
    SEP() \
    X(room_change_types::SetScaleFactor) \
    SEP() \
    X(room_change_types::SetServerId) \
    SEP() \
    X(room_change_types::TryMoveCloseTo) \
    SEP() \
    X(room_change_types::UndeleteRoom) \
    SEP() \
    X(room_change_types::Update)

#define XFOREACH_EXIT_CHANGE_TYPES(X, SEP) \
    X(exit_change_types::ModifyExitConnection) \
    SEP() \
    X(exit_change_types::ModifyExitFlags) \
    SEP() \
    X(exit_change_types::NukeExit) \
    SEP() \
    X(exit_change_types::SetDoorFlags) \
    SEP() \
    X(exit_change_types::SetDoorName) \
    SEP() \
    X(exit_change_types::SetExitFlags)

#define XFOREACH_INFOMARK_CHANGE_TYPES(X, SEP) \
    X(infomark_change_types::AddInfomark) \
    SEP() \
    X(infomark_change_types::UpdateInfomark) \
    SEP() \
    X(infomark_change_types::RemoveInfomark)

#define XFOREACH_CHANGE_TYPE(X, SEP) \
    XFOREACH_WORLD_CHANGE_TYPES(X, SEP) \
    SEP() \
    XFOREACH_ROOM_CHANGE_TYPES(X, SEP) \
    SEP() \
    XFOREACH_EXIT_CHANGE_TYPES(X, SEP) \
    SEP() \
    XFOREACH_INFOMARK_CHANGE_TYPES(X, SEP)
