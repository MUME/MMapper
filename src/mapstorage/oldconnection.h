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

#include "defs.h"

#include <QDateTime>

class Room;
class Door;

typedef class QString ConnectionNote;

enum ConnectionType { CT_NORMAL = 0, CT_LOOP, CT_ONEWAY, CT_UNDEFINED };

enum ConnectionDirection { CD_NORTH=0, CD_SOUTH, CD_EAST, CD_WEST, CD_UP, 
			   CD_DOWN, CD_UNKNOWN, CD_NONE };

ConnectionDirection opposite(ConnectionDirection in);

#define CF_DOOR       bit1
#define CF_CLIMB      bit2
#define CF_RANDOM     bit3
#define CF_SPECIAL    bit4
#define CF_RESERVED1  bit5
#define CF_RESERVED2  bit6
#define CF_RESERVED3  bit7
#define CF_RESERVED4  bit8
typedef quint8 ConnectionFlags;

typedef QDateTime ConnectionTimeStamp;

class Connection {

public:
    Connection();
    ~Connection();
    
    const ConnectionNote& getNote() const { return m_note; };
    quint32 getIndex() const { return m_index; };
    Room* getRoom(quint8 idx) const { return m_rooms[idx]; };
    Room* getTargetRoom(Room* r) const { Room* tmp; r == m_rooms[FIRST] ? tmp = m_rooms[SECOND] : tmp = m_rooms[FIRST]; return tmp; };
    Door* getDoor(Room* r) const { Door* tmp; r == m_rooms[FIRST] ? tmp = m_doors[FIRST] : tmp = m_doors[SECOND]; return tmp; };
    Door* getDoor(quint8 idx) const { return m_doors[idx]; };
    ConnectionDirection getDirection(Room* r) const {ConnectionDirection tmp; r == m_rooms[FIRST] ? tmp = m_directions[FIRST] : tmp = m_directions[SECOND]; return tmp; };
    ConnectionDirection getTargetDirection(Room* r) const {ConnectionDirection tmp; r == m_rooms[FIRST] ? tmp = m_directions[SECOND] : tmp = m_directions[FIRST]; return tmp; };
    ConnectionDirection getDirection(quint8 idx) const { return m_directions[idx]; };
    ConnectionType getType() const { return m_type; };
    ConnectionFlags getFlags() const { return m_flags; };
    const ConnectionTimeStamp& getTimeStamp() const { return m_timeStamp; };

//    void setID(ConnectionID id) { m_ID = id; };
    void setNote(ConnectionNote note) { m_note = note; };
    void setIndex(quint32 idx) { m_index = idx; };
    void setRoom(Room* room, quint8 idx){ m_rooms[idx] = room; };
    void setDoor(Door* door, quint8 idx){ m_doors[idx] = door; m_flags |= CF_DOOR;};
    void setDirection(ConnectionDirection direction, quint8 idx){m_directions[idx] = direction; };
    void setDirection(ConnectionDirection direction, Room* r){ if(r == m_rooms[FIRST]) m_directions[FIRST] = direction; if(r == m_rooms[SECOND])  m_directions[SECOND] = direction; };
    void setType( ConnectionType type ){ m_type = type; };
    void setFlags( ConnectionFlags flags ) { m_flags = flags; };
    void setTimeStamp(ConnectionTimeStamp timeStamp) { m_timeStamp = timeStamp; };

private:

    //connection note
    ConnectionNote m_note;
    
    ConnectionDirection m_directions[2];

    ConnectionTimeStamp m_timeStamp;

    //Index to rooms
    Room* m_rooms[2];

    //doors (in case of exit with doors)
    Door* m_doors[2];

    //type of connections
    ConnectionType m_type;
    ConnectionFlags m_flags;

    quint32 m_index;
};

#endif
