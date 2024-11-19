#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)

#include "Charset.h"
#include "macros.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>

#include <QStringView>

namespace mmqt {
// Convert a QStringView to std::u16string_view.
// This function does not allocate, it simply creates an u16string_view pointing to the same data
// as the QStringView.
//
// For this reason, caller must take care that pointed data outlives the u16string_view
// (as it also happens when using a QStringView).
NODISCARD inline std::u16string_view as_u16string_view(const QStringView str) noexcept
{
    static_assert(sizeof(QChar) == sizeof(char16_t),
                  "QChar and char16_t must have the same sizeof()");
    static_assert(alignof(QChar) == alignof(char16_t),
                  "QChar and char16_t must have the same alignof()");
    return std::u16string_view{reinterpret_cast<const char16_t *>(str.data()),
                               static_cast<size_t>(str.size())};
}
} // namespace mmqt

// convert a UTF-16 string_view to integer number. String view must contain only decimal digits
// or (for signed numbers) start with the minus character '-'
template<typename T>
NODISCARD inline std::optional<T> to_integer(std::u16string_view str)
{
    static_assert(std::is_integral_v<T>, "to_integer() template type T must be integral");

    using MAXT = std::conditional_t<std::is_unsigned_v<T>, uint64_t, int64_t>;
    const std::optional<MAXT> maxval = to_integer<MAXT>(str);
    if (!maxval) {
        return std::nullopt;
    }
    const T val = static_cast<T>(maxval.value());
    if (maxval != static_cast<MAXT>(val)) {
        return std::nullopt;
    }
    return val;
}

template<>
NODISCARD std::optional<int64_t> to_integer<int64_t>(std::u16string_view str);

template<>
NODISCARD std::optional<uint64_t> to_integer<uint64_t>(std::u16string_view str);
