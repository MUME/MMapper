#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/TaggedInt.h"
#include "../global/hash.h"
#include "../global/macros.h"

#include <climits>
#include <cstdint>
#include <ostream>

class AnsiOstream;

namespace tags {
struct NODISCARD RoomIdTag final
{};
struct NODISCARD ExternalRoomIdTag final
{};
struct NODISCARD ServerRoomIdTag final
{};
} // namespace tags

struct NODISCARD RoomId final : public TaggedInt<RoomId, tags::RoomIdTag, uint32_t>
{
    using TaggedInt::TaggedInt;
    constexpr RoomId()
        : RoomId{UINT_MAX}
    {}
    NODISCARD constexpr uint32_t asUint32() const { return value(); }
    friend std::ostream &operator<<(std::ostream &os, RoomId id);
    friend AnsiOstream &operator<<(AnsiOstream &os, RoomId id);
};

struct NODISCARD ExternalRoomId final
    : public TaggedInt<ExternalRoomId, tags::ExternalRoomIdTag, uint32_t>
{
    using TaggedInt::TaggedInt;
    constexpr ExternalRoomId()
        : ExternalRoomId{UINT_MAX}
    {}
    NODISCARD constexpr uint32_t asUint32() const { return value(); }
    friend std::ostream &operator<<(std::ostream &os, ExternalRoomId id);
    friend AnsiOstream &operator<<(AnsiOstream &os, ExternalRoomId id);
};

struct NODISCARD ServerRoomId final
    : public TaggedInt<ServerRoomId, tags::ServerRoomIdTag, uint32_t>
{
    using TaggedInt::TaggedInt;
    constexpr ServerRoomId()
        : ServerRoomId{0}
    {}
    NODISCARD constexpr uint32_t asUint32() const { return value(); }
    friend std::ostream &operator<<(std::ostream &os, ServerRoomId id);
    friend AnsiOstream &operator<<(AnsiOstream &os, ServerRoomId id);
};

static_assert(sizeof(RoomId) == sizeof(uint32_t));
static constexpr const RoomId INVALID_ROOMID{UINT_MAX};
static_assert(RoomId{} == INVALID_ROOMID);

static_assert(sizeof(ExternalRoomId) == sizeof(uint32_t));
static constexpr const ExternalRoomId INVALID_EXTERNAL_ROOMID{UINT_MAX};
static_assert(ExternalRoomId{} == INVALID_EXTERNAL_ROOMID);

static_assert(sizeof(ServerRoomId) == sizeof(uint32_t));
static constexpr const ServerRoomId INVALID_SERVER_ROOMID{0};
static_assert(ServerRoomId{} == INVALID_SERVER_ROOMID);

template<>
struct std::hash<RoomId>
{
    std::size_t operator()(const RoomId id) const noexcept { return numeric_hash(id.asUint32()); }
};
template<>
struct std::hash<ExternalRoomId>
{
    std::size_t operator()(const ExternalRoomId &id) const noexcept
    {
        return numeric_hash(id.asUint32());
    }
};
template<>
struct std::hash<ServerRoomId>
{
    std::size_t operator()(const ServerRoomId &id) const noexcept
    {
        return numeric_hash(id.asUint32());
    }
};
