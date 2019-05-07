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

enum class CharacterPosition {
    UNDEFINED = 0,
    FIGHTING,
    STANDING,
    SITTING,
    RESTING,
    SLEEPING,
    INCAPACITATED,
    DEAD
};
static constexpr const size_t NUM_CHARACTER_POSITIONS = static_cast<size_t>(CharacterPosition::DEAD)
                                                        + 1u;
static_assert(NUM_CHARACTER_POSITIONS == 8);
DEFINE_ENUM_COUNT(CharacterPosition, NUM_CHARACTER_POSITIONS)
Q_DECLARE_METATYPE(CharacterPosition)

// TODO: States for BLEEDING CASTING FLUSHING DISEASED HUNGRY THIRSTY
enum class CharacterAffect { BLIND, BASHED, SLEPT, POISONED };
static constexpr const int NUM_CHARACTER_AFFECTS = static_cast<int>(CharacterAffect::POISONED) + 1;
static_assert(NUM_CHARACTER_AFFECTS == 4);
DEFINE_ENUM_COUNT(CharacterAffect, NUM_CHARACTER_AFFECTS)
Q_DECLARE_METATYPE(CharacterAffect)

class CharacterAffects final : public enums::Flags<CharacterAffects, CharacterAffect, uint32_t>
{
    using Flags::Flags;
};

inline constexpr const CharacterAffects operator|(CharacterAffect lhs, CharacterAffect rhs) noexcept
{
    return CharacterAffects{lhs} | CharacterAffects{rhs};
}

#endif // MMAPPER2CHARACTER_H
