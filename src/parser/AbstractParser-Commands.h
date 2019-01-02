#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
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

#ifndef MMAPPER_ABSTRACTPARSER_COMMANDS_H
#define MMAPPER_ABSTRACTPARSER_COMMANDS_H

#include <QByteArray>

#include "../global/Flags.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitFlags.h"
#include "../mapdata/mmapper2room.h"
#include "Abbrev.h"
#include "DoorAction.h"

static constexpr const auto staticRoomFields = RoomField::NAME | RoomField::DESC;
static constexpr const auto dynamicRoomFields = staticRoomFields | RoomField::DYNAMIC_DESC;

extern const Abbrev cmdBack;
extern const Abbrev cmdDirections;
extern const Abbrev cmdDoorHelp;
extern const Abbrev cmdGroupHelp;
extern const Abbrev cmdGroupTell;
extern const Abbrev cmdHelp;
extern const Abbrev cmdMapHelp;
extern const Abbrev cmdMarkCurrent;
extern const Abbrev cmdName;
extern const Abbrev cmdNote;
extern const Abbrev cmdRemoveDoorNames;
extern const Abbrev cmdSearch;
extern const Abbrev cmdSet;
extern const Abbrev cmdTime;
extern const Abbrev cmdTrollExit;
extern const Abbrev cmdVote;
extern const Abbrev cmdPDynamic;
extern const Abbrev cmdPStatic;
extern const Abbrev cmdPNote;
extern const Abbrev cmdPrint;

QByteArray getCommandName(DoorActionType action);

Abbrev getParserCommandName(DoorActionType action);
Abbrev getParserCommandName(DoorFlag x);
Abbrev getParserCommandName(ExitFlag x);
Abbrev getParserCommandName(RoomAlignType x);
Abbrev getParserCommandName(RoomLightType x);
Abbrev getParserCommandName(RoomLoadFlag x);
Abbrev getParserCommandName(RoomMobFlag x);
Abbrev getParserCommandName(RoomPortableType x);
Abbrev getParserCommandName(RoomRidableType x);
Abbrev getParserCommandName(RoomSundeathType x);
Abbrev getParserCommandName(RoomTerrainType x);

#endif // MMAPPER_ABSTRACTPARSER_COMMANDS_H
