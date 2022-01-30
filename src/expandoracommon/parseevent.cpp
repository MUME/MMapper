// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "parseevent.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>

#include "../global/TextUtils.h"
#include "../global/utils.h"
#include "../mapdata/ExitDirection.h"
#include "../parser/CommandId.h"
#include "../parser/ConnectedRoomFlags.h"
#include "../parser/ExitsFlags.h"
#include "../parser/PromptFlags.h"
#include "property.h"

ParseEvent::ArrayOfProperties::ArrayOfProperties() = default;
ParseEvent::ArrayOfProperties::~ArrayOfProperties() = default;

void ParseEvent::ArrayOfProperties::setProperty(const size_t pos, std::string s)
{
    ArrayOfProperties::at(pos) = Property{std::move(s)};
}

NODISCARD static std::string getTerrainBytes(const RoomTerrainEnum &terrain)
{
    const bool terrainValid = (terrain != RoomTerrainEnum::UNDEFINED);
    const auto terrainBytes = terrainValid ? std::string(1, static_cast<int8_t>(terrain))
                                           : std::string{};
    assert(terrainBytes.size() == (terrainValid ? 1 : 0));
    return terrainBytes;
}

void ParseEvent::setProperty(const RoomTerrainEnum &terrain)
{
    m_properties.setProperty(2, getTerrainBytes(terrain));
}

ParseEvent::~ParseEvent() = default;

void ParseEvent::countSkipped()
{
    m_numSkipped = [this]() {
        decltype(m_numSkipped) numSkipped = 0;
        for (const auto &property : m_properties) {
            if (property.isSkipped()) {
                numSkipped++;
            }
        }
        return numSkipped;
    }();
}

QString ParseEvent::toQString() const
{
    QString exitsStr;
    // REVISIT: Duplicate code with AbstractParser
    if (m_exitsFlags.isValid() && m_connectedRoomFlags.isValid()) {
        for (const ExitDirEnum dir : enums::getAllExitsNESWUD()) {
            const ExitFlags exitFlags = m_exitsFlags.get(dir);
            if (exitFlags.isExit()) {
                exitsStr.append("[");
                exitsStr.append(lowercaseDirection(dir));
                if (exitFlags.isClimb())
                    exitsStr.append("/");
                if (exitFlags.isRoad())
                    exitsStr.append("=");
                if (exitFlags.isDoor())
                    exitsStr.append("(");
                if (m_connectedRoomFlags.hasDirectSunlight(dir))
                    exitsStr.append("^");
                exitsStr.append("]");
            }
        }
    }
    QString promptStr;
    promptStr.append(::toQStringLatin1(getTerrainBytes(m_terrain)));
    if (m_promptFlags.isValid()) {
        if (m_promptFlags.isLit())
            promptStr.append("*");
        else if (m_promptFlags.isDark())
            promptStr.append("o");
    }

    return QString("[%1,%2,%3,%4,%5,%6,%7]")
        .arg(m_roomName.toQString())
        .arg(m_roomDesc.toQString())
        .arg(m_roomContents.toQString())
        .arg(exitsStr)
        .arg(promptStr)
        .arg(getUppercase(m_moveType))
        .arg(m_numSkipped)
        .replace("\n", "\\n");
}

SharedParseEvent ParseEvent::createEvent(const CommandEnum c,
                                         RoomName moved_roomName,
                                         RoomDesc moved_roomDesc,
                                         RoomContents moved_roomContents,
                                         const RoomTerrainEnum &terrain,
                                         const ExitsFlagsType &exitsFlags,
                                         const PromptFlagsType &promptFlags,
                                         const ConnectedRoomFlagsType &connectedRoomFlags)
{
    auto result = std::make_shared<ParseEvent>(c);
    ParseEvent *const event = result.get();

    // the moved strings are used by const ref here before they're moved.
    event->setProperty(moved_roomName);
    event->setProperty(moved_roomDesc);
    event->setProperty(terrain);

    // After this block, the moved values are gone.
    event->m_roomName = std::exchange(moved_roomName, {});
    event->m_roomDesc = std::exchange(moved_roomDesc, {});
    event->m_roomContents = std::exchange(moved_roomContents, {});
    event->m_terrain = terrain;
    event->m_exitsFlags = exitsFlags;
    event->m_promptFlags = promptFlags;
    event->m_connectedRoomFlags = connectedRoomFlags;
    event->countSkipped();

    return result;
}

SharedParseEvent ParseEvent::createDummyEvent()
{
    return createEvent(CommandEnum::UNKNOWN,
                       RoomName{},
                       RoomDesc{},
                       RoomContents{},
                       RoomTerrainEnum::UNDEFINED,
                       ExitsFlagsType{},
                       PromptFlagsType{},
                       ConnectedRoomFlagsType{});
}
