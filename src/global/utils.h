#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <algorithm>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>

#include "NullPointerException.h"
#include "macros.h"

namespace utils {
namespace details {
template<typename T>
constexpr bool isBitMask()
{
    return (std::is_integral_v<T> && std::is_unsigned_v<T>) || std::is_enum_v<T>;
}
} // namespace details

template<typename T>
constexpr bool isPowerOfTwo(const T x) noexcept
{
    static_assert(details::isBitMask<T>());
    if constexpr (std::is_enum_v<T>) {
        using U = std::underlying_type_t<T>;
        return isPowerOfTwo<U>(static_cast<U>(x));
    } else {
        return x != 0u && (x & (x - 1u)) == 0u;
    }
}

template<typename T>
constexpr bool isAtLeastTwoBits(const T x) noexcept
{
    static_assert(details::isBitMask<T>());
    if constexpr (std::is_enum_v<T>) {
        using U = std::underlying_type_t<T>;
        return isAtLeastTwoBits<U>(static_cast<U>(x));
    } else {
        return x != 0u && (x & (x - 1u)) != 0u;
    }
}

template<typename T>
bool anySet(const T src, const T mask)
{
    static_assert(details::isBitMask<T>());
    assert(isAtLeastTwoBits(mask));
    return (src & mask) != T{};
}

template<typename T>
bool allSet(const T src, const T mask)
{
    static_assert(details::isBitMask<T>());
    assert(isAtLeastTwoBits(mask));
    return (src & mask) == mask;
}

template<typename T>
bool isSet(const T src, const T bit)
{
    static_assert(details::isBitMask<T>());
    assert(isPowerOfTwo(bit));
    // use the test for any
    return (src & bit) != T{};
}

} // namespace utils

template<typename T>
bool isClamped(T x, T lo, T hi)
{
    return x >= lo && x <= hi;
}

namespace utils {
int round_ftoi(float f);
} // namespace utils

template<typename Base, typename Derived>
std::unique_ptr<Base> static_upcast(std::unique_ptr<Derived> &&ptr)
{
    static_assert(std::is_base_of_v<Base, Derived>);
    return std::unique_ptr<Base>(ptr.release());
}

template<typename T>
inline T &deref(T *const ptr)
{
    if (ptr == nullptr)
        throw NullPointerException();
    return *ptr;
}
// Technically we could make this return move or copy of the pointed-to value,
// but that's probably not what the caller expects.
template<typename T>
inline T deref(std::shared_ptr<T> &&ptr) = delete;
template<typename T>
inline T &deref(const std::shared_ptr<T> &ptr)
{
    if (ptr == nullptr)
        throw NullPointerException();
    return *ptr;
}
// Technically we could make this return move or copy of the pointed-to value,
// but that's probably not what the caller expects.
template<typename T>
inline T deref(std::unique_ptr<T> &&ptr) = delete;
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
    static_assert(std::is_same_v<Base, std::remove_reference_t<Base>>);
    static_assert(std::is_same_v<Derived, std::remove_reference_t<Derived>>);
    static_assert(std::is_pointer_v<Base>);
    static_assert(std::is_pointer_v<Derived>);
    static_assert(!std::is_same_v<Base, Derived>);

    using actual_base = std::remove_pointer_t<Base>;
    using actual_derived = std::remove_pointer_t<Derived>;

    static_assert(!std::is_same_v<actual_base, actual_derived>);
    static_assert(std::is_base_of_v<actual_base, actual_derived>);
    static_assert(std::is_const_v<actual_base> == std::is_const_v<actual_derived>);

    // Using reference to force dynamic_cast to throw.
    return &dynamic_cast<actual_derived &>(deref(ptr));
}

///  Can throw NullPointerException
template<typename /* must be specified */ Base, typename /* deduced */ Derived>
Base checked_static_upcast(Derived ptr) noexcept(false)
{
    static_assert(std::is_same_v<Derived, std::remove_reference_t<Derived>>);
    static_assert(std::is_same_v<Base, std::remove_reference_t<Base>>);
    static_assert(std::is_pointer_v<Derived>);
    static_assert(std::is_pointer_v<Base>);
    static_assert(!std::is_same_v<Derived, Base>);

    using actual_derived = std::remove_pointer_t<Derived>;
    using actual_base = std::remove_pointer_t<Base>;

    static_assert(!std::is_same_v<actual_derived, actual_base>);
    static_assert(std::is_base_of_v<actual_base, actual_derived>);
    static_assert(std::is_const_v<actual_derived> == std::is_const_v<actual_base>);

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

namespace utils {
NODISCARD std::optional<bool> getEnvBool(const char *key);
NODISCARD std::optional<int> getEnvInt(const char *key);
} // namespace utils

namespace utils {
// Use this if you're tired of having to use memcmp()
// to avoid compiler complaining about float comparison.
template<typename T>
bool equals(const T &a, const T &b)
{
    if constexpr (std::is_floating_point_v<T>)
        return std::memcmp(&a, &b, sizeof(T)) == 0;
    else
        return a == b;
}
} // namespace utils
