#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstdint>

#include "../global/Flags.h"

// X(UPPER_CASE, lower_case, CamelCase, "Friendly name")
#define X_FOREACH_EXIT_FLAG(X) \
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

enum class ExitFlag {
#define X_DECL_EXIT_FLAG(UPPER_CASE, lower_case, CamelCase, friendly) UPPER_CASE,
    X_FOREACH_EXIT_FLAG(X_DECL_EXIT_FLAG)
#undef X_DECL_EXIT_FLAG
};

#define X_COUNT(UPPER_CASE, lower_case, CamelCase, friendly) +1
static constexpr const int NUM_EXIT_FLAGS = X_FOREACH_EXIT_FLAG(X_COUNT);
#undef X_COUNT
DEFINE_ENUM_COUNT(ExitFlag, NUM_EXIT_FLAGS)

class ExitFlags final : public enums::Flags<ExitFlags, ExitFlag, uint16_t>
{
public:
    using Flags::Flags;

public:
#define X_DEFINE_ACCESSORS(UPPER_CASE, lower_case, CamelCase, friendly) \
    bool is##CamelCase() const { return contains(ExitFlag::UPPER_CASE); }
    X_FOREACH_EXIT_FLAG(X_DEFINE_ACCESSORS)
#undef X_DEFINE_ACCESSORS
};

inline constexpr const ExitFlags operator|(ExitFlag lhs, ExitFlag rhs) noexcept
{
    return ExitFlags{lhs} | ExitFlags{rhs};
}
