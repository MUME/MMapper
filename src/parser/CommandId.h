#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <array>

#include "../mapdata/ExitDirection.h"

enum class CommandIdType {
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
static_assert(CommandIdType::FLEE > CommandIdType::UNKNOWN, "Code expects FLEE to be above UNKNOWN");
static_assert(CommandIdType::FLEE > CommandIdType::LOOK, "Code expects FLEE to be above LOOK");
static_assert(CommandIdType::FLEE < CommandIdType::SCOUT, "Code expects FLEE to be below SCOUT");
static_assert(CommandIdType::FLEE < CommandIdType::NONE, "Code expects FLEE to be below NONE");

/* does not include NONE */
static constexpr const int NUM_COMMANDS = 10;

namespace enums {

#define ALL_COMMANDS ::enums::getAllCommands()
const std::array<CommandIdType, NUM_COMMANDS> &getAllCommands();

} // namespace enums

bool isDirectionNESWUD(const CommandIdType cmd);
bool isDirection7(const CommandIdType cmd);
ExitDirection getDirection(const CommandIdType cmd);

const char *getUppercase(const CommandIdType cmd);
const char *getLowercase(const CommandIdType cmd);
