#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <climits>
#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <vector>

#include "hash.h"

struct NODISCARD RoomId final
{
private:
    uint32_t value = 0;

public:
    RoomId() = default;
    constexpr explicit RoomId(uint32_t value) noexcept
        : value{value}
    {}
    inline constexpr explicit operator uint32_t() const { return value; }
    inline constexpr uint32_t asUint32() const { return value; }
    inline constexpr bool operator<(RoomId rhs) const { return value < rhs.value; }
    inline constexpr bool operator>(RoomId rhs) const { return value > rhs.value; }
    inline constexpr bool operator<=(RoomId rhs) const { return value <= rhs.value; }
    inline constexpr bool operator>=(RoomId rhs) const { return value >= rhs.value; }
    inline constexpr bool operator==(RoomId rhs) const { return value == rhs.value; }
    inline constexpr bool operator!=(RoomId rhs) const { return value != rhs.value; }

public:
    // TODO: Is this still needed?
    friend inline uint32_t qHash(RoomId id) { return id.asUint32(); }
};
static constexpr const RoomId INVALID_ROOMID{std::numeric_limits<uint32_t>::max()};
static constexpr const RoomId DEFAULT_ROOMID{0};

using RoomIdSet = std::set<RoomId>;

template<>
struct std::hash<RoomId>
{
    std::size_t operator()(const RoomId &id) const noexcept { return numeric_hash(id.asUint32()); }
};

template<typename T>
class NODISCARD roomid_vector : private std::vector<T>
{
private:
    using base = std::vector<T>;

public:
    using std::vector<T>::vector;

public:
    NODISCARD decltype(auto) operator[](RoomId roomId) { return base::at(roomId.asUint32()); }
    NODISCARD decltype(auto) operator[](RoomId roomId) const { return base::at(roomId.asUint32()); }

public:
    using base::begin;
    using base::end;

public:
    using base::resize;
    using base::size;
};

class Room;
using RoomIndex = roomid_vector<std::shared_ptr<Room>>;

class RoomRecipient;
using RoomLocks = roomid_vector<std::set<RoomRecipient *>>;

class RoomCollection;
using SharedRoomCollection = std::shared_ptr<RoomCollection>;
using RoomHomes = roomid_vector<SharedRoomCollection>;
