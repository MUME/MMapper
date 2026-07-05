#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "RuleOf5.h"
#include "macros.h"

#include <array>
#include <cstddef>

namespace MMapper {

constexpr struct NODISCARD Uninitialized
{
} uninitialized;

/// Like std::array, except it has a sane default initializer.
template<typename T, size_t N>
class NODISCARD Array : public std::array<T, N>
{
public:
    /// Uses std::array's implicitly-defined default constructor
    /// that uses the rules of aggregate initialization.
    explicit constexpr Array(Uninitialized) {}

public:
    /// calls std::array's initializing default constructor
    constexpr Array()
        : std::array<T, N>{}
    {}

public:
    DEFAULT_CTORS_AND_ASSIGN_OPS(Array);
    ~Array() = default;

public:
    template<typename First, typename... Types>
        requires(std::is_same_v<First, Types> && ...)
    explicit constexpr Array(First &&first, Types &&...args)
        : std::array<T, N>{std::forward<First>(first), std::forward<Types>(args)...}
    {
        static_assert(N == 1 + sizeof...(Types), "missing initializers");
    }
};

template<typename T, typename... U>
    requires(std::is_same_v<T, U> && ...)
Array(T, U...) -> Array<T, 1 + sizeof...(U)>;
} // namespace MMapper

namespace concepts {
namespace detail {
namespace cpp11 {
template<typename T>
struct IsMmapperArray : std::false_type
{};

template<typename T, size_t N>
struct IsMmapperArray<MMapper::Array<T, N>> : std::true_type
{};

} // namespace cpp11
namespace cpp17 {
template<typename T>
inline constexpr bool IsMmapperArray_v = cpp11::IsMmapperArray<T>::value;
}
} // namespace detail
template<typename T>
concept IsMmapperArray = detail::cpp17::IsMmapperArray_v<T>;
} // namespace concepts
