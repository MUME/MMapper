#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
#include <QByteArrayDataPtr>
#include <QCharRef>
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
        explicit Cycler() = default;
        virtual ~Cycler() override;

        DEFAULT_CTORS_AND_ASSIGN_OPS(Cycler);
        Cycler clone() const;

        void addProperty(std::string string);
        void addProperty(const QString &string);
        void addProperty(const QByteArray &byteArray);
    };

private:
    Cycler m_cycler{};
    QString m_roomName{};
    QString m_dynamicDesc{};
    QString m_staticDesc{};
    ExitsFlagsType m_exitsFlags{};
    PromptFlagsType m_promptFlags{};
    ConnectedRoomFlagsType m_connectedRoomFlags{};

    CommandIdType moveType = CommandIdType::NONE;
    uint numSkipped = 0u;

public:
    explicit ParseEvent(const CommandIdType command)
        : moveType(command)
    {}

public:
    ParseEvent(ParseEvent &&) = default;
    explicit ParseEvent(const ParseEvent &other);

public:
    ParseEvent &operator=(ParseEvent);
    static void swap(ParseEvent &lhs, ParseEvent &rhs);
    ParseEvent clone() const { return ParseEvent{*this}; }

public:
    operator QString() const;

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
    CommandIdType getMoveType() const { return moveType; }
    uint getNumSkipped() const { return numSkipped; }
    auto size() const { return m_cycler.size(); }

public:
    const QString &getRoomName() const;
    const QString &getDynamicDesc() const;
    const QString &getStaticDesc() const;
    ExitsFlagsType getExitsFlags() const;
    PromptFlagsType getPromptFlags() const;
    ConnectedRoomFlagsType getConnectedRoomFlags() const;

public:
    static SharedParseEvent createEvent(const CommandIdType c,
                                        const QString &roomName,
                                        const QString &dynamicDesc,
                                        const QString &staticDesc,
                                        const ExitsFlagsType &exitsFlags,
                                        const PromptFlagsType &promptFlags,
                                        const ConnectedRoomFlagsType &connectedRoomFlags);

    static SharedParseEvent createDummyEvent();
};
