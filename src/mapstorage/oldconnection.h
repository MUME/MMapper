/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper2 project. 
** Maintained by Marek Krejza <krejza@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file COPYING included in the packaging of
** this file.  
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
*************************************************************************/

#ifndef OLDCONNECTION_H
#define OLDCONNECTION_H
#include <QDateTime>
#include "exit.h"
#include "defs.h"
#include "olddoor.h"
#include "parser.h"

class Room;
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
    
    ConnectionNote getNote(){ return m_note; };
    quint32 getIndex() { return m_index; };
    Room* getRoom(quint8 idx){ return m_rooms[idx]; };
    Room* getTargetRoom(Room* r){ Room* tmp; r == m_rooms[FIRST] ? tmp = m_rooms[SECOND] : tmp = m_rooms[FIRST]; return tmp; };
    Door* getDoor(Room* r){ Door* tmp; r == m_rooms[FIRST] ? tmp = m_doors[FIRST] : tmp = m_doors[SECOND]; return tmp; };
    Door* getDoor(quint8 idx){ return m_doors[idx]; };
    ConnectionDirection getDirection(Room* r){ConnectionDirection tmp; r == m_rooms[FIRST] ? tmp = m_directions[FIRST] : tmp = m_directions[SECOND]; return tmp; };
    ConnectionDirection getTargetDirection(Room* r){ConnectionDirection tmp; r == m_rooms[FIRST] ? tmp = m_directions[SECOND] : tmp = m_directions[FIRST]; return tmp; };
    ConnectionDirection getDirection(quint8 idx){ return m_directions[idx]; };
    ConnectionType getType() { return m_type; };
    ConnectionFlags getFlags() { return m_flags; };
    ConnectionTimeStamp getTimeStamp() { return m_timeStamp; };

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
