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
