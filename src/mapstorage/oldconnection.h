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

#ifndef OLDCONNECTION_H
#define OLDCONNECTION_H

#include <cstddef>
#include <cstdint>
#include <QDateTime>
#include <QString>
#include <QtCore>
#include <QtGlobal>

#include "../global/EnumIndexedArray.h"
#include "../global/Flags.h"

class Door;
class Room;

using ConnectionNote = QString;

enum class Hand { LEFT = 0, RIGHT };
DEFINE_ENUM_COUNT(Hand, 2);

enum class ConnectionType { NORMAL = 0, LOOP, ONE_WAY };

enum class ConnectionDirection { NORTH = 0, SOUTH, EAST, WEST, UP, DOWN, UNKNOWN, NONE };

ConnectionDirection opposite(ConnectionDirection in);

enum class ConnectionFlag {
    DOOR = 0,
    CLIMB,
    RANDOM,
    SPECIAL,
};
static constexpr const size_t NUM_CONNECTION_FLAGS = static_cast<size_t>(ConnectionFlag::SPECIAL)
                                                     + 1u;
static_assert(NUM_CONNECTION_FLAGS == 4u, "");
DEFINE_ENUM_COUNT(ConnectionFlag, NUM_CONNECTION_FLAGS);

class ConnectionFlags : public enums::Flags<ConnectionFlags, ConnectionFlag, uint8_t>
{
public:
    using Flags::Flags;
};

using ConnectionTimeStamp = QDateTime;

class Connection
{
public:
    static constexpr const Hand FIRST = Hand::LEFT;
    static constexpr const Hand SECOND = Hand::RIGHT;

public:
    explicit Connection();
    ~Connection();

    const ConnectionNote &getNote() const { return m_note; };
    quint32 getIndex() const { return m_index; };
    [[deprecated]] Room *getRoom(quint8 idx) const { return m_rooms[static_cast<Hand>(idx)]; };
    Room *getRoom(Hand idx) const { return m_rooms[idx]; };
    Room *getTargetRoom(Room *r) const
    {
        Room *tmp;
        r == m_rooms[FIRST] ? tmp = m_rooms[SECOND] : tmp = m_rooms[FIRST];
        return tmp;
    };
    Door *getDoor(Room *r) const
    {
        Door *tmp;
        r == m_rooms[FIRST] ? tmp = m_doors[FIRST] : tmp = m_doors[SECOND];
        return tmp;
    };
    Door *getDoor(Hand idx) const { return m_doors[idx]; };
    ConnectionDirection getDirection(Room *r) const
    {
        ConnectionDirection tmp;
        r == m_rooms[FIRST] ? tmp = m_directions[FIRST] : tmp = m_directions[SECOND];
        return tmp;
    };
    ConnectionDirection getTargetDirection(Room *r) const
    {
        ConnectionDirection tmp;
        r == m_rooms[FIRST] ? tmp = m_directions[SECOND] : tmp = m_directions[FIRST];
        return tmp;
    };
    ConnectionDirection getDirection(Hand idx) const { return m_directions[idx]; };
    ConnectionType getType() const { return m_type; };
    ConnectionFlags getFlags() const { return m_flags; };
    const ConnectionTimeStamp &getTimeStamp() const { return m_timeStamp; };

    //    void setID(ConnectionID id) { m_ID = id; };
    void setNote(ConnectionNote note) { m_note = note; };
    void setIndex(quint32 idx) { m_index = idx; };
    [[deprecated]] void setRoom(Room *room, quint8 idx) { m_rooms[static_cast<Hand>(idx)] = room; };
    void setRoom(Room *room, Hand idx) { m_rooms[idx] = room; };
    void setDoor(Door *door, Hand idx)
    {
        m_doors[idx] = door;
        m_flags |= ConnectionFlag::DOOR;
    };
    void setDirection(ConnectionDirection direction, Hand idx) { m_directions[idx] = direction; };
    void setDirection(ConnectionDirection direction, Room *r)
    {
        if (r == m_rooms[FIRST])
            m_directions[FIRST] = direction;
        if (r == m_rooms[SECOND])
            m_directions[SECOND] = direction;
    };
    void setType(ConnectionType type) { m_type = type; };
    void setFlags(ConnectionFlags flags) { m_flags = flags; };
    void setTimeStamp(ConnectionTimeStamp timeStamp) { m_timeStamp = timeStamp; };

private:
    //connection note
    ConnectionNote m_note{};

    EnumIndexedArray<ConnectionDirection, Hand> m_directions{};

    ConnectionTimeStamp m_timeStamp{};

    //Index to rooms
    EnumIndexedArray<Room *, Hand> m_rooms{};

    //doors (in case of exit with doors)
    EnumIndexedArray<Door *, Hand> m_doors{};

    //type of connections
    ConnectionType m_type = ConnectionType::NORMAL;
    ConnectionFlags m_flags{0};

    quint32 m_index = 0u;
};

#endif
