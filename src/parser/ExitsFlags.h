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

#ifndef MMAPPER_EXITSFLAGS_H
#define MMAPPER_EXITSFLAGS_H

#include <cassert>
#include <cstdint>
#include <stdexcept>

#include "../global/Flags.h"
#include "../global/bits.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/ExitFlags.h"

enum class ExitFlagExt : uint32_t { EXITS_FLAGS_VALID = bit31 };
// 2nd declaration to avoid having to type "ExitFlagExt::" to use this
static constexpr const ExitFlagExt EXITS_FLAGS_VALID = ExitFlagExt::EXITS_FLAGS_VALID;

/* FIXME: This name creates a lot of confusion with ExitFlags.
 * Maybe just make it an array of ExitFlags? */
class ExitsFlagsType
{
public:
    static constexpr const uint32_t MASK
        = (ExitFlag::EXIT | ExitFlag::DOOR | ExitFlag::ROAD | ExitFlag::CLIMB).asUint32();
    static_assert(MASK == 0b1111, "");
    static constexpr const int SHIFT = 4;
    static constexpr const int NUM_DIRS = 6;

private:
    uint32_t value = 0u;

    static int getShift(ExitDirection dir)
    {
        assert(static_cast<int>(dir) < NUM_DIRS);
        return static_cast<int>(dir) * SHIFT;
    }

public:
    explicit ExitsFlagsType() {}
    explicit operator uint32_t() const { return value; }
    static ExitsFlagsType create_unsafe(const uint32_t value)
    {
        if (false) {
            ExitsFlagsType tmp{};
            tmp.value = value;

            ExitsFlagsType result{};
            for (const auto dir : ALL_EXITS_NESWUD)
                result.set(dir, tmp.get(dir));

            if (tmp.isValid())
                result.setValid();

            assert(tmp.value == result.value);
            return result;
        } else {
            static constexpr const uint32_t FULL_MASK = 0x40FFFFFFu;
            static_assert(FULL_MASK
                              == (static_cast<uint32_t>(EXITS_FLAGS_VALID)
                                  | ((1u << (SHIFT * NUM_DIRS)) - 1u)),
                          "");

            ExitsFlagsType result{};
            result.value = value & FULL_MASK;
            // NOTE: can't assert this, because old versions will have invalid data;
            // the whole point of the mask it to clean up the bad data.
            // assert(result.value == value);
            return result;
        }
    }

public:
    bool operator==(ExitsFlagsType rhs) const { return value == rhs.value; }
    bool operator!=(ExitsFlagsType rhs) const { return value != rhs.value; }

public:
    ExitFlags get(const ExitDirection dir) const
    {
        return static_cast<ExitFlags>((value >> getShift(dir)) & MASK);
    }

    void set(const ExitDirection dir, const ExitFlag flag) { set(dir, ExitFlags{flag}); }
    void set(const ExitDirection dir, const ExitFlags flags)
    {
        // can't assert here, because callers expect to pass without masking
        //
        // assert(flags == (flags & ExitFlags{MASK}));
        // assert((flags.asUint32() & ~MASK) == 0u);
        const auto shift = getShift(dir);
        value &= ~(MASK << shift);
        value |= ((flags).asUint32() & MASK) << shift;
    }

public:
    bool isValid() const { return (value & static_cast<uint32_t>(EXITS_FLAGS_VALID)) != 0u; }
    void setValid() { value |= static_cast<uint32_t>(EXITS_FLAGS_VALID); }
    void removeValid() { value &= ~static_cast<uint32_t>(EXITS_FLAGS_VALID); }

public:
    [[deprecated]] ExitsFlagsType &operator|=(ExitFlagExt flag)
    {
        if (flag != EXITS_FLAGS_VALID)
            throw std::invalid_argument("flag");
        setValid();
        return *this;
    }

public:
    void reset() { value = 0u; }
};

#endif // MMAPPER_EXITSFLAGS_H
