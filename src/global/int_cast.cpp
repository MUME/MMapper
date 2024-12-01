// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "int_cast.h"

namespace { // anonymous
template<size_t bits>
NODISCARD constexpr uint64_t upow2() noexcept
{
    static_assert(sizeof(uint64_t) == 8);
    static_assert(bits >= 0 && bits <= 64);
    return 1ull << bits;
}

template<size_t bits>
NODISCARD constexpr uint64_t umask() noexcept
{
    return upow2<bits>() - 1ull;
}

template<size_t bits>
NODISCARD constexpr int64_t imask() noexcept
{
    static_assert(bits < 64);
    return static_cast<int64_t>(umask<bits>());
}

} // namespace

void test::test_int_cast()
{
    using namespace int_cast::exact;

    constexpr auto neg1 = -1LL;
    static_assert(can_cast<int8_t>(neg1));
    static_assert(can_cast<int16_t>(neg1));
    static_assert(can_cast<int32_t>(neg1));
    static_assert(can_cast<int64_t>(neg1));

    static_assert(!can_cast<uint8_t>(neg1));
    static_assert(!can_cast<uint16_t>(neg1));
    static_assert(!can_cast<uint32_t>(neg1));
    static_assert(!can_cast<uint64_t>(neg1));

    static_assert(can_cast<int8_t>(umask<7>()));
    static_assert(can_cast<int16_t>(umask<15>()));
    static_assert(can_cast<int32_t>(umask<31>()));
    static_assert(can_cast<int64_t>(umask<63>()));

    static_assert(can_cast<uint8_t>(imask<7>()));
    static_assert(can_cast<uint16_t>(imask<15>()));
    static_assert(can_cast<uint32_t>(imask<31>()));
    static_assert(can_cast<uint64_t>(imask<63>()));

    static_assert(!can_cast<int8_t>(upow2<7>()));
    static_assert(!can_cast<int16_t>(upow2<15>()));
    static_assert(!can_cast<int32_t>(upow2<31>()));
    static_assert(!can_cast<int64_t>(upow2<63>()));
}
