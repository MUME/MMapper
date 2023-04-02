#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)

#include <cstdint>
#include <optional>

#include "hash.h"

class QDataStream;

/**
 * Room id provided by MUME server.
 * Although it serves the same purpose as MMapper's RoomId, it is a separate type for two reasons:
 *
 * 1. MUME server started sending its (obfuscated) room ids only on 2023-04-01
 * 2. They can be omitted in mazes, when a character is blinded or cannot see in the dark, etc.
 */
struct NODISCARD RoomServerId final
{
private:
    std::optional<uint64_t> value;

public:
    RoomServerId() = default;
    constexpr explicit RoomServerId(uint64_t value) noexcept
        : value{value}
    {}
    inline constexpr bool isSet() const { return bool(value); }
    /// @throws std::bad_optional_access if !isSet()
    inline uint64_t asUint64() const { return value.value(); }

    inline constexpr bool operator<(const RoomServerId &rhs) const { return value < rhs.value; }
    inline constexpr bool operator>(const RoomServerId &rhs) const { return value > rhs.value; }
    inline constexpr bool operator<=(const RoomServerId &rhs) const { return value <= rhs.value; }
    inline constexpr bool operator>=(const RoomServerId &rhs) const { return value >= rhs.value; }
    inline constexpr bool operator==(const RoomServerId &rhs) const { return value == rhs.value; }
    inline constexpr bool operator!=(const RoomServerId &rhs) const { return value != rhs.value; }
};
static constexpr const RoomServerId UNKNOWN_ROOMSERVERID{};

template<>
struct std::hash<RoomServerId>
{
    std::size_t operator()(const RoomServerId &id) const noexcept
    {
        return numeric_hash(id.isSet() ? id.asUint64() : uint64_t(-1));
    }
};

QDataStream &operator<<(QDataStream &os, const RoomServerId &id);
QDataStream &operator>>(QDataStream &os, RoomServerId &id);
