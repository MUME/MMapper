#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "macros.h"

// refer to Badge.cpp for an example
template<typename T>
class NODISCARD Badge final
{
    // Only members of T can call the private Badge<T>() constructor.
    friend T;
    // Caution: Don't change this to '= default'; this must be a non-default constructor,
    explicit Badge() {}
};

namespace example {
extern void badge_example();
} // namespace example
