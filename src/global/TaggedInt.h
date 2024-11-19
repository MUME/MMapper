#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "macros.h"

#include <limits>
#include <stdexcept>
#include <type_traits>

template<typename Crtp_, typename Tag_, typename Wrapped_, Wrapped_ DefaultValue_ = Wrapped_{}>
struct NODISCARD TaggedInt
{
public:
    using TagType = Tag_;
    using WrappedType = Wrapped_;
    static_assert(std::is_integral_v<WrappedType>);

public:
    static constexpr const WrappedType DEFAULT_VALUE = DefaultValue_;
    static constexpr const WrappedType MIN_VALUE = std::numeric_limits<WrappedType>::min();
    static constexpr const WrappedType MAX_VALUE = std::numeric_limits<WrappedType>::max();

private:
    WrappedType m_value = DEFAULT_VALUE;

public:
    constexpr TaggedInt() = default;

    constexpr explicit TaggedInt(WrappedType value)
        : m_value{value}
    {}

public:
    NODISCARD constexpr WrappedType value() const { return m_value; }
    NODISCARD constexpr explicit operator WrappedType() const { return m_value; }

public:
    NODISCARD constexpr bool operator==(const TaggedInt &rhs) const
    {
        return m_value == rhs.m_value;
    }
    NODISCARD constexpr bool operator!=(const TaggedInt &rhs) const { return !operator==(rhs); }

public:
    NODISCARD constexpr bool operator<(const TaggedInt &rhs) const { return m_value < rhs.m_value; }
    NODISCARD constexpr bool operator>(const TaggedInt &rhs) const { return rhs < *this; }
    NODISCARD constexpr bool operator<=(const TaggedInt &rhs) const { return !operator>(rhs); }
    NODISCARD constexpr bool operator>=(const TaggedInt &rhs) const { return !operator<(rhs); }

public:
    NODISCARD constexpr Crtp_ next() const
    {
        if (value() == MAX_VALUE) {
            throw std::runtime_error("overflow");
        }
        return Crtp_(value() + 1);
    }

    // pre-increment: ++x;
    ALLOW_DISCARD Crtp_ &operator++()
    {
        *this = next();
        return *this;
    }

    // post-increment: auto y = x++;
    NODISCARD Crtp_ operator++(int) // NOLINT (returning const is an anti-pattern)
    {
        WrappedType before = value();
        *this = next();
        return Crtp_{before};
    }
};

// for lack of a better place to put these...

template<typename...>
struct NODISCARD underlying_helper;

template<typename T>
struct NODISCARD underlying_helper<T>
{
    using type = T;
};

template<typename T>
struct NODISCARD underlying_helper<T, std::enable_if_t<std::is_enum_v<T>>>
{
    using type = typename std::underlying_type_t<T>;
};

template<typename Crtp_, typename Tag_, typename Type_>
struct NODISCARD underlying_helper<TaggedInt<Crtp_, Tag_, Type_>>
{
    using type = Type_;
};
