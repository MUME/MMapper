#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)

#include <cstdint>
#include <optional>

#include "hash.h"

/**
 * Room id provided by MUME server.
 * Although it serves the same purpose as MMapper's RoomId, it is a separate type for two reasons:
 *
 * 1. MUME server started sending its (obfuscated) room ids only on 2023-04-01
 * 2. They can be omitted in mazes, when a character is blinded or cannot see in the dark, etc.
 */
struct NODISCARD ServerRoomId final
{
private:
    std::optional<uint64_t> value;

public:
    ServerRoomId() = default;
    constexpr explicit ServerRoomId(uint64_t value) noexcept
        : value{value}
    {}
    inline constexpr bool isSet() const { return bool(value); }
    /// @throws std::bad_optional_access if !isSet()
    inline uint64_t asUint64() const { return value.value(); }

    inline constexpr bool operator<(const ServerRoomId &rhs) const { return value < rhs.value; }
    inline constexpr bool operator>(const ServerRoomId &rhs) const { return value > rhs.value; }
    inline constexpr bool operator<=(const ServerRoomId &rhs) const { return value <= rhs.value; }
    inline constexpr bool operator>=(const ServerRoomId &rhs) const { return value >= rhs.value; }
    inline constexpr bool operator==(const ServerRoomId &rhs) const { return value == rhs.value; }
    inline constexpr bool operator!=(const ServerRoomId &rhs) const { return value != rhs.value; }
};
static constexpr const ServerRoomId UNKNOWN_SERVERROOMID{};

template<>
struct std::hash<ServerRoomId>
{
    std::size_t operator()(const ServerRoomId &id) const noexcept
    {
        return numeric_hash(id.isSet() ? id.asUint64() : uint64_t(-1));
    }
};

QDataStream &operator<<(QDataStream &os, const ServerRoomId &id);
QDataStream &operator>>(QDataStream &os, ServerRoomId &id);
