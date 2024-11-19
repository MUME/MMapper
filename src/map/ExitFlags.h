#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Flags.h"

#include <cstdint>

// X(UPPER_CASE, lower_case, CamelCase, "Friendly name")
#define XFOREACH_EXIT_FLAG(X) \
    X(EXIT, exit, Exit, "Exit") \
    X(DOOR, door, Door, "Door") \
    X(ROAD, road, Road, "Road") \
    X(CLIMB, climb, Climb, "Climb") \
    X(RANDOM, random, Random, "Random") \
    X(SPECIAL, special, Special, "Special") \
    X(NO_MATCH, no_match, NoMatch, "No match") \
    X(FLOW, flow, Flow, "Water flow") \
    X(NO_FLEE, no_flee, NoFlee, "No flee") \
    X(DAMAGE, damage, Damage, "Damage") \
    X(FALL, fall, Fall, "Fall") \
    X(GUARDED, guarded, Guarded, "Guarded") \
    /* define exit flags above */

enum class NODISCARD ExitFlagEnum : uint8_t {
#define X_DECL_EXIT_FLAG(UPPER_CASE, lower_case, CamelCase, friendly) UPPER_CASE,
    XFOREACH_EXIT_FLAG(X_DECL_EXIT_FLAG)
#undef X_DECL_EXIT_FLAG
};

#define X_COUNT(UPPER_CASE, lower_case, CamelCase, friendly) +1
static constexpr const int NUM_EXIT_FLAGS = XFOREACH_EXIT_FLAG(X_COUNT);
#undef X_COUNT
DEFINE_ENUM_COUNT(ExitFlagEnum, NUM_EXIT_FLAGS)

class NODISCARD ExitFlags final : public enums::Flags<ExitFlags, ExitFlagEnum, uint16_t>
{
public:
    using Flags::Flags;

public:
#define X_DEFINE_ACCESSORS(UPPER_CASE, lower_case, CamelCase, friendly) \
    NODISCARD bool is##CamelCase() const \
    { \
        return contains(ExitFlagEnum::UPPER_CASE); \
    }
    XFOREACH_EXIT_FLAG(X_DEFINE_ACCESSORS)
#undef X_DEFINE_ACCESSORS
};

NODISCARD inline constexpr const ExitFlags operator|(ExitFlagEnum lhs, ExitFlagEnum rhs) noexcept
{
    return ExitFlags{lhs} | ExitFlags{rhs};
}

NODISCARD extern std::string_view to_string_view(ExitFlagEnum flag);
NODISCARD extern std::string_view getName(ExitFlagEnum flag);
