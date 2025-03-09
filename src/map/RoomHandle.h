#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/Badge.h"
#include "../global/ConfigConsts.h"
#include "../global/macros.h"
#include "Map.h"
#include "ParseTree.h"

#include <optional>
#include <utility>

class NODISCARD RoomHandle final : public RoomExitFieldsGetters<RoomHandle>
{
private:
    Map m_map;
    const RawRoom *m_room = nullptr;

public:
    // This must be implict for use in std::array.
    RoomHandle() = default;
    explicit RoomHandle(Badge<Map>, Map map, const RawRoom *const room)
        : m_map{std::move(map)}
        , m_room{room}
    {
        if constexpr ((IS_DEBUG_BUILD)) {
            sanityCheck();
        }
    }

public:
    void reset() { *this = RoomHandle{}; }

public:
    NODISCARD bool operator==(std::nullopt_t) const = delete;
    NODISCARD bool operator!=(std::nullopt_t) const = delete;
    NODISCARD bool operator==(std::nullptr_t) const = delete;
    NODISCARD bool operator!=(std::nullptr_t) const = delete;

public:
#define X_DECL_GETTER(Type, Name, Init) NODISCARD Type get##Name() const;
    XFOREACH_ROOM_PROPERTY(X_DECL_GETTER)
#undef X_DECL_GETTER

public:
    NODISCARD const RawRoom &getRaw() const;
    NODISCARD ExternalRawRoom getRawCopyExternal() const;

public:
    NODISCARD Map getMap() const
    {
        return m_map;
    }
    NODISCARD RoomId getId() const
    {
        return deref(m_room).getId();
    }
    NODISCARD ExternalRoomId getIdExternal() const;

private:
    void sanityCheck() const;

public:
    NODISCARD bool exists() const;
    NODISCARD explicit operator bool() const
    {
        return exists();
    }

public:
    NODISCARD ServerRoomId getServerId() const;
    NODISCARD const Coordinate &getPosition() const;
    NODISCARD bool isTemporary() const;

public:
    NODISCARD ExitDirFlags computeExitDirections() const;
    NODISCARD ExitsFlagsType computeExitsFlags() const;

public:
    NODISCARD const auto &getExits() const
    {
        return getRaw().getExits();
    }
    NODISCARD const RawExit &getExit(const ExitDirEnum dir) const
    {
        return getRaw().getExit(dir);
    }

public:
    NODISCARD bool operator==(const RoomHandle &rhs) const;
    NODISCARD bool operator!=(const RoomHandle &rhs) const
    {
        return !(rhs == *this);
    }

public:
    // REVISIT: Only used by compareWeakProps() and TestExpandoraCommon::roomCompareTest().
    // Can we just remove it and let them call previewRoom()?
    NODISCARD std::string toStdStringUtf8() const;
};

template<>
struct std::hash<RoomHandle>
{
    NODISCARD std::size_t operator()(const RoomHandle &room) const noexcept
    {
        return std::hash<size_t>()(room.getId().asUint32());
    }
};

NODISCARD bool matches(const RawRoom &room, const ParseEvent &parseEvent);
