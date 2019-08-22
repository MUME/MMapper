#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstdint>

#define BIT32_FOREACH(x) \
    x(1) x(2) x(3) x(4) x(5) x(6) x(7) x(8) x(9) x(10) x(11) x(12) x(13) x(14) x(15) x(16) x(17) \
        x(18) x(19) x(20) x(21) x(22) x(23) x(24) x(25) x(26) x(27) x(28) x(29) x(30) x(31) x(32)

/* TODO: eliminate bitxx constants.
 * They're unnecessary, and they're also confusing because they're 1-based. */
#define DEFINE_BITS(x) static constexpr const uint32_t bit##x = (1u << ((x) -1));
BIT32_FOREACH(DEFINE_BITS)
#define ADD(x) +1
static_assert(BIT32_FOREACH(ADD) == 32);
static_assert(bit1 == 1u);
static_assert(bit32 == 2147483648u);
#undef ADD
#undef DEFINE_BITS
