#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
