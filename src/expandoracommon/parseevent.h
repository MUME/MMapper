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
#include <QVariant>
#include <QtGlobal>

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
class ParseEvent final
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
    RoomName m_roomName;
    RoomDynamicDesc m_dynamicDesc;
    RoomStaticDesc m_staticDesc;
    ExitsFlagsType m_exitsFlags;
    PromptFlagsType m_promptFlags;
    ConnectedRoomFlagsType m_connectedRoomFlags;

    CommandEnum m_moveType = CommandEnum::NONE;
    uint m_numSkipped = 0u;

public:
    explicit ParseEvent(const CommandEnum command)
        : m_moveType{command}
    {}

public:
    DEFAULT_CTORS_AND_ASSIGN_OPS(ParseEvent);

public:
    // REVISIT: Cloning the event should no longer be necessary since all public methods are const;
    // instead, we we should make ParseEvent use `shared_from_this` and just copy the shared_ptr.
    ParseEvent clone() const { return ParseEvent{*this}; }

public:
    QString toQString() const;
    explicit operator QString() const { return toQString(); }
    friend QDebug operator<<(QDebug os, const ParseEvent &ev) { return os << ev.toQString(); }

public:
    virtual ~ParseEvent();

private:
    void setProperty(const RoomName &name) { m_properties.setProperty(0, name.getStdString()); }
    void setProperty(const RoomStaticDesc &desc)
    {
        m_properties.setProperty(1, desc.getStdString());
    }
    void setProperty(const PromptFlagsType &prompt);
    void countSkipped();

public:
    const RoomName &getRoomName() const { return m_roomName; }
    const RoomDynamicDesc &getDynamicDesc() const { return m_dynamicDesc; }
    const RoomStaticDesc &getStaticDesc() const { return m_staticDesc; }
    ExitsFlagsType getExitsFlags() const { return m_exitsFlags; }
    PromptFlagsType getPromptFlags() const { return m_promptFlags; }
    ConnectedRoomFlagsType getConnectedRoomFlags() const { return m_connectedRoomFlags; }
    CommandEnum getMoveType() const { return m_moveType; }
    uint getNumSkipped() const { return m_numSkipped; }
    const Property &operator[](const size_t pos) const { return m_properties.at(pos); }

public:
    static SharedParseEvent createEvent(CommandEnum c,
                                        RoomName roomName,
                                        RoomDynamicDesc dynamicDesc,
                                        RoomStaticDesc staticDesc,
                                        const ExitsFlagsType &exitsFlags,
                                        const PromptFlagsType &promptFlags,
                                        const ConnectedRoomFlagsType &connectedRoomFlags);

    static SharedParseEvent createDummyEvent();
};
