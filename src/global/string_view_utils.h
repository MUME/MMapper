#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)

#include <string_view>
#include <QString>

// Convert a QStringRef to std::u16string_view.
// This function does not allocate, it simply creates an u16string_view pointing to the same data
// as the QStringRef.
//
// For this reason, caller must take care that pointed data outlives the u16string_view
// (as it also happens when using a QStringRef).
inline std::u16string_view to_u16string_view(const QStringRef ref) noexcept
{
    static_assert(sizeof(QChar) == sizeof(char16_t),
                  "QChar and char16_t must have the same sizeof()");
    return std::u16string_view{reinterpret_cast<const char16_t *>(ref.data()),
                               static_cast<size_t>(ref.size())};
}

namespace std {

/// \return true if UTF-16 and Latin1 string_views have the same contents, without allocating
bool operator==(const std::u16string_view left, const std::string_view right) noexcept;

/// \return true if Latin1 and UTF-16 string_views have the same contents, without allocating
inline bool operator==(const std::string_view left, const std::u16string_view right) noexcept
{
    return right == left;
}

} // namespace std
