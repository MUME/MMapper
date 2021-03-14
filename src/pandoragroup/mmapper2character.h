#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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

enum class NODISCARD CharacterPositionEnum {
#define X_DECL_CHARACTER_POSITION(UPPER_CASE, lower_case, CamelCase, friendly) UPPER_CASE,
    X_FOREACH_CHARACTER_POSITION(X_DECL_CHARACTER_POSITION)
#undef X_DECL_CHARACTER_POSITION
};

#define X_COUNT(UPPER_CASE, lower_case, CamelCase, friendly) +1
static constexpr const int NUM_CHARACTER_POSITIONS = X_FOREACH_CHARACTER_POSITION(X_COUNT);
#undef X_COUNT
DEFINE_ENUM_COUNT(CharacterPositionEnum, NUM_CHARACTER_POSITIONS)
Q_DECLARE_METATYPE(CharacterPositionEnum)

// X(UPPER_CASE, lower_case, CamelCase, "Friendly Name")
#define X_FOREACH_CHARACTER_AFFECT(X) \
    X(BLIND, blind, Blind, "Blind") \
    X(BASHED, bashed, Bashed, "Bashed") \
    X(SLEPT, slept, Slept, "Slept") \
    X(POISONED, poisoned, Poisoned, "Poisoned") \
    X(BLEEDING, bleeding, Bleeding, "Bleeding") \
    X(HUNGRY, hungry, Hungry, "Hungry") \
    X(THIRSTY, thirsty, Thirsty, "Thirsty") \
    X(SNARED, snared, snared, "Snared") \
    X(SEARCH, search, Search, "Searching") \
    X(RIDING, riding, Riding, "Riding") \
    /* define character affects above */

// TODO: States for CASTING DISEASED
enum class NODISCARD CharacterAffectEnum {
#define X_DECL_CHARACTER_AFFECT(UPPER_CASE, lower_case, CamelCase, friendly) UPPER_CASE,
    X_FOREACH_CHARACTER_AFFECT(X_DECL_CHARACTER_AFFECT)
#undef X_DECL_CHARACTER_AFFECT
};

#define X_COUNT(UPPER_CASE, lower_case, CamelCase, friendly) +1
static constexpr const int NUM_CHARACTER_AFFECTS = X_FOREACH_CHARACTER_AFFECT(X_COUNT);
#undef X_COUNT
DEFINE_ENUM_COUNT(CharacterAffectEnum, NUM_CHARACTER_AFFECTS)
Q_DECLARE_METATYPE(CharacterAffectEnum)

class NODISCARD CharacterAffects final
    : public enums::Flags<CharacterAffects, CharacterAffectEnum, uint32_t>
{
    using Flags::Flags;

public:
#define X_DEFINE_ACCESSORS(UPPER_CASE, lower_case, CamelCase, friendly) \
    NODISCARD bool is##CamelCase() const { return contains(CharacterAffectEnum::UPPER_CASE); }
    X_FOREACH_CHARACTER_AFFECT(X_DEFINE_ACCESSORS)
#undef X_DEFINE_ACCESSORS
};

NODISCARD inline constexpr const CharacterAffects operator|(CharacterAffectEnum lhs,
                                                            CharacterAffectEnum rhs) noexcept
{
    return CharacterAffects{lhs} | CharacterAffects{rhs};
}
