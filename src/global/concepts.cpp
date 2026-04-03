// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "concepts.h"

#include <cstdint>

namespace concepts {
static_assert(!IsNumeric<bool>);
static_assert(IsBoolean<bool>);

static_assert(!IsNumeric<char>);
static_assert(IsCharacter<char>);
static_assert(IsCharacter<char32_t>);

static_assert(IsNumeric<signed char>);
static_assert(IsNumeric<int>);
static_assert(IsNumeric<uint32_t>);
} // namespace concepts
