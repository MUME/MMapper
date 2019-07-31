#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstdint>

#include "../global/Flags.h"

enum class FontFormatFlagEnum {
    NONE,
    ITALICS,
    UNDERLINE,
    // NOTE: You must manually update the count if you add any flags.
};

DEFINE_ENUM_COUNT(FontFormatFlagEnum, 3);

struct FontFormatFlags final : public enums::Flags<FontFormatFlags, FontFormatFlagEnum, uint8_t>
{
public:
    using Flags::Flags;
};
