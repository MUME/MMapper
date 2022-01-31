#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)

#include <cstddef> // size_t
#include <cstdint> // int64_t, uint64_t
#include <string_view>
#include <type_traits>
#include <QStringView>

// Convert a QStringView to std::u16string_view.
// This function does not allocate, it simply creates an u16string_view pointing to the same data
// as the QStringView.
//
// For this reason, caller must take care that pointed data outlives the u16string_view
// (as it also happens when using a QStringView).
inline std::u16string_view as_u16string_view(const QStringView str) noexcept
{
    static_assert(sizeof(QChar) == sizeof(char16_t),
                  "QChar and char16_t must have the same sizeof()");
    return std::u16string_view{reinterpret_cast<const char16_t *>(str.data()),
                               static_cast<size_t>(str.size())};
}

// convert a UTF-16 string_view to integer number. String view must contain only decimal digits
// or (for signed numbers) start with the minus character '-'
template<typename T>
inline T to_integer(std::u16string_view str, bool &ok)
{
    static_assert(std::is_integral<T>::value, "to_integer() template type T must be integral");

    using MAXT = std::conditional_t<std::is_unsigned<T>::value, uint64_t, int64_t>;
    const MAXT maxval = to_integer<MAXT>(str, ok);
    const T val = static_cast<T>(maxval);
    ok = ok && (maxval == static_cast<MAXT>(val));
    return val;
}

template<>
int64_t to_integer<int64_t>(std::u16string_view str, bool &ok);

template<>
uint64_t to_integer<uint64_t>(std::u16string_view str, bool &ok);

namespace std {
/// \return true if UTF-16 and Latin1 string_views have the same contents, without allocating
bool operator==(const std::u16string_view left, const std::string_view right) noexcept;

/// \return true if Latin1 and UTF-16 string_views have the same contents, without allocating
inline bool operator==(const std::string_view left, const std::u16string_view right) noexcept
{
    return right == left;
}

/// \return true if UTF-16 string_view and Latin1 string literal have the same contents, without allocating
template<size_t N>
bool operator==(const std::u16string_view left, const char (&right)[N]) noexcept
{
    return left == std::string_view{right, N - 1};
}

} // namespace std
