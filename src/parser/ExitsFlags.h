#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cassert>
#include <cstdint>
#include <stdexcept>

#include "../global/Flags.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/ExitFlags.h"

enum class NODISCARD ExitFlagExtEnum : uint32_t { EXITS_FLAGS_VALID = (1u << 30) };
// 2nd declaration to avoid having to type "ExitFlagExt::" to use this
static constexpr const ExitFlagExtEnum EXITS_FLAGS_VALID = ExitFlagExtEnum::EXITS_FLAGS_VALID;

/* FIXME: This name creates a lot of confusion with ExitFlags.
 * Maybe just make it an array of ExitFlags? */
class NODISCARD ExitsFlagsType final
{
public:
    static constexpr const uint32_t MASK = (ExitFlagEnum::EXIT | ExitFlagEnum::DOOR
                                            | ExitFlagEnum::ROAD | ExitFlagEnum::CLIMB)
                                               .asUint32();
    static_assert(MASK == 0b1111);
    static constexpr const int SHIFT = 4;
    static constexpr const int NUM_DIRS = 6;

private:
    uint32_t value = 0u;

    NODISCARD static int getShift(const ExitDirEnum dir)
    {
        assert(static_cast<int>(dir) < NUM_DIRS);
        return static_cast<int>(dir) * SHIFT;
    }

public:
    ExitsFlagsType() = default;
    explicit operator uint32_t() const { return value; }
    NODISCARD static ExitsFlagsType create_unsafe(const uint32_t value)
    {
        if (false) {
            ExitsFlagsType tmp;
            tmp.value = value;

            ExitsFlagsType result;
            for (const ExitDirEnum dir : ALL_EXITS_NESWUD)
                result.set(dir, tmp.get(dir));

            if (tmp.isValid())
                result.setValid();

            assert(tmp.value == result.value);
            return result;
        } else {
            static constexpr const uint32_t FULL_MASK = 0x40FFFFFFu;
            static_assert(
                FULL_MASK
                == (static_cast<uint32_t>(EXITS_FLAGS_VALID) | ((1u << (SHIFT * NUM_DIRS)) - 1u)));

            ExitsFlagsType result;
            result.value = value & FULL_MASK;
            // NOTE: can't assert this, because old versions will have invalid data;
            // the whole point of the mask it to clean up the bad data.
            // assert(result.value == value);
            return result;
        }
    }

public:
    NODISCARD bool operator==(ExitsFlagsType rhs) const { return value == rhs.value; }
    NODISCARD bool operator!=(ExitsFlagsType rhs) const { return value != rhs.value; }

public:
    NODISCARD ExitFlags get(const ExitDirEnum dir) const
    {
        return static_cast<ExitFlags>((value >> getShift(dir)) & MASK);
    }

    void set(const ExitDirEnum dir, const ExitFlagEnum flag) { set(dir, ExitFlags{flag}); }
    void set(const ExitDirEnum dir, const ExitFlags flags)
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
    NODISCARD bool isValid() const
    {
        return (value & static_cast<uint32_t>(EXITS_FLAGS_VALID)) != 0u;
    }
    void setValid() { value |= static_cast<uint32_t>(EXITS_FLAGS_VALID); }
    void removeValid() { value &= ~static_cast<uint32_t>(EXITS_FLAGS_VALID); }

public:
    void reset() { value = 0u; }
};
