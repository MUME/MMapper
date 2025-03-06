#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/Flags.h"
#include "DoorFlags.h"
#include "ExitDirection.h"

#include <cassert>
#include <cstdint>

enum class NODISCARD DoorFlagExtEnum : uint32_t { DOORS_FLAGS_VALID = (1u << 30) };
// 2nd declaration to avoid having to type "DoorFlagExt::" to use this
static constexpr const DoorFlagExtEnum DOORS_FLAGS_VALID = DoorFlagExtEnum::DOORS_FLAGS_VALID;

class NODISCARD DoorsFlagsType final
{
public:
    static constexpr const uint32_t MASK = DoorFlags{DoorFlagEnum::HIDDEN}.asUint32();
    static_assert(MASK == 0b1);
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
    DoorsFlagsType() = default;
    NODISCARD explicit operator uint32_t() const { return value; }
    NODISCARD static DoorsFlagsType create_unsafe(const uint32_t value)
    {
        if (false) {
            DoorsFlagsType tmp;
            tmp.value = value;

            DoorsFlagsType result;
            for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
                result.set(dir, tmp.get(dir));
            }

            if (tmp.isValid()) {
                result.setValid();
            }

            assert(tmp.value == result.value);
            return result;
        } else {
            static constexpr const uint32_t FULL_MASK = 0x40FFFFFFu;
            static_assert(
                FULL_MASK
                == (static_cast<uint32_t>(DOORS_FLAGS_VALID) | ((1u << (SHIFT * NUM_DIRS)) - 1u)));

            DoorsFlagsType result;
            result.value = value & FULL_MASK;
            // NOTE: can't assert this, because old versions will have invalid data;
            // the whole point of the mask it to clean up the bad data.
            // assert(result.value == value);
            return result;
        }
    }

public:
    NODISCARD bool operator==(DoorsFlagsType rhs) const { return value == rhs.value; }
    NODISCARD bool operator!=(DoorsFlagsType rhs) const { return value != rhs.value; }

public:
    NODISCARD DoorFlags get(const ExitDirEnum dir) const
    {
        return static_cast<DoorFlags>((value >> getShift(dir)) & MASK);
    }

    void set(const ExitDirEnum dir, const DoorFlagEnum flag) { set(dir, DoorFlags{flag}); }
    void set(const ExitDirEnum dir, const DoorFlags flags)
    {
        // can't assert here, because callers expect to pass without masking
        //
        // assert(flags == (flags & DoorFlags{MASK}));
        // assert((flags.asUint32() & ~MASK) == 0u);
        const auto shift = getShift(dir);
        value &= ~(MASK << shift);
        value |= ((flags).asUint32() & MASK) << shift;
    }

    void insert(const ExitDirEnum dir, const DoorFlagEnum flag)
    {
        const auto shift = getShift(dir);
        value |= (DoorFlags{flag}.asUint32() & MASK) << shift;
    }

public:
    NODISCARD bool isValid() const
    {
        return (value & static_cast<uint32_t>(DOORS_FLAGS_VALID)) != 0u;
    }
    void setValid() { value |= static_cast<uint32_t>(DOORS_FLAGS_VALID); }
    void removeValid() { value &= ~static_cast<uint32_t>(DOORS_FLAGS_VALID); }

public:
    void reset() { value = 0u; }
};
