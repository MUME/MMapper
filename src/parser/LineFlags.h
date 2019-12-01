#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstdint>

#include "../global/Flags.h"

// X(UPPER_CASE, lower_case, CamelCase, "Friendly name")
#define X_FOREACH_LINE_FLAG(X) \
    X(NONE, none, None, "None") \
    X(ROOM, room, Room, "Room") \
    X(NAME, name, Name, "Name") \
    X(DESCRIPTION, description, Description, "Description") \
    X(EXITS, exits, Exits, "Exits") \
    X(PROMPT, prompt, Prompt, "Prompt") \
    X(TERRAIN, terrain, Terrain, "Terrain") \
    X(HEADER, header, Header, "Header") \
    X(WEATHER, weather, Weather, "Weather") \
    X(STATUS, status, Status, "Status") \
    X(SNOOP, snoop, Snoop, "Snoop") \
    /* define line flags above */

enum class LineFlagEnum : uint32_t {
#define X_DECL_LINE_FLAG(UPPER_CASE, lower_case, CamelCase, friendly) UPPER_CASE,
    X_FOREACH_LINE_FLAG(X_DECL_LINE_FLAG)
#undef X_DECL_LINE_FLAG
};

#define X_COUNT(UPPER_CASE, lower_case, CamelCase, friendly) +1
static constexpr const int NUM_LINE_FLAGS = X_FOREACH_LINE_FLAG(X_COUNT);
#undef X_COUNT
DEFINE_ENUM_COUNT(LineFlagEnum, NUM_LINE_FLAGS)

class LineFlags final : public enums::Flags<LineFlags, LineFlagEnum, uint16_t>
{
public:
    using Flags::Flags;

public:
#define X_DEFINE_ACCESSORS(UPPER_CASE, lower_case, CamelCase, friendly) \
    bool is##CamelCase() const { return contains(LineFlagEnum::UPPER_CASE); }
    X_FOREACH_LINE_FLAG(X_DEFINE_ACCESSORS)
#undef X_DEFINE_ACCESSORS
};

constexpr inline auto toUint(const LineFlagEnum val) noexcept
{
    using U = uint32_t;
    static_assert(std::is_same_v<U, std::underlying_type_t<LineFlagEnum>>);
    return static_cast<U>(val);
}

inline constexpr LineFlagEnum operator|(const LineFlagEnum lhs, const LineFlagEnum rhs)
{
    return static_cast<LineFlagEnum>(toUint(lhs) | toUint(rhs));
}

inline constexpr LineFlagEnum operator&(const LineFlagEnum lhs, const LineFlagEnum rhs)
{
    return static_cast<LineFlagEnum>(toUint(lhs) & toUint(rhs));
}
