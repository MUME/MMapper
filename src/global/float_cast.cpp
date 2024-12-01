// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "float_cast.h"

void test::test_float_cast()
{
    std::ignore = static_cast<uint64_t>(0x1.FFFFFFFFFFFFFp+63);

    using namespace float_cast;

    std::ignore = checked_cast_float_to_int<int>(1.5);
    try {
        std::ignore = checked_cast_float_to_int_roundtrip<int>(1.5);
        throw std::runtime_error("oops");
    } catch (const CastErrorException &ex) {
        assert(ex.err == CastErrorEnum::RoundTripFailure);
    }

    try {
        std::ignore = checked_cast_float_to_int<uint64_t>(0x1p64);
        throw std::runtime_error("oops");
    } catch (const CastErrorException &ex) {
        assert(ex.err == CastErrorEnum::TooBig);
    }

    try {
        std::ignore = checked_cast_int_to_float_roundtrip<double>(~0ull);
        throw std::runtime_error("oops");
    } catch (const CastErrorException &ex) {
        assert(ex.err == CastErrorEnum::TooBig);
    }

    try {
        std::ignore = checked_cast_int_to_float_roundtrip<double>(0x7FFF'FFFF'FFFF'FFFFull);
        throw std::runtime_error("oops");
    } catch (const CastErrorException &ex) {
        assert(ex.err == CastErrorEnum::RoundTripFailure);
    }

    {
        auto n = 0x7FFF'FFFF'FFFF'F000ull;
        auto f = checked_cast_int_to_float_roundtrip<double>(n);
        if (n != static_cast<decltype(n)>(f)) {
            std::abort();
        }
    }
}

namespace { // anonymous
MAYBE_UNUSED constexpr void cxpr_test_detail()
{
    using namespace float_cast_detail;
    // negative zero isn't differentiated.
    static_assert(utils::isSameFloat(0.0, -0.0));

    using D = std::numeric_limits<double>;

    // nan does not compare equal to itself
    static_assert(!utils::isSameFloat(D::quiet_NaN(), D::quiet_NaN()));

    static_assert(!isNan(0.0));
    static_assert(!isNan(D::infinity()));
    static_assert(isNan(D::quiet_NaN()));
    static_assert(isNan(D::signaling_NaN()));
}

MAYBE_UNUSED constexpr void cxpr_test_convert_rountrip()
{
    using namespace float_cast;
    static_assert(convert_int_to_float_roundtrip<double>(0ull));
    static_assert(!convert_int_to_float_roundtrip<double>(~0ull));
}

MAYBE_UNUSED constexpr void cxpr_test_can_cast()
{
    using namespace float_cast;

    static_assert(can_cast_float_to_int<uint64_t>(1.5));
    static_assert(!can_cast_float_to_int_roundtrip<uint64_t>(1.5));
    static_assert(can_cast_float_to_int_roundtrip<uint64_t>(1.0));

    using Err = CastErrorEnum;
#define CONV(x, y) convert_float_to_int_roundtrip<x>(y)
    static_assert(!CONV(uint64_t, 1.5));
    static_assert(CONV(uint64_t, 1.5) == Err::RoundTripFailure);

    static_assert(CONV(uint64_t, 0x1p63).is_valid());
    static_assert(CONV(uint64_t, 0x1p63) == uint64_t(1) << 63);

    static_assert(CONV(uint64_t, 0x1p64) == Err::TooBig);
    static_assert(CONV(uint64_t, -0x1p63) == Err::TooSmall);

    using D = std::numeric_limits<double>;
    static_assert(CONV(uint64_t, -D::infinity()) == Err::NotFinite);
    static_assert(CONV(uint64_t, D::infinity()) == Err::NotFinite);
    static_assert(CONV(uint64_t, D::quiet_NaN()) == Err::NotFinite);
    static_assert(CONV(uint64_t, D::signaling_NaN()) == Err::NotFinite);
#undef CONV

    static_assert(can_cast_int_to_float_roundtrip<double>(1ull));
    static_assert(!can_cast_int_to_float_roundtrip<double>(~0ull));
}

} // namespace
