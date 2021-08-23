#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstdint>

#include "../global/Flags.h"

enum class NODISCARD FontFormatFlagEnum {
    NONE,
    ITALICS,
    UNDERLINE,
    /// Horizontal centering; mutually exclusive with HALIGN_RIGHT.
    /// Note: HALIGN_CENTER takes precedence over HALIGN_RIGHT if both are set.
    HALIGN_CENTER,
    /// Horizontal right-alignemnt; mutually exclusive with HALIGN_CENTER.
    HALIGN_RIGHT,
    //
    // NOTE: You must manually update the count if you add any flags.
    //
};

DEFINE_ENUM_COUNT(FontFormatFlagEnum, 5);

struct NODISCARD FontFormatFlags final
    : public enums::Flags<FontFormatFlags, FontFormatFlagEnum, uint8_t>
{
public:
    using Flags::Flags;
};
