#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Flags.h"

#include <cstdint>

// X(UPPER_CASE, lower_case, CamelCase, "Friendly Name")
#define XFOREACH_DOOR_FLAG(X) \
    X(HIDDEN, hidden, Hidden, "Hidden") \
    X(NEED_KEY, need_key, NeedKey, "Need key") \
    X(NO_BLOCK, no_block, NoBlock, "No block") \
    X(NO_BREAK, no_break, NoBreak, "No break") \
    X(NO_PICK, no_pick, NoPick, "No pick") \
    X(DELAYED, delayed, Delayed, "Delayed") \
    X(CALLABLE, callable, Callable, "Callable") \
    X(KNOCKABLE, knockable, Knockable, "Knockable") \
    X(MAGIC, magic, Magic, "Magic") \
    X(ACTION, action, Action, "Action-controlled") \
    X(NO_BASH, no_bash, NoBash, "No bash") \
    /* define door flags above */

enum class NODISCARD DoorFlagEnum : uint8_t {
#define X_DECL_DOOR_FLAG(UPPER_CASE, lower_case, CamelCase, friendly) UPPER_CASE,
    XFOREACH_DOOR_FLAG(X_DECL_DOOR_FLAG)
#undef X_DECL_DOOR_FLAG
};

#define X_COUNT(UPPER_CASE, lower_case, CamelCase, friendly) +1
static constexpr const int NUM_DOOR_FLAGS = XFOREACH_DOOR_FLAG(X_COUNT);
#undef X_COUNT
DEFINE_ENUM_COUNT(DoorFlagEnum, NUM_DOOR_FLAGS)

class NODISCARD DoorFlags final : public enums::Flags<DoorFlags, DoorFlagEnum, uint16_t>
{
public:
    using Flags::Flags;

public:
#define X_DEFINE_ACCESSORS(UPPER_CASE, lower_case, CamelCase, friendly) \
    NODISCARD bool is##CamelCase() const { return contains(DoorFlagEnum::UPPER_CASE); }
    XFOREACH_DOOR_FLAG(X_DEFINE_ACCESSORS)
#undef X_DEFINE_ACCESSORS

public:
    // REVISIT: this name is different from the rest
    NODISCARD bool needsKey() const { return isNeedKey(); }
};

NODISCARD inline constexpr const DoorFlags operator|(DoorFlagEnum lhs, DoorFlagEnum rhs) noexcept
{
    return DoorFlags{lhs} | DoorFlags{rhs};
}

NODISCARD extern std::string_view to_string_view(DoorFlagEnum flag);
NODISCARD extern std::string_view getName(DoorFlagEnum flag);
