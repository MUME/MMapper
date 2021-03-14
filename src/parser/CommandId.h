#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Array.h"

#include "../mapdata/ExitDirection.h"

enum class NODISCARD CommandEnum {
    NORTH = 0,
    SOUTH,
    EAST,
    WEST,
    UP,
    DOWN,
    UNKNOWN,
    LOOK,
    FLEE,
    SCOUT,
    /*SYNC, RESET, */
    NONE
};
static_assert(CommandEnum::FLEE > CommandEnum::UNKNOWN, "Code expects FLEE to be above UNKNOWN");
static_assert(CommandEnum::FLEE > CommandEnum::LOOK, "Code expects FLEE to be above LOOK");
static_assert(CommandEnum::FLEE < CommandEnum::SCOUT, "Code expects FLEE to be below SCOUT");
static_assert(CommandEnum::FLEE < CommandEnum::NONE, "Code expects FLEE to be below NONE");

/* does not include NONE */
static constexpr const int NUM_COMMANDS = 10;

namespace enums {

#define ALL_COMMANDS ::enums::getAllCommands()
NODISCARD const MMapper::Array<CommandEnum, NUM_COMMANDS> &getAllCommands();

} // namespace enums

NODISCARD bool isDirectionNESWUD(CommandEnum cmd);
NODISCARD bool isDirection7(CommandEnum cmd);
NODISCARD ExitDirEnum getDirection(CommandEnum cmd);
NODISCARD CommandEnum getCommand(ExitDirEnum dir);

NODISCARD const char *getUppercase(CommandEnum cmd);
NODISCARD const char *getLowercase(CommandEnum cmd);
