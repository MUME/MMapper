#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019-2026 The MMapper Authors

#include "macros.h"

#include <cstring>
#include <string_view>
#include <type_traits>

template<typename T>
MAYBE_UNUSED NODISCARD static auto numeric_hash(const T val) noexcept
    -> std::enable_if_t<std::is_arithmetic_v<T>, size_t>
{
    static constexpr const size_t size = sizeof(val);
    char buf[size];
    std::memcpy(buf, &val, size);
    return std::hash<std::string_view>()({buf, size});
}

template<typename T>
void hash_combine(std::size_t &seed, const T &value) noexcept
{
    constexpr const size_t GOLDEN_RATIO = 0x9e3779b97f4a7c15;
    seed ^= std::hash<T>{}(value) + GOLDEN_RATIO + (seed << 6) + (seed >> 2);
}
