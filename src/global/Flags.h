#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "bits.h"
#include "utils.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <type_traits>

namespace enums {

// REVISIT: add CountOf_v ?
template<typename E>
struct NODISCARD CountOf
{};
#define DEFINE_ENUM_COUNT(E, N) \
    namespace enums { \
    template<> \
    struct NODISCARD CountOf<E> \
    { \
        static_assert(std::is_enum_v<E>); \
        static constexpr const size_t value = N; \
    }; \
    }

template<typename CRTP,
         typename Flag_,
         typename UnderlyingType_,
         size_t NUM_FLAGS_ = CountOf<Flag_>::value>
class NODISCARD Flags
{
public:
    using Flag = Flag_;
    using underlying_type = UnderlyingType_;
    static_assert(std::is_enum_v<Flag>);
    static_assert(std::is_integral_v<underlying_type>);
    static_assert(!std::numeric_limits<underlying_type>::is_signed);
    static constexpr const size_t NUM_FLAGS = NUM_FLAGS_;
    static_assert(NUM_FLAGS != 0u);
    static_assert(NUM_FLAGS <= std::numeric_limits<underlying_type>::digits);

private:
    static constexpr const underlying_type MASK = static_cast<underlying_type>(
        static_cast<underlying_type>(static_cast<underlying_type>(~underlying_type{0u})
                                     >> (std::numeric_limits<underlying_type>::digits - NUM_FLAGS)));
    underlying_type m_flags = 0u;

    template<typename T>
    NODISCARD static inline constexpr underlying_type narrow(const T x) noexcept
    {
        return static_cast<underlying_type>(x & MASK);
    }

public:
    /* implicit */ constexpr Flags() noexcept = default;

    explicit constexpr Flags(const underlying_type flags) noexcept
        : m_flags{narrow(flags & MASK)}
    {}

    // This might be tempting to make implcit, but it's not worth it because it doesn't enable the
    // `Flags binop(Flag, Flag)` helper operators (see below).
    explicit constexpr Flags(const Flag flag) noexcept
        : Flags{narrow(underlying_type{1u} << static_cast<int>(flag))}
    {}

private:
    NODISCARD constexpr CRTP &crtp_self() noexcept { return static_cast<CRTP &>(*this); }
    NODISCARD constexpr const CRTP &crtp_self() const noexcept
    {
        return static_cast<CRTP &>(*this);
    }

public:
    NODISCARD explicit constexpr operator underlying_type() const noexcept { return m_flags; }
    NODISCARD constexpr uint32_t asUint32() const noexcept
    {
        // Note: static_assert here isn't checked unless you call this function.
        static_assert(sizeof(underlying_type) <= sizeof(uint32_t),
                      "asUint32() is disabled because the underlying type is larger than 32-bits");
        return m_flags;
    }

public:
    NODISCARD friend inline constexpr bool operator==(const CRTP lhs, const CRTP rhs) noexcept
    {
        return lhs.m_flags == rhs.m_flags;
    }
    NODISCARD friend inline constexpr bool operator!=(const CRTP lhs, const CRTP rhs) noexcept
    {
        return lhs.m_flags != rhs.m_flags;
    }

    // REVISIT: These `Flags binop(Flag, Flag)` helper operators are not effective without `using namespace enums;`,
    // because the scoping rules require inline friends to be defined in the enclosing namepace of the class
    // that defines them, which ends up being `::enums` in our case. :(
    // Note: The others operators that use a Flags argument work because of ADL.
public:
    NODISCARD friend inline constexpr CRTP operator&(const Flag lhs, const Flag rhs) noexcept
    {
        return CRTP{lhs} & CRTP{rhs};
    }

    NODISCARD friend inline constexpr CRTP operator|(const Flag lhs, const Flag rhs) noexcept
    {
        return CRTP{lhs} | CRTP{rhs};
    }

    NODISCARD friend inline constexpr CRTP operator^(const Flag lhs, const Flag rhs) noexcept
    {
        /* parens to keep clang-format from formatting this strangely */
        return CRTP{lhs} ^ (CRTP{rhs});
    }

public:
    NODISCARD friend inline constexpr CRTP operator&(const CRTP lhs, const Flag rhs) noexcept
    {
        return lhs & CRTP{rhs};
    }
    NODISCARD friend inline constexpr CRTP operator|(const CRTP lhs, const Flag rhs) noexcept
    {
        return lhs | CRTP{rhs};
    }
    NODISCARD friend inline constexpr CRTP operator^(const CRTP lhs, const Flag rhs) noexcept
    {
        /* parens to keep clang-format from formatting this strangely */
        return lhs ^ (CRTP{rhs});
    }

public:
    NODISCARD friend inline constexpr CRTP operator&(const CRTP lhs, const CRTP rhs) noexcept
    {
        return CRTP{narrow(lhs.m_flags & rhs.m_flags)};
    }

    NODISCARD friend inline constexpr CRTP operator|(const CRTP lhs, const CRTP rhs) noexcept
    {
        return CRTP{narrow(lhs.m_flags | rhs.m_flags)};
    }

    NODISCARD friend inline constexpr CRTP operator^(const CRTP lhs, const CRTP rhs) noexcept
    {
        return CRTP{narrow(lhs.m_flags ^ rhs.m_flags)};
    }

public:
    NODISCARD inline constexpr CRTP operator~() const noexcept { return CRTP{narrow(~m_flags)}; }
    NODISCARD inline constexpr explicit operator bool() const noexcept { return m_flags != 0u; }

public:
    inline constexpr CRTP &operator&=(const Flag rhs) { return crtp_self() &= CRTP{rhs}; }
    inline constexpr CRTP &operator&=(const CRTP rhs)
    {
        auto &self = crtp_self();
        self = (self & rhs);
        return self;
    }

public:
    inline constexpr CRTP &operator|=(const Flag rhs) noexcept { return crtp_self() |= CRTP{rhs}; }
    inline constexpr CRTP &operator|=(const CRTP rhs) noexcept
    {
        auto &self = crtp_self();
        self = (self | rhs);
        return self;
    }

public:
    inline constexpr CRTP &operator^=(const Flag rhs) noexcept { return crtp_self() ^= CRTP{rhs}; }
    inline constexpr CRTP &operator^=(const CRTP rhs) noexcept
    {
        auto &self = crtp_self();
        self = (self ^ rhs);
        return self;
    }

public:
    NODISCARD inline constexpr bool contains(const Flag flag) const noexcept
    {
        return (m_flags & CRTP{flag}.m_flags) != 0u;
    }
    NODISCARD inline constexpr bool containsAny(const CRTP rhs) const noexcept
    {
        return (m_flags & rhs.m_flags) != 0u;
    }
    NODISCARD inline constexpr bool containsAll(const CRTP rhs) const noexcept
    {
        return (m_flags & rhs.m_flags) == rhs.m_flags;
    }
    inline constexpr void insert(const Flag flag) noexcept { crtp_self() |= flag; }
    inline constexpr void remove(const Flag flag) noexcept { crtp_self() &= ~CRTP{flag}; }
    inline constexpr void clear() noexcept { m_flags = 0; }
    NODISCARD inline constexpr bool isEmpty() const { return m_flags == 0; }
    NODISCARD inline constexpr bool empty() const { return isEmpty(); }
    NODISCARD inline size_t count() const { return static_cast<size_t>(bits::bitCount(m_flags)); }
    NODISCARD inline size_t size() const { return count(); }

private:
    static void remove_lowest_bit(underlying_type &flags)
    {
        using U = underlying_type;
        static_assert(std::is_unsigned_v<U>);
        assert(flags != 0u);
        flags = static_cast<U>(flags & static_cast<U>(flags - U{1}));
    }

public:
    // CAUTION: This function behaves differently than you might expect;
    // it returns the nth bit set, defined in order from least-significant
    // to most-significant bit that's currently set in the mask.
    //
    // Refer to test::testFlags() in Flags.cpp for an example.
    //
    // Note: both [] and at() perform bounds checking in debug mode.
    NODISCARD Flag at(const size_t n_) const noexcept
    {
        auto n = n_;
        assert(n < count());
        for (auto tmp = m_flags; tmp != 0u; remove_lowest_bit(tmp)) {
            const auto lsb = bits::leastSignificantBit(tmp);
            assert(lsb >= 0);
            const auto x = static_cast<Flag>(lsb);
            assert(contains(x));
            if (n-- == 0) {
                return x;
            }
        }

        std::abort(); /* crash */
    }
    NODISCARD Flag operator[](const size_t n) const noexcept { return at(n); }

    /// Syntax:
    ///  find_first_matching([](Flag f) -> bool { /* ... */ });
    /// Think of it as
    ///  using Predicate = std::function<bool(Flag)>;
    template<typename Predicate>
    NODISCARD std::optional<Flag> find_first_matching(Predicate &&predicate) const
    {
        for (auto tmp = m_flags; tmp != 0u; remove_lowest_bit(tmp)) {
            const auto lsb = bits::leastSignificantBit(tmp);
            assert(lsb >= 0);
            const auto x = static_cast<Flag>(lsb);
            assert(contains(x));
            if (predicate(x)) {
                return x;
            }
        }
        return std::nullopt;
    }

    /// Syntax:
    ///  for_each([](Flag f) -> void { /* ... */ });
    /// Think of it as
    ///  using Callback = std::function<void(Flag)>;
    template<typename Callback>
    void for_each(Callback &&callback) const
    {
        for (auto tmp = m_flags; tmp != 0u; remove_lowest_bit(tmp)) {
            const auto lsb = bits::leastSignificantBit(tmp);
            assert(lsb >= 0);
            const auto x = static_cast<Flag>(lsb);
            assert(contains(x));
            callback(x);
        }
    }

public:
    struct NODISCARD Iterator final
    {
    private:
        Flags::underlying_type m_bits = 0;
        size_t m_pos = 0;

    public:
        Iterator(const Flags flags, const size_t pos)
            : m_bits{flags.m_flags}
            , m_pos{pos}
        {}

    public:
        NODISCARD Flag operator*() const { return Flags{m_bits}.at(m_pos); }
        ALLOW_DISCARD Iterator operator++()
        {
            assert(m_pos < Flags{m_bits}.size());
            ++m_pos;
            return *this;
        }
        void operator++(int) = delete;
        NODISCARD bool operator==(const Iterator &rhs) const
        {
            return m_bits == rhs.m_bits && m_pos == rhs.m_pos;
        }
        NODISCARD bool operator!=(const Iterator &rhs) const { return !(rhs == *this); }
    };

public:
    NODISCARD Iterator begin() const { return Iterator{*this, 0}; }
    NODISCARD Iterator end() const { return Iterator{*this, size()}; }
};

} // namespace enums

namespace test {
extern void testFlags();
} // namespace test
