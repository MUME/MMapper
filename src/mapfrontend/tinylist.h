#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef TINYLIST
#define TINYLIST

#include <cassert>
#include <cstdint>
#include <type_traits>
#include <vector>

/**
 * extremely shrinked array list implementation to be used for each room
 * because we have so many rooms and want to search them really fast, we
 *  - allocate as little memory as possible
 *  - allow only 128 elements (1 per character)
 *  - only need 3 lines to access an element
 */
class SearchTreeNode;

class TinyList
{
public:
    using T = SearchTreeNode *;
    using iterator = T *;
    using index_type = uint32_t;
    using size_type = uint32_t;

private:
    static constexpr const index_type limit = 256u;

public:
    explicit TinyList() = default;

    ~TinyList() = default;

public:
    template<typename I>
    using IsConvertible =
        typename std::enable_if_t<std::is_integral<I>::value && !std::is_same<I, T>::value>;

    template<typename I, typename = IsConvertible<I>>
    static inline index_type index(I c)
    {
        if (std::is_signed<I>::value)
            return static_cast<index_type>(static_cast<std::make_unsigned_t<I>>(c));
        return static_cast<index_type>(c);
    }

    template<typename I, typename = IsConvertible<I>>
    T get(I c) const
    {
        return get(index(c));
    }

    template<typename I, typename = IsConvertible<I>>
    void put(I c, T object)
    {
        put(index(c), object);
    }

    template<typename I, typename = IsConvertible<I>>
    void remove(I c)
    {
        remove(index(c));
    }

public:
    T get(index_type c) const
    {
        assert(c < limit);
        if (c >= size())
            return nullptr;
        else
            return list[c];
    }

    void put(index_type c, T object)
    {
        assert(c < limit);
        if (c >= size()) {
            list.resize(c + 1u);
        }
        list[c] = object;
    }

    void remove(index_type c)
    {
        assert(c < limit);
        if (c < size())
            list[c] = nullptr;
    }

    size_type size() const { return static_cast<size_type>(list.size()); }

    iterator begin() { return list.data(); }

    iterator end() { return begin() + size(); }

private:
    std::vector<T> list{};
};

#endif
