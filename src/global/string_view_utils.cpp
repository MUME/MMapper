// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)

#include "string_view_utils.h"

#include "Consts.h"

#include <cstddef>

template<>
std::optional<uint64_t> to_integer<uint64_t>(std::u16string_view str)
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

template<>
std::optional<int64_t> to_integer<int64_t>(std::u16string_view str)
{
    if (str.empty()) {
        return std::nullopt;
    }
    bool negative = false;
    if (str.front() == char_consts::C_MINUS_SIGN) {
        negative = true;
        str.remove_prefix(1);
    }
    const auto opt_uret = to_integer<uint64_t>(str);
    if (!opt_uret.has_value()) {
        return std::nullopt;
    }

    // max int64_t
    constexpr const uint64_t max_int64 = ~uint64_t(0) >> 1;
    constexpr const auto max_neg = max_int64 + 1u;
    static_assert(max_neg == 1ull << 63);

    const auto uret = opt_uret.value();

    if (negative) {
        if (uret <= max_neg) {
            return static_cast<int64_t>(-uret);
        }
    } else {
        if (uret <= max_int64) {
            return static_cast<int64_t>(uret);
        }
    }
    return std::nullopt;
}
