// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "parseevent.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>

#include "../global/utils.h"
#include "../mapdata/ExitDirection.h"
#include "../parser/CommandId.h"
#include "../parser/ConnectedRoomFlags.h"
#include "../parser/ExitsFlags.h"
#include "../parser/PromptFlags.h"
#include "property.h"

ParseEvent::Cycler::~Cycler() = default;

ParseEvent::Cycler ParseEvent::Cycler::clone() const
{
    ParseEvent::Cycler result;
    const DUP &from = *this;
    for (const UP &p : from) {
        if (p != nullptr) {
            result.emplace_back(std::make_unique<Property>(*p));
        } else {
            assert(false);
            result.emplace_back(nullptr);
        }
    }
    result.pos = pos;
    return result;
}

void ParseEvent::Cycler::addProperty(std::string s)
{
    if (s.empty())
        emplace_back(std::make_unique<Property>(Property::TagSkip{}));
    else
        emplace_back(std::make_unique<Property>(std::move(s)));
}

void ParseEvent::Cycler::addProperty(const QByteArray &byteArray)
{
    addProperty(byteArray.toStdString());
}

void ParseEvent::Cycler::addProperty(const QString &string)
{
    addProperty(string.toStdString());
}

ParseEvent::ParseEvent(const ParseEvent &other)
    : m_cycler{other.m_cycler.clone()}
    , m_roomName{other.m_roomName}
    , m_dynamicDesc{other.m_dynamicDesc}
    , m_staticDesc{other.m_staticDesc}
    , m_exitsFlags{other.m_exitsFlags}
    , m_promptFlags{other.m_promptFlags}
    , m_connectedRoomFlags{other.m_connectedRoomFlags}
    , moveType{other.moveType}
    , numSkipped{other.numSkipped}
{}

void ParseEvent::swap(ParseEvent &lhs, ParseEvent &rhs)
{
    using std::swap;
    swap(lhs.m_cycler, rhs.m_cycler);
    swap(lhs.m_roomName, rhs.m_roomName);
    swap(lhs.m_dynamicDesc, rhs.m_dynamicDesc);
    swap(lhs.m_staticDesc, rhs.m_staticDesc);
    swap(lhs.m_exitsFlags, rhs.m_exitsFlags);
    swap(lhs.m_promptFlags, rhs.m_promptFlags);
    swap(lhs.m_connectedRoomFlags, rhs.m_connectedRoomFlags);
    swap(lhs.moveType, rhs.moveType);
    swap(lhs.numSkipped, rhs.numSkipped);
}

ParseEvent &ParseEvent::operator=(ParseEvent other)
{
    swap(*this, other);
    return *this;
}

ParseEvent::~ParseEvent() = default;

void ParseEvent::reset()
{
    m_cycler.reset();
    for (auto &property : m_cycler) {
        property->reset();
    }
}

void ParseEvent::countSkipped()
{
    numSkipped = 0;
    for (auto &property : m_cycler) {
        if (property->isSkipped()) {
            numSkipped++;
        }
    }
}

static std::string getPromptBytes(const PromptFlagsType &promptFlags)
{
    auto promptBytes = promptFlags.isValid()
                           ? std::string(1, static_cast<int8_t>(promptFlags.getTerrainType()))
                           : std::string{};
    assert(promptBytes.size() == promptFlags.isValid() ? 1 : 0);
    return promptBytes;
}

ParseEvent::operator QString() const
{
    QString exitsStr;
    // REVISIT: Duplicate code with AbstractParser
    if (m_exitsFlags.isValid() && m_connectedRoomFlags.isValid()) {
        for (const auto &dir : enums::getAllExitsNESWUD()) {
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
                const DirectionalLightEnum lightType = m_connectedRoomFlags.getDirectionalLight(
                    static_cast<DirectionEnum>(dir));
                if (lightType == DirectionalLightEnum::DIRECT_SUN_ROOM)
                    exitsStr.append("^");
                exitsStr.append("]");
            }
        }
    }
    QString promptStr;
    if (m_promptFlags.isValid()) {
        promptStr.append(QString::fromStdString(getPromptBytes(m_promptFlags)));
        if (m_promptFlags.isLit())
            promptStr.append("*");
        else if (m_promptFlags.isDark())
            promptStr.append("o");
    }

    return QString("[%1,%2,%3,%4,%5,%6,%7]")
        .arg(m_roomName)
        .arg(m_dynamicDesc)
        .arg(m_staticDesc)
        .arg(exitsStr)
        .arg(promptStr)
        .arg(getUppercase(moveType))
        .arg(numSkipped)
        .replace("\n", "\\n");
}

SharedParseEvent ParseEvent::createEvent(const CommandEnum c,
                                         const QString &roomName,
                                         const QString &dynamicDesc,
                                         const QString &staticDesc,
                                         const ExitsFlagsType &exitsFlags,
                                         const PromptFlagsType &promptFlags,
                                         const ConnectedRoomFlagsType &connectedRoomFlags)
{
    auto event = std::make_shared<ParseEvent>(c);
    event->m_roomName = roomName;
    event->m_dynamicDesc = dynamicDesc;
    event->m_staticDesc = staticDesc;
    event->m_exitsFlags = exitsFlags;
    event->m_promptFlags = promptFlags;
    event->m_connectedRoomFlags = connectedRoomFlags;
    event->countSkipped();

    auto &cycler = event->m_cycler;
    cycler.addProperty(roomName);
    cycler.addProperty(staticDesc);
    cycler.addProperty(getPromptBytes(promptFlags));
    assert(cycler.size() == 3);

    return event;
}

SharedParseEvent ParseEvent::createDummyEvent()
{
    return createEvent(CommandEnum::UNKNOWN,
                       QString{},
                       QString{},
                       QString{},
                       ExitsFlagsType{},
                       PromptFlagsType{},
                       ConnectedRoomFlagsType{});
}

const QString &ParseEvent::getRoomName() const
{
    return m_roomName;
}

const QString &ParseEvent::getDynamicDesc() const
{
    return m_dynamicDesc;
}

const QString &ParseEvent::getStaticDesc() const
{
    return m_staticDesc;
}

ExitsFlagsType ParseEvent::getExitsFlags() const
{
    return m_exitsFlags;
}

PromptFlagsType ParseEvent::getPromptFlags() const
{
    return m_promptFlags;
}

ConnectedRoomFlagsType ParseEvent::getConnectedRoomFlags() const
{
    return m_connectedRoomFlags;
}
