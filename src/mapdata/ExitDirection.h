#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstdint>

#include "../global/Array.h"
#include "ExitFlags.h"

enum class ExitDirEnum { NORTH = 0, SOUTH, EAST, WEST, UP, DOWN, UNKNOWN, NONE };

static constexpr const uint32_t NUM_EXITS_NESW = 4u;
static constexpr const uint32_t NUM_EXITS_NESWUD = 6u;
static constexpr const uint32_t NUM_EXITS = 7u;
static constexpr const uint32_t NUM_EXITS_INCLUDING_NONE = 8u;

namespace enums {
const MMapper::Array<ExitDirEnum, NUM_EXITS_NESW> &getAllExitsNESW();
const MMapper::Array<ExitDirEnum, NUM_EXITS_NESWUD> &getAllExitsNESWUD();
const MMapper::Array<ExitDirEnum, NUM_EXITS> &getAllExits7();

#define ALL_EXITS_NESW enums::getAllExitsNESW()
#define ALL_EXITS_NESWUD enums::getAllExitsNESWUD()
#define ALL_EXITS7 enums::getAllExits7()

} // namespace enums

// TODO: move these to another namespace once all of the exit types are merged

bool isNESW(ExitDirEnum dir);
bool isUpDown(ExitDirEnum dir);
bool isNESWUD(ExitDirEnum dir);
ExitDirEnum opposite(ExitDirEnum in);
const char *lowercaseDirection(ExitDirEnum dir);

namespace Mmapper2Exit {

ExitDirEnum dirForChar(char dir);

char charForDir(ExitDirEnum dir);
} // namespace Mmapper2Exit
