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
