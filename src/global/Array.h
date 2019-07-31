#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <array>

#include "RuleOf5.h"

namespace MMapper {

constexpr struct Uninitialized
{
} uninitialized;

/// Like std::array, except it has a sane default initializer.
template<typename T, size_t N>
class Array : public std::array<T, N>
{
public:
    /// Uses std::array's implicitly-defined default constructor
    /// that uses the rules of aggregate initialization.
    explicit Array(Uninitialized) {}

public:
    /// calls std::array's initializing default constructor
    Array()
        : std::array<T, N>{}
    {}

public:
    DEFAULT_CTORS_AND_ASSIGN_OPS(Array);
    ~Array() = default;

public:
    template<typename First, typename... Types>
    constexpr Array(First &&first, Types &&... args)
        : std::array<T, N>{std::forward<First>(first), std::forward<Types>(args)...}
    {}
};

#if __cpp_deduction_guides >= 201606
// deducation guide copied from std::array
template<typename _Tp, typename... _Up>
Array(_Tp, _Up...)
    ->Array<std::enable_if_t<(std::is_same_v<_Tp, _Up> && ...), _Tp>, 1 + sizeof...(_Up)>;
#endif
} // namespace MMapper
