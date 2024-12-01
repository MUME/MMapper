#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "ConversionResult.h"
#include "cast_error.h"
#include "macros.h"

#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace int_cast::exact {

template<typename To, typename From>
NODISCARD static constexpr ConversionResult<To> convert(const From from) noexcept
{
    static_assert(std::is_integral_v<From>);
    static_assert(std::is_integral_v<To>);
    static_assert(!std::is_same_v<From, bool>);
    static_assert(!std::is_same_v<To, bool>);

    if constexpr (std::is_signed_v<From>) {
        if constexpr (std::is_unsigned_v<To> || sizeof(To) < sizeof(From)) {
            const auto min = static_cast<From>(std::numeric_limits<To>::min());
            if (from < min) {
                return CastErrorEnum::TooSmall;
            }
        }
    }
    if constexpr (sizeof(To) < sizeof(From)
                  || (sizeof(To) == sizeof(From)
                      && std::is_unsigned_v<From> && std::is_signed_v<To>) ) {
        const auto max = static_cast<From>(std::numeric_limits<To>::max());
        if (from > max) {
            return CastErrorEnum::TooBig;
        }
    }

    return ConversionResult<To>{static_cast<To>(from)};
}

template<typename To, typename From>
NODISCARD constexpr std::optional<To> opt_convert(const From from) noexcept
{
    const auto maybe = convert<To, From>(from);
    if (!maybe.is_valid()) {
        return std::nullopt;
    }
    return maybe.get_value();
}

template<typename To, typename From>
NODISCARD constexpr bool can_cast(const From from) noexcept
{
    return convert<To, From>(from).is_valid();
}

template<typename To, typename From>
NODISCARD constexpr To checked_cast(const From from) CAN_THROW
{
    const auto maybe = convert<To, From>(from);
    if (!maybe.is_valid()) {
        throw CastErrorException{maybe.get_error()};
    }
    return maybe.get_value();
}

} // namespace int_cast::exact

namespace test {
extern void test_int_cast();
} // namespace test
