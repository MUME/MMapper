#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstdint>

enum class FontFormatFlags { NONE = 0, ITALICS = (1 << 0), UNDERLINE = (1 << 1) };
static inline FontFormatFlags operator&(FontFormatFlags lhs, FontFormatFlags rhs)
{
    return static_cast<FontFormatFlags>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}
