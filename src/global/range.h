#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "macros.h"

#include <iterator> // IWYU pragma: keep (std::rbegin)

template<typename It>
struct NODISCARD range
{
    It begin_;
    It end_;

    NODISCARD It begin() const { return begin_; }
    NODISCARD It end() const { return end_; }
};

template<typename It>
NODISCARD range<It> make_range(It begin, It end)
{
    return range<It>{begin, end};
}

template<typename T>
NODISCARD auto make_reverse_range(T &&container)
{
    return make_range(std::rbegin(container), std::rend(container));
}
