#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "macros.h"

#include <source_location>

namespace mm {
using source_location = std::source_location;
} // namespace mm

#define MM_SOURCE_LOCATION() (std::source_location::current())
