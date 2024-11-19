#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "Crtp.h"
#include "ExitsFlags.h"
#include "RawExit.h"
#include "RoomFields.h"
#include "parseevent.h"

namespace detail {
template<typename Tag_>
struct NODISCARD TaggedRawRoom final : public RoomFieldsGetters<TaggedRawRoom<Tag_>>,
                                       public RoomExitFieldsGetters<TaggedRawRoom<Tag_>>,
                                       public RoomFieldsSetters<TaggedRawRoom<Tag_>>,
                                       public RoomExitFieldsSetters<TaggedRawRoom<Tag_>>
{
public:
    using ExitType = TaggedRawExit<Tag_>;
    using IdType = typename ExitType::IdType;
    using Exits = EnumIndexedArray<ExitType, ExitDirEnum, NUM_EXITS>;
    static constexpr const IdType INVALID_ID{INVALID_ROOMID.value()};

public:
    RoomFields fields;
    Exits exits;
    Coordinate position;
    IdType id = INVALID_ID;
    ServerRoomId server_id = INVALID_SERVER_ROOMID;
    RoomStatusEnum status = RoomStatusEnum::Temporary;

public:
    NODISCARD IdType getId() const { return id; }
    void setId(const IdType newId) { this->id = newId; }

public:
    NODISCARD ServerRoomId getServerId() const { return server_id; }
    void setServerId(const ServerRoomId newId) { this->server_id = newId; }

public:
    NODISCARD const Coordinate &getPosition() const { return position; }
    void setPosition(const Coordinate &c) { this->position = c; }

public:
    NODISCARD RoomFields &getRoomFields() { return fields; }
    NODISCARD const RoomFields &getRoomFields() const { return fields; }
    NODISCARD Exits &getExits() { return exits; }
    NODISCARD const Exits &getExits() const { return exits; }

public:
    NODISCARD ExitType &getExit(const ExitDirEnum dir) { return exits[dir]; }
    NODISCARD const ExitType &getExit(const ExitDirEnum dir) const { return exits[dir]; }

public:
    NODISCARD bool operator==(const TaggedRawRoom &rhs) const
    {
        return fields == rhs.fields && exits == rhs.exits && server_id == rhs.server_id
               && position == rhs.position && id == rhs.id && status == rhs.status;
    }
    NODISCARD bool operator!=(const TaggedRawRoom &rhs) const { return !(rhs == *this); }
    NODISCARD bool isTrivial() const { return *this == TaggedRawRoom{}; }
    NODISCARD bool hasExit(const ExitDirEnum dir) const { return !exits[dir].isTrivial(); }

    NODISCARD bool isTemporary() const { return status == RoomStatusEnum::Temporary; }
    NODISCARD bool isPermanent() const { return status == RoomStatusEnum::Permanent; }

public:
    NODISCARD std::string toStdStringUtf8() const;
};
} // namespace detail

using RawRoom = detail::TaggedRawRoom<tags::RoomIdTag>;
using ExternalRawRoom = detail::TaggedRawRoom<tags::ExternalRoomIdTag>;

extern void sanitize(ExternalRawRoom &);

template<>
NODISCARD std::string RawRoom::toStdStringUtf8() const;
template<>
NODISCARD std::string ExternalRawRoom::toStdStringUtf8() const;

NODISCARD extern ExitDirFlags computeExitDirections(const RawRoom &room);
NODISCARD extern ExitsFlagsType computeExitsFlags(const RawRoom &room);
