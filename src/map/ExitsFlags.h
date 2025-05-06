#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/EnumIndexedArray.h"
#include "../global/Flags.h"
#include "ExitDirection.h"
#include "ExitFlags.h"

#include <cassert>
#include <cstdint>

class NODISCARD ExitsFlagsType final
    : private EnumIndexedArray<ExitFlags, ExitDirEnum, NUM_EXITS_NESWUD>
{
private:
    using Base = EnumIndexedArray<ExitFlags, ExitDirEnum, NUM_EXITS_NESWUD>;
    bool m_isValid = false;

public:
    using Base::Base;

public:
    NODISCARD explicit operator uint32_t() const = delete;

public:
    NODISCARD bool operator==(const ExitsFlagsType &other) const
    {
        return Base::operator==(other) && isValid() == other.isValid();
    }
    NODISCARD bool operator!=(const ExitsFlagsType &other) const { return !operator==(other); }

public:
    NODISCARD ExitFlags get(const ExitDirEnum dir) const { return Base::at(dir); }
    NODISCARD ExitFlags getWithUnmappedFlag(const ExitDirEnum dir) const
    {
        auto flags = get(dir);
        if (flags.isExit()) {
            flags |= ExitFlagEnum::UNMAPPED;
        }
        return flags;
    }

    void set(const ExitDirEnum dir, const ExitFlagEnum flag) { set(dir, ExitFlags{flag}); }
    void set(const ExitDirEnum dir, const ExitFlags flags) { Base::at(dir) = flags; }

    void insert(const ExitDirEnum dir, const ExitFlagEnum flag) { insert(dir, ExitFlags{flag}); }
    void insert(const ExitDirEnum dir, const ExitFlags flags) { Base::at(dir) |= flags; }

    void remove(const ExitDirEnum dir, const ExitFlagEnum flag) { remove(dir, ExitFlags{flag}); }
    void remove(const ExitDirEnum dir, const ExitFlags flags) { Base::at(dir) &= ~flags; }

public:
    NODISCARD bool isValid() const { return m_isValid; }
    void setValid() { m_isValid = true; }
    void removeValid() { m_isValid = false; }

public:
    void reset() { *this = {}; }
};
