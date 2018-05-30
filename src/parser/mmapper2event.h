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

#ifndef MMAPPER2EVENT_H
#define MMAPPER2EVENT_H

#include "defs.h"

#include <QtGlobal>

class QString;
class ParseEvent;

enum CommandIdType {
    CID_NORTH = 0,
    CID_SOUTH,
    CID_EAST,
    CID_WEST,
    CID_UP,
    CID_DOWN,
    CID_UNKNOWN,
    CID_LOOK,
    CID_FLEE,
    CID_SCOUT,
    /*CID_SYNC, CID_RESET, */ CID_NONE
};

enum DoorActionType {
    DAT_OPEN,
    DAT_CLOSE,
    DAT_LOCK,
    DAT_UNLOCK,
    DAT_PICK,
    DAT_ROCK,
    DAT_BASH,
    DAT_BREAK,
    DAT_BLOCK,
    DAT_NONE
};

// bit1 through bit24
// EF_EXIT, EF_DOOR, EF_ROAD, EF_CLIMB
#define EXITS_FLAGS_VALID bit31
typedef quint32 ExitsFlagsType;

// bit1 through bit12
#define DIRECT_SUN_ROOM bit1
#define INDIRECT_SUN_ROOM bit2

#define ANY_DIRECT_SUNLIGHT (bit1 + bit3 + bit5 + bit9 + bit11)
#define CONNECTED_ROOM_FLAGS_VALID bit15
typedef quint16 ConnectedRoomFlagsType;

// bit0-3 -> char representation of RoomTerrainType
#define TERRAIN_TYPE (bit1 + bit2 + bit3 + bit4)
#define LIT_ROOM bit5
#define DARK_ROOM bit6
#define PROMPT_FLAGS_VALID bit7
typedef quint8 PromptFlagsType;

namespace Mmapper2Event {
ParseEvent *createEvent(const CommandIdType &c,
                        const QString &roomName,
                        const QString &dynamicDesc,
                        const QString &staticDesc,
                        const ExitsFlagsType &exitFlags,
                        const PromptFlagsType &promptFlags,
                        const ConnectedRoomFlagsType &connectedRoomFlags);

QString getRoomName(const ParseEvent &e);

QString getRoomDesc(const ParseEvent &e);

QString getParsedRoomDesc(const ParseEvent &e);

ExitsFlagsType getExitFlags(const ParseEvent &e);

PromptFlagsType getPromptFlags(const ParseEvent &e);

ConnectedRoomFlagsType getConnectedRoomFlags(const ParseEvent &e);
} // namespace Mmapper2Event
#endif
