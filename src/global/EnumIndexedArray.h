#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

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
