#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <cstring>
#include <string_view>
#include <type_traits>

#include "macros.h"

template<typename T>
NODISCARD static auto numeric_hash(const T val) noexcept
    -> std::enable_if_t<std::is_arithmetic_v<T>, size_t>
{
    static constexpr const size_t size = sizeof(val);
    char buf[size];
    std::memcpy(buf, &val, size);
    return std::hash<std::string_view>()({buf, size});
}
