#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "Array.h"
#include "Flags.h"
#include "View.h"
#include "enums.h"
#include "macros.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <type_traits>

template<typename T, concepts::IsUnsignedEnum E, size_t SIZE_ = enums::CountOf<E>::value>
    requires(SIZE_ > 0)
class NODISCARD EnumIndexedArray : private MMapper::Array<T, SIZE_>
{
public:
    using base = MMapper::Array<T, SIZE_>;
    using typename base::const_iterator;
    using typename base::const_pointer;
    using typename base::const_reference;
    using typename base::iterator;
    using typename base::pointer;
    using typename base::reference;
    using typename base::size_type;
    using typename base::value_type;
    using element_type = T; // std::array doesn't have element_type
    using index_type = E;
    static constexpr size_t SIZE = SIZE_;

public:
    // inherits all constructors
    using base::base;

public:
    NODISCARD decltype(auto) at(E e) { return base::at(static_cast<uint32_t>(e)); }
    NODISCARD decltype(auto) at(E e) const { return base::at(static_cast<uint32_t>(e)); }
    NODISCARD decltype(auto) operator[](E e) { return at(e); }
    NODISCARD decltype(auto) operator[](E e) const { return at(e); }

public:
    using base::data;
    using base::empty;
    using base::size;

public:
    using base::begin;
    using base::cbegin;
    using base::cend;
    using base::end;

public:
    template<typename U>
    NODISCARD std::optional<E> findIndexOf(const U element) const
    {
        const auto beg = this->begin();
        const auto end = this->end();
        const auto it = std::find_if(beg, end, [element](auto &x) -> bool { return x == element; });
        if (it == end) {
            return std::nullopt;
        }
        return static_cast<E>(it - beg);
    }
    // This is a custom variation of findIndexOf() that allows searching pointer
    // values when the pointer lives in a smart pointer, an optional, etc.
    //
    // Note: findIndexOf() would write "x == ptr" but this uses "std::addressof(deref(x)) == ptr" instead.
    // Also: "deref(x)" can throw, and std::addressof() ignores overloaded "operator&".
    template<typename U>
        requires(requires(const T &x) {
            { std::addressof(deref(x)) } -> std::same_as<U *>;
        })
    NODISCARD std::optional<E> findIndexOfPointer(U *const pointer) const
    {
        const auto beg = this->begin();
        const auto end = this->end();
        const auto it = std::find_if(beg, end, [pointer](auto &x) -> bool {
            return std::addressof(deref(x)) == pointer;
        });
        if (it == end) {
            return std::nullopt;
        }
        return static_cast<E>(it - beg);
    }

    template<typename Callback>
    void for_each(Callback &&callback)
    {
        for (auto &x : *this) {
            callback(x);
        }
    }

public:
    NODISCARD bool operator==(const EnumIndexedArray &other) const
    {
        return static_cast<const base &>(*this) == static_cast<const base &>(other);
    }
    NODISCARD bool operator!=(const EnumIndexedArray &other) const { return !operator==(other); }
};

namespace concepts {
namespace detail {
namespace cpp11 {
template<typename T>
struct IsEnumIndexedArray : std::false_type
{};
template<typename T, IsUnsignedEnum E, size_t size>
struct IsEnumIndexedArray<EnumIndexedArray<T, E, size>> : std::true_type
{};
} // namespace cpp11
namespace cpp17 {
template<typename T>
inline constexpr bool IsEnumIndexedArray_v = cpp11::IsEnumIndexedArray<T>::value;
}
} // namespace detail
template<typename T>
concept IsEnumIndexedArray = detail::cpp17::IsEnumIndexedArray_v<T>;
} // namespace concepts

namespace concepts {
template<typename T>
concept IsEnumIndexedArrayOfStrings
    = IsEnumIndexedArray<T> and std::convertible_to<typename T::value_type, std::string_view>;
} // namespace concepts
