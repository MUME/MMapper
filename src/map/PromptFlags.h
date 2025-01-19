#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/utils.h"
#include "mmapper2room.h"

#include <cstdint>

#define X_FOREACH_PROMPT_WEATHER_ENUM(X) \
    X(UNDEFINED) \
    X(CLOUDS) \
    X(RAIN) \
    X(HEAVY_RAIN) \
    X(SNOW)
#define X_FOREACH_PROMPT_FOG_ENUM(X) \
    X(UNDEFINED) \
    X(LIGHT_FOG) \
    X(HEAVY_FOG)

#define DECL(X) X,
enum class NODISCARD PromptWeatherEnum { X_FOREACH_PROMPT_WEATHER_ENUM(DECL) };
enum class NODISCARD PromptFogEnum { X_FOREACH_PROMPT_FOG_ENUM(DECL) };
#undef DECL

#define ADD(X) +1
static constexpr const size_t NUM_PROMPT_WEATHER_TYPES = (X_FOREACH_PROMPT_WEATHER_ENUM(ADD));
static constexpr const size_t NUM_PROMPT_FOG_TYPES = (X_FOREACH_PROMPT_FOG_ENUM(ADD));
#undef ADD

static_assert(NUM_PROMPT_WEATHER_TYPES == 5);
static_assert(NUM_PROMPT_FOG_TYPES == 3);

class NODISCARD PromptFlagsType final
{
public:
    static constexpr const auto LIT_ROOM = 1u;
    static constexpr const auto DARK_ROOM = 1u << 1;
    static constexpr const auto LIGHT_MASK = LIT_ROOM | DARK_ROOM;
    static constexpr const auto PROMPT_FLAGS_VALID = 1u << 2;
    // bit3-4 -> PromptFogEnum
    static constexpr const auto FOG_TYPE = 0b11u << 3;
    // bit5-9 -> PromptWeatherEnum
    static constexpr const auto WEATHER_TYPE = 0b111u << 5;

private:
    uint32_t flags = 0u;

private:
    NODISCARD static uint32_t encodeFogType(const PromptFogEnum pf)
    {
        return std::clamp<uint32_t>(static_cast<uint32_t>(pf), 0, 2);
    }
    NODISCARD static uint32_t encodeWeatherType(const PromptWeatherEnum pw)
    {
        return std::clamp<uint32_t>(static_cast<uint32_t>(pw), 0, 4);
    }

public:
    PromptFlagsType() = default;

public:
    NODISCARD explicit operator uint32_t() const { return flags; }
    NODISCARD bool operator==(const PromptFlagsType rhs) const { return flags == rhs.flags; }
    NODISCARD bool operator!=(const PromptFlagsType rhs) const { return flags != rhs.flags; }

public:
    NODISCARD bool isValid() const { return flags & PROMPT_FLAGS_VALID; }
    void setValid() { flags |= PROMPT_FLAGS_VALID; }

public:
    NODISCARD PromptFogEnum getFogType() const
    {
        return static_cast<PromptFogEnum>(flags & static_cast<uint32_t>(FOG_TYPE));
    }
    void setFogType(const PromptFogEnum type)
    {
        using flags_type = decltype(flags);
        flags = static_cast<flags_type>(flags & ~FOG_TYPE);
        flags = static_cast<flags_type>(flags | (encodeFogType(type) & FOG_TYPE));
    }

public:
    NODISCARD PromptWeatherEnum getWeatherType() const
    {
        return static_cast<PromptWeatherEnum>(flags & WEATHER_TYPE);
    }
    void setWeatherType(const PromptWeatherEnum type)
    {
        using flags_type = decltype(flags);
        flags = static_cast<flags_type>(flags & ~WEATHER_TYPE);
        flags = static_cast<flags_type>(flags | (encodeWeatherType(type) & WEATHER_TYPE));
    }

public:
    NODISCARD bool isNiceWeather() const
    {
        return getWeatherType() == PromptWeatherEnum::UNDEFINED
               && getFogType() == PromptFogEnum::UNDEFINED;
    }

public:
    NODISCARD bool isLit() const { return (flags & LIT_ROOM) != 0; }
    void setLit()
    {
        using flags_type = decltype(flags);
        flags = static_cast<flags_type>(flags & ~LIGHT_MASK);
        flags = static_cast<flags_type>(flags | (LIT_ROOM & LIGHT_MASK));
    }
    NODISCARD bool isDark() const { return (flags & DARK_ROOM) != 0; }
    void setDark()
    {
        using flags_type = decltype(flags);
        flags = static_cast<flags_type>(flags & ~LIGHT_MASK);
        flags = static_cast<flags_type>(flags | (DARK_ROOM & LIGHT_MASK));
    }

public:
    void reset() { flags = uint32_t{0}; }
};

template<>
struct std::hash<PromptFlagsType>
{
    NODISCARD std::size_t operator()(const PromptFlagsType x) const noexcept
    {
        return std::hash<uint32_t>()(static_cast<uint32_t>(x));
    }
};

NODISCARD extern std::string_view to_string_view(PromptFogEnum);
NODISCARD extern std::string_view to_string_view(PromptWeatherEnum);
