#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstdint>

#include "../global/DirectionType.h"
#include "../global/bits.h"
#include "../global/utils.h"

enum class DirectionalLightEnum { NONE = 0, DIRECT_SUN_ROOM = 1, INDIRECT_SUN_ROOM = 2, BOTH = 3 };

inline DirectionalLightEnum operator&(DirectionalLightEnum lhs, DirectionalLightEnum rhs)
{
    return static_cast<DirectionalLightEnum>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
inline DirectionalLightEnum operator|(DirectionalLightEnum lhs, DirectionalLightEnum rhs)
{
    return static_cast<DirectionalLightEnum>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

static constexpr const auto ANY_DIRECT_SUNLIGHT = (bit1 | bit3 | bit5 | bit7 | bit9 | bit11);
static constexpr const auto CONNECTED_ROOM_FLAGS_VALID = bit15;

class ConnectedRoomFlagsType
{
private:
    uint16_t flags = 0;

public:
    ConnectedRoomFlagsType() = default;
    explicit ConnectedRoomFlagsType(uint16_t flags)
        : flags{flags}
    {
        // REVISIT: turn this into an "unsafe" static function?
    }

public:
    explicit operator uint16_t() const { return flags; }
    bool operator==(const ConnectedRoomFlagsType rhs) const { return flags == rhs.flags; }
    bool operator!=(const ConnectedRoomFlagsType rhs) const { return flags != rhs.flags; }

public:
    bool isValid() const { return IS_SET(flags, CONNECTED_ROOM_FLAGS_VALID); }
    void setValid() { flags |= CONNECTED_ROOM_FLAGS_VALID; }

public:
    void setAnyDirectSunlight() { flags |= ANY_DIRECT_SUNLIGHT; }

    bool hasAnyDirectSunlight() const { return IS_SET(flags, ANY_DIRECT_SUNLIGHT); }

public:
    void reset() { flags = uint16_t{0}; }

private:
    static constexpr const auto MASK = static_cast<uint8_t>(
        static_cast<uint8_t>(DirectionalLightEnum::DIRECT_SUN_ROOM)
        | static_cast<uint8_t>(DirectionalLightEnum::INDIRECT_SUN_ROOM));

    static int getShift(DirectionEnum dir) { return static_cast<int>(dir) * 2; }

public:
    void setDirectionalLight(DirectionEnum dir, DirectionalLightEnum light)
    {
        const auto shift = getShift(dir);
        using flag_type = decltype(flags);
        flags = static_cast<flag_type>(flags & ~(MASK << shift));
        flags = static_cast<flag_type>(flags | ((static_cast<uint8_t>(light) & MASK) << shift));
    }

    DirectionalLightEnum getDirectionalLight(DirectionEnum dir) const
    {
        const auto shift = getShift(dir);
        return static_cast<DirectionalLightEnum>((flags >> shift) & MASK);
    }

    bool hasDirectionalSunlight(DirectionEnum dir) const
    {
        return getDirectionalLight(dir) == DirectionalLightEnum::DIRECT_SUN_ROOM;
    }
};
