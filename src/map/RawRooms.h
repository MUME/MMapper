#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../global/IndexedVector.h"
#include "../global/macros.h"
#include "InvalidMapOperation.h"
#include "RawExit.h"
#include "RawRoom.h"

class NODISCARD RawRooms final
{
private:
    IndexedVector<RawRoom, RoomId> m_rooms;

public:
    NODISCARD RawRoom &getRawRoomRef(RoomId pos) { return m_rooms.at(pos); }
    NODISCARD const RawRoom &getRawRoomRef(RoomId pos) const { return m_rooms.at(pos); }

    NODISCARD auto begin() const { return m_rooms.begin(); }
    NODISCARD auto end() const { return m_rooms.end(); }

public:
    NODISCARD size_t size() const { return m_rooms.size(); }
    void resize(const size_t numRooms) { m_rooms.resize(numRooms); }

    void removeAt(const RoomId id) { getRawRoomRef(id) = RawRoom{}; }

    void requireUninitialized(const RoomId id) const
    {
        if (getRawRoomRef(id) != RawRoom{}) {
            throw InvalidMapOperation();
        }
    }

public:
#define X_DECL_ACCESSORS(_Type, _Name, _Init) \
    NODISCARD const _Type &getRoom##_Name(const RoomId id) const \
    { \
        return getRawRoomRef(id).fields._Name; \
    } \
    void setRoom##_Name(const RoomId id, _Type x) \
    { \
        if (getRoom##_Name(id) != x) { \
            getRawRoomRef(id).fields._Name = std::move(x); \
        } \
    }

    XFOREACH_ROOM_STRING_PROPERTY(X_DECL_ACCESSORS)
    XFOREACH_ROOM_FLAG_PROPERTY(X_DECL_ACCESSORS)
    XFOREACH_ROOM_ENUM_PROPERTY(X_DECL_ACCESSORS)
#undef X_DECL_ACCESSORS

public:
#define X_DEFINE_ACCESSOR(_Type, _Name, _Init) \
    void setExit##_Type(const RoomId id, const ExitDirEnum dir, _Type x) \
    { \
        if (getExit##_Type(id, dir) != x) { \
            getRawRoomRef(id).getExit(dir).fields._Name = std::move(x); \
        } \
    } \
    NODISCARD const _Type &getExit##_Type(const RoomId id, const ExitDirEnum dir) const \
    { \
        return getRawRoomRef(id).getExit(dir).fields._Name; \
    }
    XFOREACH_EXIT_PROPERTY(X_DEFINE_ACCESSOR)
#undef X_DEFINE_ACCESSOR

public:
    void setExitOutgoing(const RoomId id, const ExitDirEnum dir, const TinyRoomIdSet &set)
    {
        getRawRoomRef(id).getExit(dir).outgoing = set;
        enforceInvariants(id, dir);
    }
    NODISCARD const TinyRoomIdSet &getExitOutgoing(const RoomId id, const ExitDirEnum dir) const
    {
        return getRawRoomRef(id).getExit(dir).outgoing;
    }

public:
    void setExitIncoming(const RoomId id, const ExitDirEnum dir, const TinyRoomIdSet &set)
    {
        getRawRoomRef(id).getExit(dir).incoming = set;
    }
    NODISCARD const TinyRoomIdSet &getExitIncoming(const RoomId id, const ExitDirEnum dir) const
    {
        return getRawRoomRef(id).getExit(dir).incoming;
    }

public:
    void setExitFlags_safe(RoomId id, ExitDirEnum dir, ExitFlags input_flags);

public:
    void enforceInvariants(RoomId id, ExitDirEnum dir);
    void enforceInvariants(RoomId id);

public:
    NODISCARD bool satisfiesInvariants(RoomId id, ExitDirEnum dir) const;
    NODISCARD bool satisfiesInvariants(RoomId id) const;

public:
    NODISCARD ExitFlags getExitFlags(const RoomId id, const ExitDirEnum dir) const
    {
        return getExitExitFlags(id, dir);
    }
    void setExitInOut(const RoomId id,
                      const ExitDirEnum dir,
                      const InOutEnum inOut,
                      const TinyRoomIdSet &set)
    {
        const bool isOut = inOut == InOutEnum::Out;
        auto &data = getRawRoomRef(id).getExit(dir);
        (isOut ? data.outgoing : data.incoming) = set;
        if (isOut) {
            enforceInvariants(id, dir);
        }
    }

    NODISCARD const TinyRoomIdSet &getExitInOut(const RoomId id,
                                                const ExitDirEnum dir,
                                                const InOutEnum inOut) const
    {
        const bool isOut = inOut == InOutEnum::Out;
        const auto &data = getRawRoomRef(id).getExit(dir);
        return isOut ? data.outgoing : data.incoming;
    }

public:
    void setServerId(const RoomId id, const ServerRoomId serverId)
    {
        getRawRoomRef(id).server_id = serverId;
    }
    NODISCARD const ServerRoomId &getServerId(const RoomId id) const
    {
        return getRawRoomRef(id).server_id;
    }

public:
    void setPosition(const RoomId id, const Coordinate &coord)
    {
        getRawRoomRef(id).position = coord;
    }
    NODISCARD const Coordinate &getPosition(const RoomId id) const
    {
        return getRawRoomRef(id).position;
    }

public:
    NODISCARD RoomStatusEnum getStatus(const RoomId id) const
    {
        return getRawRoomRef(id).status;
    }
    void setStatus(const RoomId id, RoomStatusEnum status)
    {
        if (status != getStatus(id)) {
            getRawRoomRef(id).status = status;
        }
    }

public:
    NODISCARD bool operator==(const RawRooms &rhs) const
    {
        return m_rooms == rhs.m_rooms;
    }
    NODISCARD bool operator!=(const RawRooms &rhs) const
    {
        return !(rhs == *this);
    }
};
