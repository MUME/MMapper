#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)

#include <cstdint>

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
    /**
     * The command "help xml" in MUME states:
     *
     * room    room section; has the following attributes:
     *   terrain="<terrain>" [...]
     *   area="<area>"   [...]
     *   id=<id> (optional) room number (an integer in [1..0x7fffffff]);
     *           all rooms do not have numbers.
     *
     * Thus we can use 0 as "unknown RoomServerId"
     */
    uint32_t value;

public:
    constexpr RoomServerId() noexcept
        : value{0}
    {}
    constexpr explicit RoomServerId(uint32_t value) noexcept
        : value{value}
    {}
    inline constexpr bool isSet() const { return value != 0; }
    inline uint32_t asUint32() const { return value; }

    inline constexpr bool operator<(RoomServerId rhs) const { return value < rhs.value; }
    inline constexpr bool operator>(RoomServerId rhs) const { return value > rhs.value; }
    inline constexpr bool operator<=(RoomServerId rhs) const { return value <= rhs.value; }
    inline constexpr bool operator>=(RoomServerId rhs) const { return value >= rhs.value; }
    inline constexpr bool operator==(RoomServerId rhs) const { return value == rhs.value; }
    inline constexpr bool operator!=(RoomServerId rhs) const { return value != rhs.value; }
};
static constexpr const RoomServerId UNKNOWN_ROOMSERVERID{};

template<>
struct std::hash<RoomServerId>
{
    std::size_t operator()(RoomServerId id) const noexcept { return numeric_hash(id.asUint32()); }
};

QDataStream &operator<<(QDataStream &os, RoomServerId id);
QDataStream &operator>>(QDataStream &os, RoomServerId &id);
