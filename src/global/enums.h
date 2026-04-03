#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "Array.h"
#include "Flags.h"
#include "View.h"
#include "concepts.h"

#include <cassert>
#include <cstddef>

namespace concepts {
template<typename E>
concept with_UNDEFINED = requires(E x) { x = E::UNDEFINED; };
template<typename E>
concept without_UNDEFINED = not with_UNDEFINED<E>;

template<typename E>
concept IsEnum_with_UNDEFINED = IsUnsignedEnum<E> and with_UNDEFINED<E>;
template<typename E>
concept IsEnum_without_UNDEFINED = IsUnsignedEnum<E> and without_UNDEFINED<E>;
} // namespace concepts

namespace enums {

template<concepts::IsEnum E>
constexpr auto to_underlying(const E e) noexcept
{
#if !defined(__cpp_lib_to_underlying) or __cpp_lib_to_underlying < 202102L
    return static_cast<std::underlying_type_t<E>>(e);
#else
    return std::to_underlying(e); // c++23
#endif
}

template<concepts::IsUnsignedEnum E, const size_t N = CountOf_v<E>>
    requires(N > 0)
NODISCARD constexpr auto genEnumValues() -> MMapper::Array<E, N>
{
    MMapper::Array<E, N> result;
    for (size_t i = 0u; i < N; ++i) {
        result[i] = static_cast<E>(i);
    }
    return result;
}

template<concepts::IsUnsignedEnum E, const size_t N = CountOf_v<E>>
    requires(N > (concepts::IsEnum_without_UNDEFINED<E> ? 0 : 1))
NODISCARD auto genEnumValues_without_UNDEFINED() -> View<E>
{
    static auto values = std::invoke([]() constexpr {
        if constexpr (concepts::IsEnum_without_UNDEFINED<E>) {
            return genEnumValues<E, N>();
        } else {
            constexpr size_t UNDEF = to_underlying(E::UNDEFINED);
            constexpr size_t SIZE = (UNDEF < N) ? (N - 1) : N;
            MMapper::Array<E, SIZE> tmp;
            size_t o = 0;
            for (size_t i = 0u; i < N; ++i) {
                const auto e = static_cast<E>(i);
                if (e != E::UNDEFINED) {
                    tmp[o++] = e;
                }
            }
            assert(o == SIZE);
            return tmp;
        }
    });
    static constinit auto view = View<E>{values};
    return view;
}
} // namespace enums

#define DECL_ENUM_GETTER2(_E, _N, _name) \
    NODISCARD inline auto _name() -> View<_E> \
    { \
        using E = MM_TYPE_IDENTITY(_E); \
        static_assert(concepts::IsEnum<E>); \
        static constexpr size_t N = (_N); \
        static_assert(N == ::enums::CountOf_v<E>); \
        return ::enums::genEnumValues_without_UNDEFINED<E>(); \
    }

// for use only with flags-like enums; consider using Flags<E> instead
#define ENUM_ALLOW_BIT_AND(_E, _maybe_friend) \
    NODISCARD _maybe_friend inline constexpr MM_TYPE_IDENTITY( \
        _E) operator&(const MM_TYPE_IDENTITY(_E) lhs, const MM_TYPE_IDENTITY(_E) rhs) noexcept \
    { \
        static_assert(concepts::IsUnsignedEnum<_E>); \
        return static_cast<_E>(enums::to_underlying(lhs) & enums::to_underlying(rhs)); \
    } \
    ALLOW_DISCARD _maybe_friend inline constexpr MM_TYPE_IDENTITY(_E) & \
    operator&=(MM_TYPE_IDENTITY(_E) & lhs, const MM_TYPE_IDENTITY(_E) rhs) noexcept \
    { \
        return lhs = (lhs & rhs); \
    }

// for use only with flags-like enums; consider using enums::Flags instead
#define ENUM_ALLOW_BIT_OR(_E, _maybe_friend) \
    NODISCARD _maybe_friend inline constexpr MM_TYPE_IDENTITY( \
        _E) operator|(const MM_TYPE_IDENTITY(_E) lhs, const MM_TYPE_IDENTITY(_E) rhs) noexcept \
    { \
        static_assert(concepts::IsUnsignedEnum<_E>); \
        return static_cast<_E>(enums::to_underlying(lhs) | enums::to_underlying(rhs)); \
    } \
    ALLOW_DISCARD _maybe_friend inline constexpr MM_TYPE_IDENTITY(_E) & \
    operator|=(MM_TYPE_IDENTITY(_E) & lhs, const MM_TYPE_IDENTITY(_E) rhs) noexcept \
    { \
        return lhs = (lhs | rhs); \
    }

// for use only with flags-like enums; consider using enums::Flags instead
#define ENUM_ALLOW_BIT_COMPL(_E, _Bits, _maybe_friend) \
    NODISCARD _maybe_friend inline constexpr _E operator~(const MM_TYPE_IDENTITY(_E) x) noexcept \
    { \
        static_assert(concepts::IsUnsignedEnum<_E>); \
        constexpr size_t N = (_Bits); \
        using U = std::underlying_type_t<_E>; \
        static_assert(N > 0); \
        static_assert(N <= std::numeric_limits<U>::digits); \
        constexpr U mask = (U{1} << N) - U{1}; \
        static_assert(((mask + 1) & mask) == U{0}); \
        return static_cast<_E>(~enums::to_underlying(x) & mask); \
    }

// for use only with flags-like enums; consider using enums::Flags instead
#define ENUM_ALLOWS_BIT_OPS_WITH_WIDTH(_E, _N) \
    ENUM_ALLOW_BIT_AND(_E, ) \
    ENUM_ALLOW_BIT_OR(_E, ) ENUM_ALLOW_BIT_COMPL(_E, (_N), )

// for use only with flags-like enums; consider using enums::Flags instead
#define FRIEND_ENUM_ALLOWS_BIT_OPS_WITH_WIDTH(_E, _N) \
    ENUM_ALLOW_BIT_AND(_E, friend) \
    ENUM_ALLOW_BIT_OR(_E, friend) ENUM_ALLOW_BIT_COMPL(_E, (_N), friend)
