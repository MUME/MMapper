#pragma once
/************************************************************************
**
** Authors:   Azazello <lachupe@gmail.com>,
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#ifndef MMAPPER2CHARACTER_H
#define MMAPPER2CHARACTER_H

#include <QMetaType>
#include <QtGlobal>

#include "../global/Flags.h"

// X(UPPER_CASE, lower_case, CamelCase, "Friendly Name")
#define X_FOREACH_CHARACTER_POSITION(X) \
    X(UNDEFINED, undefined, Undefined, "No state available") \
    X(FIGHTING, fighting, Fighting, "Fighting") \
    X(STANDING, standing, Standing, "Standing") \
    X(SITTING, sitting, Sitting, "Sitting") \
    X(RESTING, resting, Resting, "Resting") \
    X(SLEEPING, sleeping, Sleeping, "Sleeping") \
    X(INCAPACITATED, incapacitated, Incapacitated, "Incapacitated") \
    X(DEAD, dead, Dead, "Dead") \
    /* define character positions above */

enum class CharacterPosition {
#define X_DECL_CHARACTER_POSITION(UPPER_CASE, lower_case, CamelCase, friendly) UPPER_CASE,
    X_FOREACH_CHARACTER_POSITION(X_DECL_CHARACTER_POSITION)
#undef X_DECL_CHARACTER_POSITION
};

#define X_COUNT(UPPER_CASE, lower_case, CamelCase, friendly) +1
static constexpr const int NUM_CHARACTER_POSITIONS = X_FOREACH_CHARACTER_POSITION(X_COUNT);
#undef X_COUNT
DEFINE_ENUM_COUNT(CharacterPosition, NUM_CHARACTER_POSITIONS)
Q_DECLARE_METATYPE(CharacterPosition)

// X(UPPER_CASE, lower_case, CamelCase, "Friendly Name")
#define X_FOREACH_CHARACTER_AFFECT(X) \
    X(BLIND, blind, Blind, "Blind") \
    X(BASHED, bashed, Bashed, "Bashed") \
    X(SLEPT, slept, Slept, "Slept") \
    X(POISONED, poisoned, Poisoned, "Poisoned") \
    X(BLEEDING, bleeding, Bleeding, "Bleeding") \
    X(HUNGRY, hungry, Hungry, "Hungry") \
    X(THIRSTY, thirsty, Thirsty, "Thirsty") \
    /* define character affects above */

// TODO: States for CASTING FLUSHING DISEASED
enum class CharacterAffect {
#define X_DECL_CHARACTER_AFFECT(UPPER_CASE, lower_case, CamelCase, friendly) UPPER_CASE,
    X_FOREACH_CHARACTER_AFFECT(X_DECL_CHARACTER_AFFECT)
#undef X_DECL_CHARACTER_AFFECT
};

#define X_COUNT(UPPER_CASE, lower_case, CamelCase, friendly) +1
static constexpr const int NUM_CHARACTER_AFFECTS = X_FOREACH_CHARACTER_AFFECT(X_COUNT);
#undef X_COUNT
DEFINE_ENUM_COUNT(CharacterAffect, NUM_CHARACTER_AFFECTS)
Q_DECLARE_METATYPE(CharacterAffect)

class CharacterAffects final : public enums::Flags<CharacterAffects, CharacterAffect, uint32_t>
{
    using Flags::Flags;

public:
#define X_DEFINE_ACCESSORS(UPPER_CASE, lower_case, CamelCase, friendly) \
    bool is##CamelCase() const { return contains(CharacterAffect::UPPER_CASE); }
    X_FOREACH_CHARACTER_AFFECT(X_DEFINE_ACCESSORS)
#undef X_DEFINE_ACCESSORS
};

inline constexpr const CharacterAffects operator|(CharacterAffect lhs, CharacterAffect rhs) noexcept
{
    return CharacterAffects{lhs} | CharacterAffects{rhs};
}

#endif // MMAPPER2CHARACTER_H
