#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#ifndef MMAPPER_EXITFLAGS_H
#define MMAPPER_EXITFLAGS_H

#include <cstdint>

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
DEFINE_ENUM_COUNT(ExitFlag, NUM_EXIT_FLAGS);

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

#endif //MMAPPER_EXITFLAGS_H
