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

#ifndef MMAPPER_UTILS_H
#define MMAPPER_UTILS_H

#include <algorithm>
#include <memory>
#include <type_traits>

#include "NullPointerException.h"

#define IS_SET(src, bit) static_cast<bool>((src) & (bit))

template<typename T>
T clamp(T x, T lo, T hi)
{
    return std::max(lo, std::min(x, hi));
}

template<typename T>
bool isClamped(T x, T lo, T hi)
{
    return x >= lo && x <= hi;
}

namespace utils {
int round_ftoi(float f);
}

template<typename Base, typename Derived>
std::unique_ptr<Base> static_upcast(std::unique_ptr<Derived> &&ptr)
{
    static_assert(std::is_base_of<Base, Derived>::value, "");
    return std::unique_ptr<Base>(ptr.release());
}

template<typename T>
inline T &deref(T *const ptr)
{
    if (ptr == nullptr)
        throw NullPointerException();
    return *ptr;
}
template<typename T>
inline T &deref(const std::shared_ptr<T> &ptr)
{
    if (ptr == nullptr)
        throw NullPointerException();
    return *ptr;
}
template<typename T>
inline T &deref(const std::unique_ptr<T> &ptr)
{
    if (ptr == nullptr)
        throw NullPointerException();
    return *ptr;
}

///  Can throw NullPointerException or std::bad_cast
template<typename /* must be specified */ Derived, typename /* deduced */ Base>
Derived checked_dynamic_downcast(Base ptr) noexcept(false)
{
    // TODO: convert to _t and _v instead of ::type and ::value when we switch to c++17,
    // and get rid of the `, ""` since c++17 allows single-arg static_assert.
    static_assert(std::is_same<Base, typename std::remove_reference<Base>::type>::value, "");
    static_assert(std::is_same<Derived, typename std::remove_reference<Derived>::type>::value, "");
    static_assert(std::is_pointer<Base>::value, "");
    static_assert(std::is_pointer<Derived>::value, "");
    static_assert(!std::is_same<Base, Derived>::value, "");

    using actual_base = typename std::remove_pointer<Base>::type;
    using actual_derived = typename std::remove_pointer<Derived>::type;

    static_assert(!std::is_same<actual_base, actual_derived>::value, "");
    static_assert(std::is_base_of<actual_base, actual_derived>::value, "");
    static_assert(std::is_const<actual_base>::value == std::is_const<actual_derived>::value, "");

    // Using reference to force dynamic_cast to throw.
    return &dynamic_cast<actual_derived &>(deref(ptr));
}

///  Can throw NullPointerException
template<typename /* must be specified */ Base, typename /* deduced */ Derived>
Base checked_static_upcast(Derived ptr) noexcept(false)
{
    // TODO: convert to _t and _v instead of ::type and ::value when we switch to c++17,
    // and get rid of the `, ""` since c++17 allows single-arg static_assert.
    static_assert(std::is_same<Derived, typename std::remove_reference<Derived>::type>::value, "");
    static_assert(std::is_same<Base, typename std::remove_reference<Base>::type>::value, "");
    static_assert(std::is_pointer<Derived>::value, "");
    static_assert(std::is_pointer<Base>::value, "");
    static_assert(!std::is_same<Derived, Base>::value, "");

    using actual_derived = typename std::remove_pointer<Derived>::type;
    using actual_base = typename std::remove_pointer<Base>::type;

    static_assert(!std::is_same<actual_derived, actual_base>::value, "");
    static_assert(std::is_base_of<actual_base, actual_derived>::value, "");
    static_assert(std::is_const<actual_derived>::value == std::is_const<actual_base>::value, "");

    return static_cast<Base>(&deref(ptr));
}

template<typename T>
inline auto as_unsigned_cstring(T s) = delete;

template<>
inline auto as_unsigned_cstring(const char *const s)
{
    return reinterpret_cast<const unsigned char *>(s);
}

template<typename T>
inline auto as_cstring(T s) = delete;

template<>
inline auto as_cstring(const unsigned char *const s)
{
    return reinterpret_cast<const char *>(s);
}

#endif // MMAPPER_UTILS_H
