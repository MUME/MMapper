#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
#include <QByteArrayDataPtr>
#include <QCharRef>
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
#include "listcycler.h"
#include "property.h"

class ParseEvent;
class Property;
using SharedParseEvent = std::shared_ptr<ParseEvent>;
using SigParseEvent = MmQtHandle<ParseEvent>;

/**
 * the ParseEvents will walk around in the SearchTree
 */
class ParseEvent final
{
private:
    using UP = std::unique_ptr<Property>;
    using DUP = std::deque<UP>;
    using LUP = ListCycler<UP, DUP>;
    struct Cycler final : LUP
    {
        Cycler() = default;
        virtual ~Cycler() override;

        DEFAULT_CTORS_AND_ASSIGN_OPS(Cycler);
        Cycler clone() const;

        void addProperty(std::string string);
        void addProperty(const QString &string);
        void addProperty(const QByteArray &byteArray);
    };

private:
    Cycler m_cycler;
    RoomName m_roomName;
    RoomDynamicDesc m_dynamicDesc;
    RoomStaticDesc m_staticDesc;
    ExitsFlagsType m_exitsFlags;
    PromptFlagsType m_promptFlags;
    ConnectedRoomFlagsType m_connectedRoomFlags;

    CommandEnum moveType = CommandEnum::NONE;
    uint numSkipped = 0u;

public:
    explicit ParseEvent(const CommandEnum command)
        : moveType(command)
    {}

public:
    DEFAULT_MOVE_CTOR(ParseEvent);
    ParseEvent(const ParseEvent &other);

public:
    /* NOTE: This handles both the move and copy assignment operators. */
    ParseEvent &operator=(ParseEvent);
    static void swap(ParseEvent &lhs, ParseEvent &rhs);
    ParseEvent clone() const { return ParseEvent{*this}; }

public:
    QString toQString() const;
    explicit operator QString() const { return toQString(); }
    friend QDebug operator<<(QDebug os, const ParseEvent &ev) { return os << ev.toQString(); }

public:
    virtual ~ParseEvent();

public:
    Property *current() { return m_cycler.current().get(); }
    Property *prev() { return m_cycler.prev().get(); }
    Property *next() { return m_cycler.next().get(); }
    void reset();

private:
    void countSkipped();

public:
    CommandEnum getMoveType() const { return moveType; }
    uint getNumSkipped() const { return numSkipped; }
    auto size() const { return m_cycler.size(); }

public:
    const RoomName &getRoomName() const;
    const RoomDynamicDesc &getDynamicDesc() const;
    const RoomStaticDesc &getStaticDesc() const;
    ExitsFlagsType getExitsFlags() const;
    PromptFlagsType getPromptFlags() const;
    ConnectedRoomFlagsType getConnectedRoomFlags() const;

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
