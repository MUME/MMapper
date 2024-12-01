#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "cast_error.h"
#include "macros.h"

#include <type_traits>

template<typename T>
class NODISCARD ConversionResult final
{
private:
    T m_value{};
    CastErrorEnum m_result = CastErrorEnum::Success;

    // This is just to avoid a padding warning
    static inline constexpr auto PAD_BYTES = (alignof(T) > sizeof(CastErrorEnum))
                                                 ? (alignof(T) - sizeof(CastErrorEnum))
                                                 : alignof(T);
    MAYBE_UNUSED char m_unused_pad[PAD_BYTES]{};

public:
    constexpr explicit ConversionResult(const T n) noexcept
        : m_value{n}
    {}
    // NOLINTNEXTLINE (purposely implicit)
    constexpr IMPLICIT ConversionResult(const CastErrorEnum err) noexcept
        : m_result{err}
    {}
    NODISCARD constexpr bool is_valid() const noexcept
    {
        return m_result == CastErrorEnum::Success;
    }
    NODISCARD constexpr T get_value() const
    {
        if (!is_valid()) {
            throw std::runtime_error("not valid");
        }
        return m_value;
    }
    NODISCARD constexpr CastErrorEnum get_error() const
    {
        if (is_valid()) {
            throw std::runtime_error("not an error");
        }
        return m_result;
    }
    constexpr explicit operator bool() const noexcept { return is_valid(); }
    NODISCARD constexpr bool operator==(const CastErrorEnum err) const
    {
        return !is_valid() && m_result == err;
    }
    template<typename U>
    NODISCARD constexpr bool operator==(const U intValue) const
    {
        static_assert((std::is_integral_v<
                           U> && std::is_integral_v<T> && std::is_signed_v<U> == std::is_signed_v<T>)
                      || (std::is_floating_point_v<U> && std::is_floating_point_v<T>) );
        return is_valid() && m_value == intValue;
    }
};
