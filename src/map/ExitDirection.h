#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Array.h"
#include "ExitFlags.h"

#include <cstdint>

class Coordinate;

enum class NODISCARD ExitDirEnum : uint8_t {
    NORTH = 0,
    SOUTH,
    EAST,
    WEST,
    UP,
    DOWN,
    UNKNOWN,
    NONE
};

static constexpr const uint32_t NUM_EXITS_NESW = 4u;
static constexpr const uint32_t NUM_EXITS_NESWUD = 6u;
static constexpr const uint32_t NUM_EXITS = 7u;
static constexpr const uint32_t NUM_EXITS_INCLUDING_NONE = 8u;

namespace enums {
NODISCARD const MMapper::Array<ExitDirEnum, NUM_EXITS_NESW> &getAllExitsNESW();
NODISCARD const MMapper::Array<ExitDirEnum, NUM_EXITS_NESWUD> &getAllExitsNESWUD();
NODISCARD const MMapper::Array<ExitDirEnum, NUM_EXITS> &getAllExits7();

#define ALL_EXITS_NESW enums::getAllExitsNESW()
#define ALL_EXITS_NESWUD enums::getAllExitsNESWUD()
#define ALL_EXITS7 enums::getAllExits7()

} // namespace enums

// TODO: move these to another namespace once all of the exit types are merged

NODISCARD extern bool isNESW(ExitDirEnum dir);
NODISCARD extern bool isUpDown(ExitDirEnum dir);
NODISCARD extern bool isNESWUD(ExitDirEnum dir);
NODISCARD extern ExitDirEnum opposite(ExitDirEnum in);
NODISCARD extern const char *lowercaseDirection(ExitDirEnum dir);

struct NODISCARD ExitDirFlags final
    : public enums::Flags<ExitDirFlags, ExitDirEnum, uint8_t, NUM_EXITS_INCLUDING_NONE>
{
public:
    using Flags::Flags;
};

namespace Mmapper2Exit {

NODISCARD ExitDirEnum dirForChar(char dir);
NODISCARD ExitDirEnum dirForChar(QChar dir);

NODISCARD char charForDir(ExitDirEnum dir);
} // namespace Mmapper2Exit

NODISCARD extern const Coordinate &exitDir(ExitDirEnum dir);
NODISCARD extern const std::string_view to_string_view(ExitDirEnum dir);
