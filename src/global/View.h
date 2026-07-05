#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "Array.h"
#include "concepts.h"
#include "macros.h"

#include <span>
#include <stdexcept>
#include <type_traits>

#include <immer/detail/type_traits.hpp>

template<typename T>
    requires(not std::is_array_v<T> and not std::is_reference_v<T>)
struct NODISCARD View : public std::span<const T>
{
public:
    using base = std::span<const T>;
    using base::base;

    using typename base::const_pointer;
    using typename base::element_type;
    using typename base::pointer;
    using typename base::reference;
    using typename base::size_type;
    using typename base::value_type;

    using iterator = decltype(std::declval<base &>().begin());
    using const_iterator = decltype(std::declval<const base &>().begin());

#if !defined(__cpp_lib_span) or __cpp_lib_span < 202311L
    NODISCARD constexpr reference at(const size_type pos) const
    {
        const base &self = *this;
        if (!(pos < self.size())) {
            throw std::out_of_range("pos");
        }
        return self[pos];
    }
#endif
    // This purposely hides the base class operator[] in order to require bounds checking.
    NODISCARD constexpr reference operator[](const size_type pos) const { return at(pos); }
};

template<concepts::IsMmapperArray A>
View(A) -> View<typename A::value_type>;

namespace concepts {

namespace detail {
namespace cpp11 {
template<typename T>
struct IsView : std::false_type
{};
template<typename T>
struct IsView<View<T>> : std::true_type
{};
} // namespace cpp11
namespace cpp17 {
template<typename T>
constexpr bool IsView_v = cpp11::IsView<T>::value;
}
} // namespace detail

template<typename T>
concept IsView = detail::cpp17::IsView_v<T>;

template<typename T>
concept IsStringViewGetter = requires(const T &x) {
    { x() } -> std::same_as<std::string_view>;
};

} // namespace concepts
