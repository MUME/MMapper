#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <iterator> // IWYU pragma: keep (std::rbegin)

template<typename It>
struct range
{
    It begin_;
    It end_;

    It begin() const { return begin_; }
    It end() const { return end_; }
};

template<typename It>
range<It> make_range(It begin, It end)
{
    return range<It>{begin, end};
}

template<typename T>
auto make_reverse_range(T &&container)
{
    return make_range(std::rbegin(container), std::rend(container));
}
