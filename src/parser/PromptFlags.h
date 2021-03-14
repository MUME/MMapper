#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstdint>

#include "../global/utils.h"
#include "../mapdata/mmapper2room.h"

#define X_FOREACH_PROMT_WEATHER_ENUM(X) \
    X(UNDEFINED) \
    X(CLOUDS) \
    X(RAIN) \
    X(HEAVY_RAIN) \
    X(SNOW)
#define X_FOREACH_PROMT_FOG_ENUM(X) \
    X(UNDEFINED) \
    X(LIGHT_FOG) \
    X(HEAVY_FOG)

#define DECL(X) X,
enum class PromptWeatherEnum { X_FOREACH_PROMT_WEATHER_ENUM(DECL) };
enum class PromptFogEnum { X_FOREACH_PROMT_FOG_ENUM(DECL) };
#undef DECL

#define ADD(X) +1
static constexpr const size_t NUM_PROMPT_WEATHER_TYPES = (X_FOREACH_PROMT_WEATHER_ENUM(ADD));
static constexpr const size_t NUM_PROMPT_FOG_TYPES = (X_FOREACH_PROMT_FOG_ENUM(ADD));
#undef ADD

static_assert(NUM_PROMPT_WEATHER_TYPES == 5);
static_assert(NUM_PROMPT_FOG_TYPES == 3);

class PromptFlagsType final
{
public:
    // bit0-3 -> RoomTerrainEnum
    static constexpr const auto TERRAIN_TYPE = 0b1111u;
    static constexpr const auto LIT_ROOM = 1u << 4;
    static constexpr const auto DARK_ROOM = 1u << 5;
    static constexpr const auto LIGHT_MASK = LIT_ROOM | DARK_ROOM;
    static constexpr const auto PROMPT_FLAGS_VALID = 1u << 6;
    // bit7-8 -> PromptFogEnum
    static constexpr const auto FOG_TYPE = 0b11u << 7;
    // bit9-11 -> PromptWeatherEnum
    static constexpr const auto WEATHER_TYPE = 0b111u << 9;

private:
    uint32_t flags = 0u;

private:
    static uint32_t encodeFogType(const PromptFogEnum pf)
    {
        return std::clamp<uint32_t>(static_cast<uint32_t>(pf), 0, 2);
    }
    static uint32_t encodeTerrainType(const RoomTerrainEnum rtt)
    {
        return std::clamp<uint32_t>(static_cast<uint32_t>(rtt), 0, 15);
    }
    static uint32_t encodeWeatherType(const PromptWeatherEnum pw)
    {
        return std::clamp<uint32_t>(static_cast<uint32_t>(pw), 0, 4);
    }

public:
    PromptFlagsType() = default;

public:
    /// NOTE: This sets the valid flag on the result.
    static PromptFlagsType fromRoomTerrainType(const RoomTerrainEnum rtt)
    {
        PromptFlagsType result;
        result.setTerrainType(rtt);
        result.setValid();
        return result;
    }

public:
    explicit operator uint32_t() const { return flags; }
    bool operator==(const PromptFlagsType rhs) const { return flags == rhs.flags; }
    bool operator!=(const PromptFlagsType rhs) const { return flags != rhs.flags; }

public:
    bool isValid() const { return flags & PROMPT_FLAGS_VALID; }
    void setValid() { flags |= PROMPT_FLAGS_VALID; }

public:
    auto getTerrainType() const { return static_cast<RoomTerrainEnum>(flags & TERRAIN_TYPE); }
    void setTerrainType(const RoomTerrainEnum type)
    {
        using flags_type = decltype(flags);
        flags = static_cast<flags_type>(flags & ~TERRAIN_TYPE);
        flags = static_cast<flags_type>(flags | (encodeTerrainType(type) & TERRAIN_TYPE));
    }

public:
    auto getFogType() const
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
    auto getWeatherType() const { return static_cast<PromptWeatherEnum>(flags & WEATHER_TYPE); }
    void setWeatherType(const PromptWeatherEnum type)
    {
        using flags_type = decltype(flags);
        flags = static_cast<flags_type>(flags & ~WEATHER_TYPE);
        flags = static_cast<flags_type>(flags | (encodeWeatherType(type) & WEATHER_TYPE));
    }

public:
    bool isNiceWeather() const
    {
        return getWeatherType() == PromptWeatherEnum::UNDEFINED
               && getFogType() == PromptFogEnum::UNDEFINED;
    }

public:
    bool isLit() const { return (flags & LIT_ROOM) != 0; }
    void setLit()
    {
        using flags_type = decltype(flags);
        flags = static_cast<flags_type>(flags & ~LIGHT_MASK);
        flags = static_cast<flags_type>(flags | (LIT_ROOM & LIGHT_MASK));
    }
    bool isDark() const { return (flags & DARK_ROOM) != 0; }
    void setDark()
    {
        using flags_type = decltype(flags);
        flags = static_cast<flags_type>(flags & ~LIGHT_MASK);
        flags = static_cast<flags_type>(flags | (DARK_ROOM & LIGHT_MASK));
    }

public:
    void reset() { flags = uint32_t{0}; }
};
