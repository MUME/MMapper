#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include <limits>
#include <stdexcept>
#include <type_traits>

#include "macros.h"

template<typename Tag_, typename Wrapped_>
struct NODISCARD TaggedInt
{
public:
    using TagType = Tag_;
    using WrappedType = Wrapped_;
    static_assert(std::is_integral_v<WrappedType>);
    static_assert(std::is_unsigned_v<WrappedType>);

private:
    static constexpr const WrappedType MAX_VALUE = std::numeric_limits<WrappedType>::max();
    WrappedType m_value = 0;

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
    NODISCARD TaggedInt next() const
    {
        if (value() == MAX_VALUE) {
            throw std::runtime_error("overflow");
        }
        return TaggedInt(value() + 1);
    }
};

// for lack of a better place to put these...

template<typename...>
struct underlying_helper;

template<typename T>
struct underlying_helper<T>
{
    using type = T;
};

template<typename T>
struct underlying_helper<T, std::enable_if_t<std::is_enum_v<T>>>
{
    using type = typename std::underlying_type_t<T>;
};

template<typename Tag_, typename Type_>
struct underlying_helper<TaggedInt<Tag_, Type_>>
{
    using type = Type_;
};
