#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "Array.h"
#include "Flags.h"
#include "macros.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

template<typename T, typename E, size_t SIZE_ = enums::CountOf<E>::value>
class NODISCARD EnumIndexedArray : private MMapper::Array<T, SIZE_>
{
public:
    using base = MMapper::Array<T, SIZE_>;
    using value_type = typename base::value_type;
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
    NODISCARD std::optional<E> findIndexOfPointer(U *const pointer) const
    {
        static_assert(
            std::is_same_v<U *, decltype(std::addressof(deref(std::declval<const T &>())))>);
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
