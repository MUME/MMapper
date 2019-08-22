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

#include <array>
#include <cstddef>

namespace enums {

template<typename T, const size_t N>
inline std::array<T, N> genEnumValues()
{
    std::array<T, N> result{};
    for (size_t i = 0u; i < N; ++i) {
        result[i] = static_cast<T>(i);
    }
    return result;
}

template<typename E, typename It>
struct counting_iterator
{
    It begin_;
    It it_;
    It end_;

    void operator++(int) = delete;
    counting_iterator &operator++()
    {
        if (it_ >= end_)
            throw std::runtime_error("overflow");
        ++it_;
        return *this;
    }

    bool operator==(const counting_iterator &rhs) const
    {
        if (begin_ != rhs.begin_ || end_ != rhs.end_)
            throw std::runtime_error("invalid comparison");
        return it_ == rhs.it_;
    }
    bool operator!=(const counting_iterator &rhs) const { return !operator==(rhs); }

    E operator*()
    {
        if (it_ >= end_)
            throw std::runtime_error("overflow");

        return static_cast<E>(it_ - begin_);
    }
};

template<typename E, typename It>
struct counting_range
{
    using iterator = counting_iterator<E, It>;
    It beg_;
    It end_;
    auto begin() { return iterator{beg_, beg_, end_}; }
    auto end() { return iterator{beg_, end_, end_}; }
};

template<typename E, typename T>
auto makeCountingIterator(T &&container)
{
    auto beg = std::begin(container);
    return counting_range<E, decltype(beg)>{beg, std::end(container)};
}

} // namespace enums
