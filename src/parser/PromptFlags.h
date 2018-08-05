#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
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

#ifndef MMAPPER_PROMPTFLAGS_H
#define MMAPPER_PROMPTFLAGS_H

#include <cstdint>

#include "../global/bits.h"
#include "../global/utils.h"
#include "../mapdata/mmapper2room.h"

// bit0-3 -> char representation of RoomTerrainType
static constexpr const auto TERRAIN_TYPE = (bit1 | bit2 | bit3 | bit4);
static constexpr const auto LIT_ROOM = bit5;
// static constexpr const auto DARK_ROOM = bit6;
static constexpr const auto PROMPT_FLAGS_VALID = bit7;

class PromptFlagsType
{
private:
    uint8_t flags{};

private:
    static uint8_t encodeTerrainType(RoomTerrainType rtt)
    {
        return static_cast<uint8_t>(clamp(static_cast<int>(rtt), 0, 15));
    }

public:
    explicit PromptFlagsType() = default;
    explicit PromptFlagsType(uint8_t flags)
        : flags{flags}
    {
        // REVISIT: turn this into an "unsafe" static function?
    }
    explicit PromptFlagsType(RoomTerrainType rtt)
        : PromptFlagsType{}
    {
        setTerrainType(rtt);
    }

public:
    explicit operator uint8_t() const { return flags; }
    bool operator==(const PromptFlagsType rhs) const { return flags == rhs.flags; }
    bool operator!=(const PromptFlagsType rhs) const { return flags != rhs.flags; }

public:
    bool isValid() const { return flags & static_cast<uint8_t>(PROMPT_FLAGS_VALID); }
    void setValid() { flags |= static_cast<uint8_t>(PROMPT_FLAGS_VALID); }

public:
    auto getTerrainType() const
    {
        return static_cast<RoomTerrainType>(flags & static_cast<uint8_t>(TERRAIN_TYPE));
    }
    void setTerrainType(RoomTerrainType type)
    {
        using flags_type = decltype(flags);
        flags = static_cast<flags_type>(flags & ~TERRAIN_TYPE);
        flags = static_cast<flags_type>(flags | (encodeTerrainType(type) & TERRAIN_TYPE));
    }

public:
    bool isLit() const { return IS_SET(flags, LIT_ROOM); }
    void setLit() { flags |= LIT_ROOM; }

public:
    void reset() { flags = uint8_t{0}; }
};

#endif //MMAPPER_PROMPTFLAGS_H
