#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "Crtp.h"
#include "ExitFields.h"
#include "InOutEnum.h"
#include "TinyRoomIdSet.h"

namespace detail {
template<typename Tag_>
struct NODISCARD TaggedRawExit final : public ExitFieldsGetters<TaggedRawExit<Tag_>>,
                                       public ExitFieldsSetters<TaggedRawExit<Tag_>>
{
public:
    using IdType = std::conditional_t<
        std::is_same_v<Tag_, ::tags::RoomIdTag>,
        RoomId,
        std::conditional_t<std::is_same_v<Tag_, ::tags::ExternalRoomIdTag>, ExternalRoomId, void>>;
    using SetType
        = std::conditional_t<std::is_same_v<Tag_, ::tags::RoomIdTag>,
                             TinyRoomIdSet,
                             std::conditional_t<std::is_same_v<Tag_, ::tags::ExternalRoomIdTag>,
                                                TinyExternalRoomIdSet,
                                                void>>;

public:
    ExitFields fields;
    SetType outgoing;
    SetType incoming;

public:
    NODISCARD ExitFields &getExitFields() { return fields; }
    NODISCARD const ExitFields &getExitFields() const { return fields; }

public:
    NODISCARD SetType &getOutgoingSet() { return outgoing; }
    NODISCARD const SetType &getOutgoingSet() const { return outgoing; }

public:
    NODISCARD SetType &getIncomingSet() { return incoming; }
    NODISCARD const SetType &getIncomingSet() const { return incoming; }

public:
    NODISCARD SetType &getInOut(const InOutEnum mode)
    {
        return (mode == InOutEnum::Out) ? outgoing : incoming;
    }
    NODISCARD const SetType &getInOut(const InOutEnum mode) const
    {
        return (mode == InOutEnum::Out) ? outgoing : incoming;
    }

public:
    NODISCARD bool operator==(const TaggedRawExit &rhs) const
    {
        return fields == rhs.fields && outgoing == rhs.outgoing && incoming == rhs.incoming;
    }
    NODISCARD bool operator!=(const TaggedRawExit &rhs) const { return !(rhs == *this); }
    NODISCARD bool isTrivial() const { return *this == TaggedRawExit{}; }
};
} // namespace detail

using RawExit = detail::TaggedRawExit<tags::RoomIdTag>;
using ExternalRawExit = detail::TaggedRawExit<tags::ExternalRoomIdTag>;
