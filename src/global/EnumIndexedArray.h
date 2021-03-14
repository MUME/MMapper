#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "Array.h"
#include "Flags.h"

template<typename T, typename E, size_t _SIZE = enums::CountOf<E>::value>
class EnumIndexedArray : private MMapper::Array<T, _SIZE>
{
public:
    using index_type = E;
    using base = typename MMapper::Array<T, _SIZE>;
    static constexpr const size_t SIZE = _SIZE;

public:
    // inherits all constructors
    using MMapper::Array<T, _SIZE>::Array;

public:
    decltype(auto) at(E e) { return base::at(static_cast<uint32_t>(e)); }
    decltype(auto) at(E e) const { return base::at(static_cast<uint32_t>(e)); }
    decltype(auto) operator[](E e) { return at(e); }
    decltype(auto) operator[](E e) const { return at(e); }

public:
    using base::data;
    using base::size;

public:
    using base::begin;
    using base::cbegin;
    using base::cend;
    using base::end;

public:
    std::optional<E> findIndexOf(const T element) const
    {
        const auto beg = this->begin();
        const auto end = this->end();
        const auto it = std::find(beg, end, element);
        if (it == end)
            return std::nullopt;

        return static_cast<E>(it - beg);
    }

    template<typename Callback>
    void for_each(Callback &&callback)
    {
        for (auto &x : *this)
            callback(x);
    }

public:
    NODISCARD bool operator==(const EnumIndexedArray &other) const
    {
        return static_cast<const base &>(*this) == static_cast<const base &>(other);
    }
    NODISCARD bool operator!=(const EnumIndexedArray &other) const { return !operator==(other); }
};
