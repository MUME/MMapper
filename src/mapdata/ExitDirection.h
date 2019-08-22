#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <array>
#include <cstdint>

enum class ExitDirection { NORTH = 0, SOUTH, EAST, WEST, UP, DOWN, UNKNOWN, NONE };

static constexpr const uint32_t NUM_EXITS_NESW = 4u;
static constexpr const uint32_t NUM_EXITS_NESWUD = 6u;
static constexpr const uint32_t NUM_EXITS = 7u;
static constexpr const uint32_t NUM_EXITS_INCLUDING_NONE = 8u;

namespace enums {
const std::array<ExitDirection, NUM_EXITS_NESW> &getAllExitsNESW();
const std::array<ExitDirection, NUM_EXITS_NESWUD> &getAllExitsNESWUD();
const std::array<ExitDirection, NUM_EXITS> &getAllExits7();

#define ALL_EXITS_NESW enums::getAllExitsNESW()
#define ALL_EXITS_NESWUD enums::getAllExitsNESWUD()
#define ALL_EXITS7 enums::getAllExits7()

} // namespace enums

// TODO: move these to another namespace once all of the exit types are merged

bool isNESW(ExitDirection dir);
bool isUpDown(ExitDirection dir);
bool isNESWUD(ExitDirection dir);
ExitDirection opposite(ExitDirection in);
const char *lowercaseDirection(ExitDirection dir);

namespace Mmapper2Exit {

ExitDirection dirForChar(const char dir);

char charForDir(ExitDirection dir);
} // namespace Mmapper2Exit
