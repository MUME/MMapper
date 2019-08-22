#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <type_traits>

namespace type_utils {
template<typename Seeking>
static inline constexpr bool startsWith()
{
    return false;
}
template<typename Seeking, typename First, typename... Rest>
static inline constexpr bool startsWith()
{
    return std::is_same_v<Seeking, std::decay_t<First>>;
}
template<typename Seeking>
static inline constexpr bool contains()
{
    return false;
}
template<typename Seeking, typename First, typename... Rest>
static inline constexpr bool contains()
{
    return startsWith<Seeking, First>() || contains<Seeking, Rest...>();
}

template<typename FirstWanted, typename... OtherWanted>
struct ValidTypes
{
    template<typename FirstArg>
    static inline constexpr bool check()
    {
        return contains<FirstArg, FirstWanted, OtherWanted...>();
    }

    template<typename FirstArg, typename SecondArg, typename... OtherArgs>
    static inline constexpr bool check()
    {
        return check<FirstArg>() && check<SecondArg, OtherArgs...>();
    }
};

} // namespace type_utils
