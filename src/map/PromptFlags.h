#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/utils.h"
#include "mmapper2room.h"

#include <cstdint>

#define XFOREACH_PROMPT_WEATHER_ENUM(X) \
    X(NICE) \
    X(CLOUDS) \
    X(RAIN) \
    X(HEAVY_RAIN) \
    X(SNOW)
#define XFOREACH_PROMPT_FOG_ENUM(X) \
    X(NO_FOG) \
    X(LIGHT_FOG) \
    X(HEAVY_FOG)

#define X_DECL(X) X,
enum class NODISCARD PromptWeatherEnum { XFOREACH_PROMPT_WEATHER_ENUM(X_DECL) };
enum class NODISCARD PromptFogEnum { XFOREACH_PROMPT_FOG_ENUM(X_DECL) };
#undef X_DECL

#define X_ADD(X) +1
static constexpr const size_t NUM_PROMPT_WEATHER_TYPES = (XFOREACH_PROMPT_WEATHER_ENUM(X_ADD));
static constexpr const size_t NUM_PROMPT_FOG_TYPES = (XFOREACH_PROMPT_FOG_ENUM(X_ADD));
#undef X_ADD

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
    static constexpr uint32_t FOG_SHIFT = 3u;
    static constexpr const auto FOG_TYPE = 0b11u << FOG_SHIFT;
    // bit5-9 -> PromptWeatherEnum
    static constexpr uint32_t WEATHER_SHIFT = 5u;
    static constexpr const auto WEATHER_TYPE = 0b111u << WEATHER_SHIFT;

private:
    uint32_t m_flags = 0u;

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
    NODISCARD explicit operator uint32_t() const { return m_flags; }
    NODISCARD bool operator==(const PromptFlagsType rhs) const { return m_flags == rhs.m_flags; }
    NODISCARD bool operator!=(const PromptFlagsType rhs) const { return m_flags != rhs.m_flags; }

public:
    NODISCARD bool isValid() const { return m_flags & PROMPT_FLAGS_VALID; }
    void setValid() { m_flags |= PROMPT_FLAGS_VALID; }

public:
    NODISCARD PromptFogEnum getFogType() const
    {
        return static_cast<PromptFogEnum>((m_flags & FOG_TYPE) >> FOG_SHIFT);
    }
    void setFogType(const PromptFogEnum type)
    {
        using flags_type = decltype(m_flags);
        m_flags = static_cast<flags_type>(m_flags & ~FOG_TYPE);
        m_flags = static_cast<flags_type>(m_flags | ((encodeFogType(type) << FOG_SHIFT) & FOG_TYPE));
    }

public:
    NODISCARD PromptWeatherEnum getWeatherType() const
    {
        return static_cast<PromptWeatherEnum>((m_flags & WEATHER_TYPE) >> WEATHER_SHIFT);
    }
    void setWeatherType(const PromptWeatherEnum type)
    {
        using flags_type = decltype(m_flags);
        m_flags = static_cast<flags_type>(m_flags & ~WEATHER_TYPE);
        m_flags = static_cast<flags_type>(
            m_flags | ((encodeWeatherType(type) << WEATHER_SHIFT) & WEATHER_TYPE));
    }

public:
    NODISCARD bool isNiceWeather() const
    {
        return getWeatherType() == PromptWeatherEnum::NICE && getFogType() == PromptFogEnum::NO_FOG;
    }

public:
    NODISCARD bool isLit() const { return (m_flags & LIT_ROOM) != 0; }
    void setLit()
    {
        using flags_type = decltype(m_flags);
        m_flags = static_cast<flags_type>(m_flags & ~LIGHT_MASK);
        m_flags = static_cast<flags_type>(m_flags | (LIT_ROOM & LIGHT_MASK));
    }
    NODISCARD bool isDark() const { return (m_flags & DARK_ROOM) != 0; }
    void setDark()
    {
        using flags_type = decltype(m_flags);
        m_flags = static_cast<flags_type>(m_flags & ~LIGHT_MASK);
        m_flags = static_cast<flags_type>(m_flags | (DARK_ROOM & LIGHT_MASK));
    }
    void setArtificial()
    {
        using flags_type = decltype(m_flags);
        m_flags = static_cast<flags_type>(m_flags & ~LIGHT_MASK);
    }

public:
    void reset() { m_flags = uint32_t{0}; }
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
