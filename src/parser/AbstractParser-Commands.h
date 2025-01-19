#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Flags.h"
#include "../map/DoorFlags.h"
#include "../map/ExitFlags.h"
#include "../map/infomark.h"
#include "../map/mmapper2room.h"
#include "Abbrev.h"
#include "DoorAction.h"

#include <QByteArray>

extern const Abbrev cmdBack;
extern const Abbrev cmdDirections;
extern const Abbrev cmdDoorHelp;
extern const Abbrev cmdGroupTell;
extern const Abbrev cmdHelp;
extern const Abbrev cmdMarkCurrent;
extern const Abbrev cmdRemoveDoorNames;
extern const Abbrev cmdSearch;
extern const Abbrev cmdSet;
extern const Abbrev cmdTime;
extern const Abbrev cmdTrollExit;
extern const Abbrev cmdVote;

NODISCARD QByteArray getCommandName(DoorActionEnum action);

NODISCARD Abbrev getParserCommandName(DoorActionEnum action);
NODISCARD Abbrev getParserCommandName(DoorFlagEnum x);
NODISCARD Abbrev getParserCommandName(ExitFlagEnum x);
NODISCARD Abbrev getParserCommandName(InfoMarkClassEnum x);
NODISCARD Abbrev getParserCommandName(RoomAlignEnum x);
NODISCARD Abbrev getParserCommandName(RoomLightEnum x);
NODISCARD Abbrev getParserCommandName(RoomLoadFlagEnum x);
NODISCARD Abbrev getParserCommandName(RoomMobFlagEnum x);
NODISCARD Abbrev getParserCommandName(RoomPortableEnum x);
NODISCARD Abbrev getParserCommandName(RoomRidableEnum x);
NODISCARD Abbrev getParserCommandName(RoomSundeathEnum x);
NODISCARD Abbrev getParserCommandName(RoomTerrainEnum x);
