#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "Flags.h"

template<typename T, typename E, size_t _SIZE = enums::CountOf<E>::value>
class EnumIndexedArray : private std::array<T, _SIZE>
{
public:
    using index_type = E;
    using base = std::array<T, _SIZE>;
    static constexpr const size_t SIZE = _SIZE;

public:
    using std::array<T, _SIZE>::array;

public:
    auto operator[](E e) -> decltype(auto) { return base::at(static_cast<uint32_t>(e)); }
    auto operator[](E e) const -> decltype(auto) { return base::at(static_cast<uint32_t>(e)); }

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
};
