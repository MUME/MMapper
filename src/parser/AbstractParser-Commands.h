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

static constexpr const auto staticRoomFields = RoomFieldEnum::NAME | RoomFieldEnum::DESC;
static constexpr const auto dynamicRoomFields = staticRoomFields | RoomFieldEnum::DYNAMIC_DESC;

extern const Abbrev cmdBack;
extern const Abbrev cmdDirections;
extern const Abbrev cmdDoorHelp;
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

QByteArray getCommandName(DoorActionEnum action);

Abbrev getParserCommandName(DoorActionEnum action);
Abbrev getParserCommandName(DoorFlagEnum x);
Abbrev getParserCommandName(ExitFlagEnum x);
Abbrev getParserCommandName(RoomAlignEnum x);
Abbrev getParserCommandName(RoomLightEnum x);
Abbrev getParserCommandName(RoomLoadFlagEnum x);
Abbrev getParserCommandName(RoomMobFlagEnum x);
Abbrev getParserCommandName(RoomPortableEnum x);
Abbrev getParserCommandName(RoomRidableEnum x);
Abbrev getParserCommandName(RoomSundeathEnum x);
Abbrev getParserCommandName(RoomTerrainEnum x);
