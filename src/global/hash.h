#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019-2026 The MMapper Authors

#include "macros.h"

#include <cstdint>
#include <cstring>
#include <string_view>

MAYBE_UNUSED NODISCARD static size_t numeric_hash(const std::integral auto val) noexcept
{
    static constexpr size_t size = sizeof(val);
    char buf[size];
    std::memcpy(buf, &val, size);
    return std::hash<std::string_view>()({buf, size});
}

#if INTPTR_MAX == INT64_MAX
static constexpr size_t GOLDEN_RATIO = 0x9e3779b97f4a7c15ULL;
#else
static constexpr size_t GOLDEN_RATIO = 0x9e3779b9U;
#endif

template<typename T>
void hash_combine(std::size_t &seed, const T &value) noexcept
{
    seed ^= std::hash<T>{}(value) + GOLDEN_RATIO + (seed << 6) + (seed >> 2);
}
