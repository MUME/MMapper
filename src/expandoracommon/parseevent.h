#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

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

#include "../global/roomserverid.h"
#include "../mapdata/mmapper2exit.h"
#include "../parser/CommandId.h"
#include "../parser/ConnectedRoomFlags.h"
#include "../parser/ExitsFlags.h"
#include "../parser/PromptFlags.h"
#include "MmQtHandle.h"
#include "property.h"

class ParseEvent;
using SharedParseEvent = std::shared_ptr<ParseEvent>;
using SigParseEvent = MmQtHandle<ParseEvent>;

/**
 * the ParseEvents will walk around in the SearchTree
 */
class NODISCARD ParseEvent final
{
public:
    static constexpr const size_t NUM_PROPS = 3;

private:
    struct ArrayOfProperties final : public std::array<Property, NUM_PROPS>
    {
    private:
        using Base = std::array<Property, NUM_PROPS>;

    public:
        ArrayOfProperties();
        ~ArrayOfProperties();
        DEFAULT_CTORS_AND_ASSIGN_OPS(ArrayOfProperties);

    public:
        using std::array<Property, NUM_PROPS>::operator[];

    public:
        void setProperty(size_t pos, std::string string);
    };

private:
    ArrayOfProperties m_properties;
    RoomServerId m_roomServerId;
    RoomName m_roomName;
    RoomDesc m_roomDesc;
    RoomContents m_roomContents;
    ExitsFlagsType m_exitsFlags;
    PromptFlagsType m_promptFlags;
    ConnectedRoomFlagsType m_connectedRoomFlags;

    uint m_numSkipped = 0u;

    RoomTerrainEnum m_terrain = RoomTerrainEnum::UNDEFINED;
    CommandEnum m_moveType = CommandEnum::NONE;

public:
    explicit ParseEvent(const CommandEnum command)
        : m_moveType{command}
    {}

public:
    DEFAULT_CTORS_AND_ASSIGN_OPS(ParseEvent);

public:
    // REVISIT: Cloning the event should no longer be necessary since all public methods are const;
    // instead, we we should make ParseEvent use `shared_from_this` and just copy the shared_ptr.
    NODISCARD ParseEvent clone() const { return ParseEvent{*this}; }

public:
    NODISCARD QString toQString() const;
    explicit operator QString() const { return toQString(); }
    friend QDebug operator<<(QDebug os, const ParseEvent &ev) { return os << ev.toQString(); }

public:
    virtual ~ParseEvent();

private:
    void setProperty(const RoomName &name) { m_properties.setProperty(0, name.getStdString()); }
    void setProperty(const RoomDesc &desc) { m_properties.setProperty(1, desc.getStdString()); }
    void setProperty(const RoomTerrainEnum &terrain);
    void countSkipped();

public:
    NODISCARD RoomServerId getRoomServerId() const { return m_roomServerId; }
    NODISCARD const RoomName &getRoomName() const { return m_roomName; }
    NODISCARD const RoomDesc &getRoomDesc() const { return m_roomDesc; }
    NODISCARD const RoomContents &getRoomContents() const { return m_roomContents; }
    NODISCARD ExitsFlagsType getExitsFlags() const { return m_exitsFlags; }
    NODISCARD PromptFlagsType getPromptFlags() const { return m_promptFlags; }
    NODISCARD ConnectedRoomFlagsType getConnectedRoomFlags() const { return m_connectedRoomFlags; }
    NODISCARD CommandEnum getMoveType() const { return m_moveType; }
    NODISCARD RoomTerrainEnum getTerrainType() const { return m_terrain; }
    NODISCARD uint getNumSkipped() const { return m_numSkipped; }
    const Property &operator[](const size_t pos) const { return m_properties.at(pos); }

public:
    static SharedParseEvent createEvent(CommandEnum c,
                                        RoomServerId roomServerId,
                                        RoomName roomName,
                                        RoomDesc roomDesc,
                                        RoomContents roomContents,
                                        const RoomTerrainEnum &terrain,
                                        const ExitsFlagsType &exitsFlags,
                                        const PromptFlagsType &promptFlags,
                                        const ConnectedRoomFlagsType &connectedRoomFlags);

    static SharedParseEvent createDummyEvent();
};
