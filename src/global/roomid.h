#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include <climits>
#include <cstdint>
#include <memory>
#include <set>
#include <vector>

struct RoomId final
{
private:
    uint32_t value{};

public:
    explicit RoomId() = default;
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
    friend inline uint32_t qHash(RoomId id) { return id.asUint32(); }
};
static constexpr const RoomId INVALID_ROOMID{UINT_MAX};
static constexpr const RoomId DEFAULT_ROOMID{0};

using RoomIdSet = std::set<RoomId>;

template<typename T>
class roomid_vector : private std::vector<T>
{
private:
    using base = std::vector<T>;

public:
    using std::vector<T>::vector;

public:
    auto operator[](RoomId roomId) -> decltype(auto) { return base::at(roomId.asUint32()); }
    auto operator[](RoomId roomId) const -> decltype(auto) { return base::at(roomId.asUint32()); }

public:
    using base::begin;
    using base::end;

public:
    using base::resize;
    using base::size;
};

class Room;
using RoomIndex = roomid_vector<Room *>;

class RoomRecipient;
using RoomLocks = roomid_vector<std::set<RoomRecipient *>>;

struct IRoomCollection;
class RoomCollection;
using SharedRoomCollection = std::shared_ptr<IRoomCollection>;
using RoomHomes = roomid_vector<SharedRoomCollection>;
