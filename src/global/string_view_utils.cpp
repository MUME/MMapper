// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)

#include "string_view_utils.h"

#include "Consts.h"

#include <limits>
#include <optional>

namespace { // anonymous
namespace detail {

NODISCARD constexpr char16_t to_char16(const char c) noexcept
{
    // zero-extend possibly signed Latin1 char to unsigned char16_t
    return static_cast<char16_t>(static_cast<uint8_t>(c));
}

static_assert(to_char16('\xFF') == 0xFFu); // not 0xFFFFu

constexpr bool are_equivalent_latin1(const std::u16string_view left,
                                     const std::string_view right) noexcept
{
    // we could use some Qt function, as for example QtStringView::compare(QLatin1String), but:
    // 1. we are reducing the dependencies on Qt
    // 2. it's difficult to find a non-allocating comparison between UTF-16 and Latin1 strings in Qt

    const size_t n = left.size();
    if (n != right.size()) {
        return false;
    }
    for (size_t i = 0; i < n; ++i) {
        if (left[i] != to_char16(right[i])) {
            return false;
        }
    }
    return true;
}

constexpr std::string_view latin1_empty{};
constexpr std::u16string_view utf16_empty{};

constexpr std::string_view latin1_x{"x"};
constexpr std::u16string_view utf16_x{u"x"};

constexpr std::string_view latin1_ff{"\xFF"};
static_assert(latin1_ff.size() == 1);
static_assert(latin1_ff.front() == static_cast<char>(0xFF));

constexpr std::string_view utf8_ff{"\u00FF"};
static_assert(utf8_ff.size() == 2);
static_assert(utf8_ff.front() == static_cast<char>(0xC3));
static_assert(utf8_ff.back() == static_cast<char>(0xBF));

constexpr std::u16string_view utf16_ff{u"\u00FF"};
static_assert(utf16_ff.size() == 1);
static_assert(utf16_ff.front() == static_cast<char16_t>(0xFF));

static_assert(are_equivalent_latin1(utf16_empty, latin1_empty));
static_assert(!are_equivalent_latin1(utf16_x, latin1_empty));
static_assert(!are_equivalent_latin1(utf16_empty, latin1_x));
static_assert(are_equivalent_latin1(utf16_x, latin1_x));
static_assert(are_equivalent_latin1(utf16_ff, latin1_ff));
static_assert(!are_equivalent_latin1(utf16_ff, utf8_ff));

} // namespace detail
} // namespace

bool are_equivalent_latin1(const std::u16string_view left, const std::string_view right) noexcept
{
    return detail::are_equivalent_latin1(left, right);
}

namespace { // anonymous
namespace detail {

NODISCARD constexpr std::optional<uint64_t> to_integer_u64(std::u16string_view str)
{
    if (str.empty()) {
        return std::nullopt;
    }
    uint64_t ret = 0;
    for (const char16_t ch : str) {
        if (ch < '0' || ch > '9') {
            return std::nullopt;
        }
        const auto digit = static_cast<uint32_t>(ch - '0');
        const uint64_t next = ret * 10 + digit;
        // on overflow we lose at least the top bit => next is less than half the value it should be,
        // so divided by 8 will be less than the original (non multiplied by 10) value
        if (next / 8 < ret) {
            return std::nullopt;
        }
        ret = next;
    }
    return ret;
}

NODISCARD constexpr std::optional<int64_t> to_integer_i64(std::u16string_view str)
{
    if (str.empty()) {
        return std::nullopt;
    }
    bool negative = false;
    if (str.front() == char_consts::C_MINUS_SIGN) {
        negative = true;
        str.remove_prefix(1);
    }
    const std::optional<uint64_t> uret = to_integer_u64(str);
    if (!uret) {
        return std::nullopt;
    }

    constexpr const uint64_t max_int64 = ~uint64_t(0) >> 1;
    static_assert(max_int64 == std::numeric_limits<int64_t>::max());

    if (negative) {
        if (uret > max_int64 + 1) {
            return std::nullopt;
        }
        return static_cast<int64_t>(-uret.value());
    } else {
        if (uret > max_int64) {
            return std::nullopt;
        }
        return static_cast<int64_t>(uret.value());
    }
}

constexpr auto i64_min = std::numeric_limits<int64_t>::min();
constexpr auto i64_max = std::numeric_limits<int64_t>::max();
constexpr auto u64_max = std::numeric_limits<uint64_t>::max();
static_assert(18446744073709551615ull == u64_max);

static_assert(to_integer_i64(u"-9223372036854775809") == std::nullopt);
static_assert(to_integer_i64(u"-9223372036854775808") == i64_min);
static_assert(to_integer_i64(u"-1") == -1);
static_assert(to_integer_i64(u"0") == 0);
static_assert(to_integer_i64(u"1") == 1);
static_assert(to_integer_i64(u"9223372036854775807") == i64_max);
static_assert(to_integer_i64(u"9223372036854775808") == std::nullopt);

static_assert(to_integer_u64(u"0") == 0);
static_assert(to_integer_u64(u"1") == 1);
static_assert(to_integer_u64(u"1234567890") == 1234567890);
static_assert(to_integer_u64(u"12345678901234567890") == 12345678901234567890ull);
static_assert(to_integer_u64(u"18446744073709551615") == u64_max);
static_assert(to_integer_u64(u"18446744073709551616") == std::nullopt);
static_assert(to_integer_u64(u"36893488147419103231") == std::nullopt);
static_assert(to_integer_u64(u"92233720368547758079") == std::nullopt);
static_assert(to_integer_u64(u"110680464442257309695") == std::nullopt);

} // namespace detail
} // namespace

template<>
std::optional<uint64_t> to_integer<uint64_t>(std::u16string_view str)
{
    return detail::to_integer_u64(str);
}

template<>
std::optional<int64_t> to_integer<int64_t>(std::u16string_view str)
{
    return detail::to_integer_i64(str);
}
