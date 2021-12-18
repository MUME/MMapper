// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)

#include "string_view_utils.h"

#include <cstddef> // size_t

namespace std {

bool operator==(const std::u16string_view left, const std::string_view right) noexcept
{
    // we could use some Qt function, as for example QtStringView::compare(QLatin1String), but:
    // 1. we are reducing the dependencies on Qt
    // 2. it's difficult to find a non-allocating comparison between UTF-16 and Latin1 strings in Qt

    const size_t n = left.size();
    if (n != right.size()) {
        return false;
    }
    const char16_t *ldata = left.data();
    const char *rdata = right.data();
    for (size_t i = 0; i < n; i++) {
        // zero-extend possibly signed Latin1 char to unsigned char16_t
        if (ldata[i] != char16_t(static_cast<unsigned char>(rdata[i]))) {
            return false;
        }
    }
    return true;
}
} // namespace std

template<>
uint64_t to_integer<uint64_t>(std::u16string_view str, bool &ok)
{
    if (str.empty()) {
        ok = false;
        return 0;
    }
    constexpr const uint64_t max_div_10 = ~uint64_t(0) / 10;
    constexpr const uint max_mod_10 = static_cast<uint>(~uint64_t(0) % 10);
    uint64_t ret = 0;
    for (const char16_t ch : str) {
        if (ch >= '0' && ch <= '9') {
            const uint digit = static_cast<uint>(ch - '0');
            if (ret > max_div_10 || (ret == max_div_10 && digit > max_mod_10)) {
                // overflow
                ok = false;
                return 0;
            }
            ret = ret * 10 + digit;
        } else {
            ok = false;
            return ret;
        }
    }
    ok = true;
    return ret;
}

template<>
int64_t to_integer<int64_t>(std::u16string_view str, bool &ok)
{
    if (str.empty()) {
        ok = false;
        return 0;
    }
    bool negative = false;
    if (str[0] == '-') {
        negative = true;
        str.remove_prefix(1);
    }
    const uint64_t uret = to_integer<uint64_t>(str, ok);
    // max int64_t
    constexpr const uint64_t max_int64 = ~uint64_t(0) >> 1;
    int64_t ret;
    if (negative) {
        ok = ok && (uret <= max_int64 + 1);
        ret = static_cast<int64_t>(-uret);
    } else {
        ok = ok && (uret <= max_int64);
        ret = static_cast<int64_t>(uret);
    }
    return ret;
}
