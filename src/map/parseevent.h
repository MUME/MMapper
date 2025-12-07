#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/EnumIndexedArray.h"
#include "../global/MmQtHandle.h"
#include "../map/RawRoom.h"
#include "../map/roomid.h"
#include "CommandId.h"
#include "ConnectedRoomFlags.h"
#include "DoorFlags.h"
#include "ExitDirection.h"
#include "ExitFieldVariant.h"
#include "ExitFlags.h"
#include "ExitsFlags.h"
#include "PromptFlags.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include <QDebug>
#include <QString>
#include <QtGlobal>

class ParseEvent;
using SharedParseEvent = std::shared_ptr<ParseEvent>;
using SigParseEvent = MmQtHandle<ParseEvent>;
using ServerExitIds = EnumIndexedArray<ServerRoomId, ExitDirEnum, NUM_EXITS>;
using RawExits = RawRoom::Exits;

// X(_MemberType, _memberName, _defaultInitializer)
#define XFOREACH_PARSEEVENT_MEMBER(X) \
    X(RoomArea, m_roomArea, ) \
    X(RoomName, m_roomName, ) \
    X(RoomDesc, m_roomDesc, ) \
    X(RoomContents, m_roomContents, ) \
    X(ServerExitIds, m_exitIds, ) \
    X(RawExits, m_exits, ) \
    X(PromptFlagsType, m_promptFlags, ) \
    X(ConnectedRoomFlagsType, m_connectedRoomFlags, ) \
    X(ServerRoomId, m_serverId, {INVALID_SERVER_ROOMID}) \
    X(RoomTerrainEnum, m_terrain, {RoomTerrainEnum::UNDEFINED}) \
    X(CommandEnum, m_moveType, {CommandEnum::LOOK})

// ParseEvent is sent as an argument from the parser to the path machine.
// It's also used for creating and querying rooms.
//
// REVISIT: add expected position after the movement (and maybe before, too)?
class NODISCARD ParseEvent final
{
private:
#define X_DECL(_MemberType, _memberName, _defaultInitializer) \
    _MemberType _memberName _defaultInitializer;
    XFOREACH_PARSEEVENT_MEMBER(X_DECL)
#undef X_DECL

public:
    explicit ParseEvent() = default;
    explicit ParseEvent(const CommandEnum command)
        : m_moveType{command}
    {}

public:
    DEFAULT_CTORS_AND_ASSIGN_OPS(ParseEvent);

public:
    NODISCARD QString toQString() const;
    explicit operator QString() const = delete;
    friend QDebug operator<<(QDebug os, const ParseEvent &ev) { return os << ev.toQString(); }

public:
    virtual ~ParseEvent();

public:
    NODISCARD ServerRoomId getServerId() const { return m_serverId; }
    NODISCARD const RoomArea &getRoomArea() const { return m_roomArea; }
    NODISCARD const RoomName &getRoomName() const { return m_roomName; }
    NODISCARD const RoomDesc &getRoomDesc() const { return m_roomDesc; }
    NODISCARD const RoomContents &getRoomContents() const { return m_roomContents; }
    NODISCARD const ServerExitIds &getExitIds() const { return m_exitIds; }

    NODISCARD const RawExits &getExits() const { return m_exits; }

    // DEPRECATED_MSG("use getExits()")
    NODISCARD ExitsFlagsType getExitsFlags() const;

    NODISCARD PromptFlagsType getPromptFlags() const { return m_promptFlags; }
    NODISCARD ConnectedRoomFlagsType getConnectedRoomFlags() const { return m_connectedRoomFlags; }
    NODISCARD CommandEnum getMoveType() const { return m_moveType; }
    NODISCARD RoomTerrainEnum getTerrainType() const { return m_terrain; }

    // DEPRECATED: Do not use this in new code;
    // use hasNameDescFlags() or canCreateNewRoom() instead.
    //
    // Returns the number of (room name, room desc, and prompt flags) that are missing.
    // For historical reasons, the PathMachine::syncing() function is allowed to use this
    // to compare against a configurable parameter called "maxSkipped" (default: 1).
    NODISCARD size_t getNumSkipped() const
    {
        return static_cast<size_t>(m_roomName.empty())          //
               + static_cast<size_t>(m_roomDesc.empty())        //
               + static_cast<size_t>(!m_promptFlags.isValid()); //
    }

    // Returns true of the event specifies room name, room desc, and prompt flags.
    NODISCARD bool hasNameDescFlags() const { return getNumSkipped() == 0; }
    NODISCARD bool hasServerId() const { return getServerId() != INVALID_SERVER_ROOMID; }
    NODISCARD bool canCreateNewRoom() const { return hasServerId() || hasNameDescFlags(); }

private:
    template<typename RefType>
    void set(RefType &&x)
    {
#define X_TRY_SET(_MemberType, _memberName, _defaultValue) \
    else if constexpr (std::is_same_v<T, _MemberType>) \
    { \
        (this->_memberName) = std::forward<RefType>(x); \
    }

        using T = utils::remove_cvref_t<RefType>;
        if constexpr (false) {
            // this case only exists to allow "else if constexpr" in the x-macro
        }
        XFOREACH_PARSEEVENT_MEMBER(X_TRY_SET)
        else
        {
            // this case catches any un-matched types; if you get this error,
            // it means you passed the wrong argument type.
            static_assert(std::is_same_v<T, void>, "invalid argument type");
        }

#undef X_TRY_SET
    }

public:
    NODISCARD static SharedParseEvent createSharedEvent(CommandEnum c,
                                                        ServerRoomId id,
                                                        RoomArea roomArea,
                                                        RoomName roomName,
                                                        RoomDesc roomDesc,
                                                        RoomContents roomContents,
                                                        const ServerExitIds &exitIds,
                                                        RoomTerrainEnum terrain,
                                                        RawExits exits,
                                                        PromptFlagsType promptFlags,
                                                        ConnectedRoomFlagsType connectedRoomFlags);
    NODISCARD static ParseEvent createEvent(CommandEnum c,
                                            ServerRoomId id,
                                            RoomArea roomArea,
                                            RoomName roomName,
                                            RoomDesc roomDesc,
                                            RoomContents roomContents,
                                            ServerExitIds exitIds,
                                            RoomTerrainEnum terrain,
                                            RawExits exits,
                                            PromptFlagsType promptFlags,
                                            ConnectedRoomFlagsType connectedRoomFlags);

    template<typename... Args>
    NODISCARD static ParseEvent createEvent2(Args &&...args)
    {
        static_assert(utils::are_distinct_v<utils::remove_cvref_t<Args>...>,
                      "each argument type can only be used once");
        ParseEvent result{};
        // note: parameter pack expansion calls set with each individual argument
        (result.set(std::forward<Args>(args)), ...);
        return result;
    }

    NODISCARD static SharedParseEvent createDummyEvent();
};
