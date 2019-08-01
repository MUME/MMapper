#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace enums {

// REVISIT: add CountOf_v ?
template<typename E>
struct CountOf
{};
#define DEFINE_ENUM_COUNT(E, N) \
    namespace enums { \
    template<> \
    struct CountOf<E> \
    { \
        static_assert(std::is_enum_v<E>); \
        static constexpr const size_t value = N; \
    }; \
    }

template<typename CRTP, typename _Flag, typename _UnderlyingType>
class Flags
{
public:
    using Flag = _Flag;
    using underlying_type = _UnderlyingType;
    static_assert(std::is_enum_v<Flag>);
    static_assert(std::is_integral_v<underlying_type>);
    static_assert(!std::numeric_limits<underlying_type>::is_signed);
    static constexpr const size_t NUM_FLAGS = CountOf<Flag>::value;
    static_assert(NUM_FLAGS != 0u);
    static_assert(NUM_FLAGS <= std::numeric_limits<underlying_type>::digits);

private:
    static constexpr const underlying_type MASK = static_cast<underlying_type>(
        static_cast<underlying_type>(static_cast<underlying_type>(~underlying_type{0u})
                                     >> (std::numeric_limits<underlying_type>::digits - NUM_FLAGS)));
    underlying_type flags = 0u;

    template<typename T>
    static inline constexpr underlying_type narrow(T x) noexcept
    {
        return static_cast<underlying_type>(x & MASK);
    }

public:
    explicit constexpr Flags() noexcept = default;

    explicit constexpr Flags(underlying_type flags) noexcept
        : flags{narrow(flags & MASK)}
    {}

    explicit constexpr Flags(Flag flag) noexcept
        : Flags{narrow(underlying_type{1u} << static_cast<int>(flag))}
    {}

private:
    CRTP &crtp_self() noexcept { return static_cast<CRTP &>(*this); }

    const CRTP &crtp_self() const noexcept { return static_cast<CRTP &>(*this); }

public:
    explicit constexpr operator underlying_type() const noexcept { return flags; }

    constexpr uint32_t asUint32() const noexcept { return flags; }

public:
    friend inline bool operator==(CRTP lhs, CRTP rhs) noexcept { return lhs.flags == rhs.flags; }

    friend inline bool operator!=(CRTP lhs, CRTP rhs) noexcept { return lhs.flags != rhs.flags; }

public:
    friend inline constexpr CRTP operator|(Flag lhs, Flag rhs) noexcept
    {
        return CRTP{lhs} | CRTP{rhs};
    }

public:
    friend inline constexpr CRTP operator&(CRTP lhs, Flag rhs) noexcept { return lhs & CRTP{rhs}; }

    friend inline constexpr CRTP operator|(CRTP lhs, Flag rhs) noexcept { return lhs | CRTP{rhs}; }

    friend inline constexpr CRTP operator^(CRTP lhs, Flag rhs) noexcept
    {
        /* parens to keep clang-format from formatting this strangely */
        return lhs ^ (CRTP{rhs});
    }

public:
    friend inline constexpr CRTP operator&(CRTP lhs, CRTP rhs) noexcept
    {
        return CRTP{narrow(lhs.flags & rhs.flags)};
    }

    friend inline constexpr CRTP operator|(CRTP lhs, CRTP rhs) noexcept
    {
        return CRTP{narrow(lhs.flags | rhs.flags)};
    }

    friend inline constexpr CRTP operator^(CRTP lhs, CRTP rhs) noexcept
    {
        return CRTP{narrow(lhs.flags ^ rhs.flags)};
    }

public:
    inline constexpr CRTP operator~() const noexcept { return CRTP{narrow(~flags)}; }

    inline constexpr explicit operator bool() const noexcept { return flags != 0u; }

public:
    CRTP &operator&=(Flag rhs) { return crtp_self() &= CRTP{rhs}; }

    CRTP &operator&=(CRTP rhs)
    {
        auto &self = crtp_self();
        self = (self & rhs);
        return self;
    }

public:
    CRTP &operator|=(Flag rhs) noexcept { return crtp_self() |= CRTP{rhs}; }

    CRTP &operator|=(CRTP rhs) noexcept
    {
        auto &self = crtp_self();
        self = (self | rhs);
        return self;
    }

public:
    CRTP &operator^=(Flag rhs) noexcept { return crtp_self() ^= CRTP{rhs}; }

    CRTP &operator^=(CRTP rhs) noexcept
    {
        auto &self = crtp_self();
        self = (self ^ rhs);
        return self;
    }

public:
    bool contains(Flag flag) const noexcept { return (flags & CRTP{flag}.flags) != 0u; }

    bool containsAny(CRTP rhs) const noexcept { return (flags & rhs.flags) != 0u; }

    bool containsAll(CRTP rhs) const noexcept { return (flags & rhs.flags) == rhs.flags; }

    void insert(Flag flag) noexcept { crtp_self() |= flag; }

    void remove(Flag flag) noexcept { crtp_self() &= ~CRTP{flag}; }
};
} // namespace enums
