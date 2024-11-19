#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "macros.h"

#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

template<typename To, typename From>
NODISCARD static inline To checked_cast(const From from)
{
    static_assert(std::is_integral_v<From>);
    static_assert(std::is_integral_v<To>);
    static_assert(!std::is_same_v<From, bool>);
    static_assert(!std::is_same_v<To, bool>);

    if constexpr (std::is_signed_v<From>) {
        if constexpr (std::is_unsigned_v<To> || sizeof(To) < sizeof(From)) {
            const auto min = static_cast<From>(std::numeric_limits<To>::min());
            if (from < min) {
                throw std::underflow_error(std::to_string(from) + " is less than "
                                           + std::to_string(min));
            }
        }
    }
    if constexpr (sizeof(To) < sizeof(From)
                  || (sizeof(To) == sizeof(From)
                      && std::is_unsigned_v<From> && std::is_signed_v<To>) ) {
        const auto max = static_cast<From>(std::numeric_limits<To>::max());
        if (from > max) {
            throw std::overflow_error(std::to_string(from) + " is greater than "
                                      + std::to_string(max));
        }
    }
    return static_cast<To>(from);
}
