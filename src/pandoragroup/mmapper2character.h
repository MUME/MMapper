#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Flags.h"
#include "../global/TaggedString.h"

#include <QMetaType>
#include <QtGlobal>

namespace tags {
struct NODISCARD CharacterNameTag final
{
    NODISCARD static bool isValid(std::string_view sv);
};
struct NODISCARD CharacterLabelTag final
{
    NODISCARD static bool isValid(std::string_view sv);
};
struct NODISCARD CharacterRoomNameTag final
{
    NODISCARD static bool isValid(std::string_view sv);
};
} // namespace tags

using CharacterName = TaggedBoxedStringUtf8<tags::CharacterNameTag>;
using CharacterLabel = TaggedBoxedStringUtf8<tags::CharacterLabelTag>;
using CharacterRoomName = TaggedBoxedStringUtf8<tags::CharacterRoomNameTag>;

// X(UPPER_CASE, lower_case, CamelCase, "Friendly Name")
#define XFOREACH_CHARACTER_POSITION(X) \
    X(UNDEFINED, undefined, Undefined, "No state available") \
    X(DEAD, dead, Dead, "Dead") \
    X(INCAPACITATED, incapacitated, Incapacitated, "Incapacitated") \
    X(SLEEPING, sleeping, Sleeping, "Sleeping") \
    X(RESTING, resting, Resting, "Resting") \
    X(SITTING, sitting, Sitting, "Sitting") \
    X(STANDING, standing, Standing, "Standing") \
    X(FIGHTING, fighting, Fighting, "Fighting") \
    /* define character positions above */

enum class NODISCARD CharacterPositionEnum : uint8_t {
#define X_DECL_CHARACTER_POSITION(UPPER_CASE, lower_case, CamelCase, friendly) UPPER_CASE,
    XFOREACH_CHARACTER_POSITION(X_DECL_CHARACTER_POSITION)
#undef X_DECL_CHARACTER_POSITION
};

#define X_COUNT(UPPER_CASE, lower_case, CamelCase, friendly) +1
static constexpr const int NUM_CHARACTER_POSITIONS = XFOREACH_CHARACTER_POSITION(X_COUNT);
#undef X_COUNT
DEFINE_ENUM_COUNT(CharacterPositionEnum, NUM_CHARACTER_POSITIONS)
Q_DECLARE_METATYPE(CharacterPositionEnum)

// X(UPPER_CASE, lower_case, CamelCase, "Friendly Name")
#define XFOREACH_CHARACTER_TYPE(X) \
    X(UNDEFINED, undefined, Undefined, "No state available") \
    X(YOU, you, You, "You") \
    X(NPC, npc, Npc, "NPC") \
    X(ALLY, ally, Ally, "Ally") \
    X(ENEMY, enemy, Enemy, "Enemy") \
    X(NEUTRAL, neutral, Neutral, "Neutral") \
    /* define character types above */

enum class NODISCARD CharacterTypeEnum : uint8_t {
#define X_DECL_CHARACTER_TYPE(UPPER_CASE, lower_case, CamelCase, friendly) UPPER_CASE,
    XFOREACH_CHARACTER_TYPE(X_DECL_CHARACTER_TYPE)
#undef X_DECL_CHARACTER_TYPE
};

#define X_COUNT(UPPER_CASE, lower_case, CamelCase, friendly) +1
static constexpr const int NUM_CHARACTER_TYPES = XFOREACH_CHARACTER_TYPE(X_COUNT);
#undef X_COUNT
DEFINE_ENUM_COUNT(CharacterTypeEnum, NUM_CHARACTER_TYPES)
Q_DECLARE_METATYPE(CharacterTypeEnum)

// X(UPPER_CASE, lower_case, CamelCase, "Friendly Name")
#define XFOREACH_CHARACTER_AFFECT(X) \
    X(RIDE, ride, ride, "Riding") \
    X(BLIND, blind, Blind, "Blind") \
    X(BASHED, bashed, Bashed, "Bashed") \
    X(SLEPT, slept, Slept, "Slept") \
    X(POISON, poison, Poison, "Poisoned") \
    X(WOUND, wound, Wound, "Wounded") \
    X(HUNGRY, hungry, Hungry, "Hungry") \
    X(THIRSTY, thirsty, Thirsty, "Thirsty") \
    X(SNARED, snared, Snared, "Snared") \
    X(WAITING, waiting, Waiting, "Waiting") \
    /* define character affects above in display priority order */

// TODO: States for CASTING DISEASED
enum class NODISCARD CharacterAffectEnum {
#define X_DECL_CHARACTER_AFFECT(UPPER_CASE, lower_case, CamelCase, friendly) UPPER_CASE,
    XFOREACH_CHARACTER_AFFECT(X_DECL_CHARACTER_AFFECT)
#undef X_DECL_CHARACTER_AFFECT
};

#define X_COUNT(UPPER_CASE, lower_case, CamelCase, friendly) +1
static constexpr const int NUM_CHARACTER_AFFECTS = XFOREACH_CHARACTER_AFFECT(X_COUNT);
#undef X_COUNT
DEFINE_ENUM_COUNT(CharacterAffectEnum, NUM_CHARACTER_AFFECTS)
Q_DECLARE_METATYPE(CharacterAffectEnum)

class NODISCARD CharacterAffectFlags final
    : public enums::Flags<CharacterAffectFlags, CharacterAffectEnum, uint32_t>
{
    using Flags::Flags;

public:
#define X_DEFINE_ACCESSORS(UPPER_CASE, lower_case, CamelCase, friendly) \
    NODISCARD bool is##CamelCase() const \
    { \
        return contains(CharacterAffectEnum::UPPER_CASE); \
    }
    XFOREACH_CHARACTER_AFFECT(X_DEFINE_ACCESSORS)
#undef X_DEFINE_ACCESSORS
};

NODISCARD inline constexpr const CharacterAffectFlags operator|(CharacterAffectEnum lhs,
                                                                CharacterAffectEnum rhs) noexcept
{
    return CharacterAffectFlags{lhs} | CharacterAffectFlags{rhs};
}
