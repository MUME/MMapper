#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
