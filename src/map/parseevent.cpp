// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "parseevent.h"

#include <memory>

#define X_TEST(_MemberType, _memberName, _defaultInitializer) utils::remove_cvref_t<_MemberType>,
// note: using void here as a sentinel type to avoid splitting the x-macro into X() and XSEP()
static_assert(utils::are_distinct_v<XFOREACH_PARSEEVENT_MEMBER(X_TEST) void>);
#undef X_TEST

NODISCARD static std::string getTerrainBytes(const RoomTerrainEnum &terrain)
{
    const bool terrainValid = (terrain != RoomTerrainEnum::UNDEFINED);
    const auto terrainBytes = terrainValid
                                  ? std::string(1, static_cast<char>(static_cast<uint8_t>(terrain)))
                                  : std::string{};
    assert(terrainBytes.size() == (terrainValid ? 1 : 0));
    return terrainBytes;
}

ParseEvent::~ParseEvent() = default;

QString ParseEvent::toQString() const
{
    using namespace char_consts;
    QString exitsStr;
    // REVISIT: Duplicate code with AbstractParser
    if (m_exitsFlags.isValid() && m_connectedRoomFlags.isValid()) {
        for (const ExitDirEnum dir : enums::getAllExitsNESWUD()) {
            const ExitFlags exitFlags = m_exitsFlags.get(dir);
            if (exitFlags.isExit()) {
                exitsStr.append(C_OPEN_BRACKET);
                exitsStr.append(lowercaseDirection(dir));
                if (exitFlags.isClimb()) {
                    exitsStr.append(C_SLASH);
                }
                if (exitFlags.isRoad()) {
                    exitsStr.append(C_EQUALS);
                }
                if (exitFlags.isDoor()) {
                    exitsStr.append(C_OPEN_PARENS);
                }
                if (m_connectedRoomFlags.hasDirectSunlight(dir)) {
                    exitsStr.append(C_CARET);
                }
                exitsStr.append(C_CLOSE_BRACKET);
            }
        }
    }
    QString promptStr;
    promptStr.append(mmqt::toQStringUtf8(getTerrainBytes(m_terrain)));
    if (m_promptFlags.isValid()) {
        if (m_promptFlags.isLit()) {
            promptStr.append(C_ASTERISK);
        } else if (m_promptFlags.isDark()) {
            promptStr.append("o");
        }
    }

    return QString("[%1,%2,%3,%4,%5,%6,%7]")
        .arg(m_roomName.toQString())
        .arg(m_roomDesc.toQString())
        .arg(m_roomContents.toQString())
        .arg(exitsStr)
        .arg(promptStr)
        .arg(getUppercase(m_moveType))
        .arg(getNumSkipped())
        .replace(string_consts::S_NEWLINE, "\\n");
}

ParseEvent ParseEvent::createEvent(const CommandEnum c,
                                   const ServerRoomId id,
                                   RoomName moved_roomName,
                                   RoomDesc moved_roomDesc,
                                   RoomContents moved_roomContents,
                                   ServerExitIds moved_exitIds,
                                   const RoomTerrainEnum terrain,
                                   const ExitsFlagsType exitsFlags,
                                   const PromptFlagsType promptFlags,
                                   const ConnectedRoomFlagsType connectedRoomFlags)
{
    ParseEvent event(c);

    // After this block, the moved values are gone.
    event.m_serverId = id;
    event.m_roomName = std::exchange(moved_roomName, {});
    event.m_roomDesc = std::exchange(moved_roomDesc, {});
    event.m_roomContents = std::exchange(moved_roomContents, {});
    event.m_exitIds = std::exchange(moved_exitIds, {});
    event.m_terrain = terrain;
    event.m_exitsFlags = exitsFlags;
    event.m_promptFlags = promptFlags;
    event.m_connectedRoomFlags = connectedRoomFlags;

    return event;
}

SharedParseEvent ParseEvent::createSharedEvent(const CommandEnum c,
                                               const ServerRoomId id,
                                               RoomName moved_roomName,
                                               RoomDesc moved_roomDesc,
                                               RoomContents moved_roomContents,
                                               ServerExitIds moved_exitIds,
                                               const RoomTerrainEnum terrain,
                                               const ExitsFlagsType exitsFlags,
                                               const PromptFlagsType promptFlags,
                                               const ConnectedRoomFlagsType connectedRoomFlags)
{
    return std::make_shared<ParseEvent>(createEvent(c,
                                                    id,
                                                    std::move(moved_roomName),
                                                    std::move(moved_roomDesc),
                                                    std::move(moved_roomContents),
                                                    std::move(moved_exitIds),
                                                    terrain,
                                                    exitsFlags,
                                                    promptFlags,
                                                    connectedRoomFlags));
}

SharedParseEvent ParseEvent::createDummyEvent()
{
    return createSharedEvent(CommandEnum::UNKNOWN,
                             INVALID_SERVER_ROOMID,
                             RoomName{},
                             RoomDesc{},
                             RoomContents{},
                             ServerExitIds{},
                             RoomTerrainEnum::UNDEFINED,
                             ExitsFlagsType{},
                             PromptFlagsType{},
                             ConnectedRoomFlagsType{});
}
