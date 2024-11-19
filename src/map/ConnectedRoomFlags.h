#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/utils.h"
#include "ExitDirection.h"

#include <cstdint>

enum class NODISCARD DirectSunlightEnum : uint32_t {
    UNKNOWN = 0,
    SAW_DIRECT_SUN = 1,
    SAW_NO_DIRECT_SUN = 2,
};

NODISCARD constexpr inline auto toUint(const DirectSunlightEnum val) noexcept
{
    using U = uint32_t;
    static_assert(std::is_same_v<U, std::underlying_type_t<DirectSunlightEnum>>);
    return static_cast<U>(val);
}

NODISCARD inline constexpr DirectSunlightEnum operator&(const DirectSunlightEnum lhs,
                                                        const DirectSunlightEnum rhs)
{
    return static_cast<DirectSunlightEnum>(toUint(lhs) & toUint(rhs));
}

static constexpr const auto SAW_ANY_DIRECT_SUNLIGHT = 0b10101010101u;
static constexpr const auto CONNECTED_ROOM_FLAGS_VALID = 1u << 14;
static constexpr const auto CONNECTED_ROOM_FLAGS_TROLL_MODE = 1u << 15;

// every other bit for all 6 directions.
static_assert(SAW_ANY_DIRECT_SUNLIGHT == ((1u << (2 * 6)) - 1u) / 3u);
static_assert(utils::isPowerOfTwo(CONNECTED_ROOM_FLAGS_VALID));
static_assert(CONNECTED_ROOM_FLAGS_VALID > SAW_ANY_DIRECT_SUNLIGHT);

class NODISCARD ConnectedRoomFlagsType final
{
private:
    uint32_t m_flags = 0;

public:
    ConnectedRoomFlagsType() = default;

public:
    NODISCARD bool operator==(const ConnectedRoomFlagsType rhs) const
    {
        return m_flags == rhs.m_flags;
    }
    NODISCARD bool operator!=(const ConnectedRoomFlagsType rhs) const
    {
        return m_flags != rhs.m_flags;
    }

public:
    NODISCARD bool isValid() const { return (m_flags & CONNECTED_ROOM_FLAGS_VALID) != 0u; }
    void setValid() { m_flags |= CONNECTED_ROOM_FLAGS_VALID; }

public:
    NODISCARD bool hasAnyDirectSunlight() const
    {
        return (m_flags & SAW_ANY_DIRECT_SUNLIGHT) != 0u;
    }

public:
    NODISCARD bool isTrollMode() const { return (m_flags & CONNECTED_ROOM_FLAGS_TROLL_MODE) != 0u; }
    void setTrollMode() { m_flags |= CONNECTED_ROOM_FLAGS_TROLL_MODE; }

public:
    void reset() { *this = ConnectedRoomFlagsType(); }

private:
    static constexpr const auto MASK = static_cast<std::underlying_type_t<DirectSunlightEnum>>(
        toUint(DirectSunlightEnum::SAW_DIRECT_SUN) | toUint(DirectSunlightEnum::SAW_NO_DIRECT_SUN));

    NODISCARD static int getShift(const ExitDirEnum dir) { return static_cast<int>(dir) * 2; }

public:
    NODISCARD DirectSunlightEnum getDirectSunlight(const ExitDirEnum dir) const
    {
        const auto shift = getShift(dir);
        return static_cast<DirectSunlightEnum>((m_flags >> shift) & MASK);
    }

public:
    void setDirectSunlight(const ExitDirEnum dir, const DirectSunlightEnum light)
    {
        const auto shift = getShift(dir);
        m_flags &= ~(MASK << shift);
        m_flags |= (toUint(light) & MASK) << shift;
    }

    NODISCARD bool hasNoDirectSunlight(const ExitDirEnum dir) const
    {
        return getDirectSunlight(dir) == DirectSunlightEnum::SAW_NO_DIRECT_SUN;
    }

    NODISCARD bool hasDirectSunlight(const ExitDirEnum dir) const
    {
        return getDirectSunlight(dir) == DirectSunlightEnum::SAW_DIRECT_SUN;
    }
};
