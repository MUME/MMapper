#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "ConversionResult.h"
#include "cast_error.h"
#include "utils.h"

#include <cassert>
#include <climits>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace float_cast_detail {
// Does not require the same type, but requires same sign and value;
// avoiding testing the same type makes it simple to test integer
// constants without casting to a shorter type.
template<typename A, typename B>
NODISCARD constexpr bool isSameIntValue(const A a, const B b) noexcept
{
    static_assert(std::is_integral_v<A>);
    static_assert(std::is_integral_v<B>);
    static_assert(std::is_signed_v<A> == std::is_signed_v<B>);
    return a == b;
}

// This only exists because c++17 std::isnan() is not constexpr;
// using "f != f" feels like a hack.
template<typename FloatType>
NODISCARD constexpr bool isNan(const FloatType f) noexcept
{
#if __cplusplus >= 202000L
    return std::isnan(f);
#else
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#endif
    return f != f; // NOLINT (this is only true for NaNs)
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif
}

// this only exists because c++17 std::isfinite() is not constexpr
template<typename FloatType>
NODISCARD constexpr bool isFinite(const FloatType f) noexcept
{
#if __cplusplus >= 202000L
    return std::isfinite(f);
#else
    constexpr auto inf = std::numeric_limits<FloatType>::infinity();
    return !isNan(f) && -inf < f && f < inf;
#endif
}

template<typename FloatType, typename IntType>
struct FloatToSmallIntConversionLimits
{
    static_assert(std::is_integral_v<IntType>);
    static_assert(std::is_floating_point_v<FloatType>);
    static_assert(
        std::is_same_v<
            FloatType,
            float> || std::is_same_v<FloatType, double> || std::is_same_v<FloatType, long double>);
    static_assert(sizeof(IntType) <= sizeof(FloatType));
    static_assert(sizeof(IntType) * CHAR_BIT
                  <= std::numeric_limits<FloatType>::digits + std::is_signed_v<IntType>);

    static inline constexpr IntType maxIntValue = std::numeric_limits<IntType>::max();
    static inline constexpr FloatType maxFloatValue = static_cast<FloatType>(maxIntValue);
};

template<typename FloatType, typename IntType>
struct FloatToLargeIntConversionLimits
{
public:
    static_assert(std::is_integral_v<IntType>);
    static_assert(std::is_floating_point_v<FloatType>);
    static_assert(
        std::is_same_v<
            FloatType,
            float> || std::is_same_v<FloatType, double> || std::is_same_v<FloatType, long double>);

private:
    static inline constexpr auto trueIntMax = std::numeric_limits<IntType>::max();
    static inline constexpr size_t floatMantissaBits = std::numeric_limits<FloatType>::digits;
    static inline constexpr size_t intBits = sizeof(IntType) * CHAR_BIT - std::is_signed_v<IntType>;
    static_assert(intBits > floatMantissaBits);

private:
    static inline constexpr size_t shift = intBits - floatMantissaBits;
    static inline constexpr auto maskedValue = (trueIntMax >> shift) << shift;

public:
    static inline constexpr IntType maxIntValue = maskedValue;
    static inline constexpr FloatType maxFloatValue = static_cast<FloatType>(maxIntValue);

private:
    // roundtrip
    static_assert(static_cast<IntType>(maxFloatValue) == maxIntValue);

private:
    static_assert(maxIntValue < trueIntMax);
    static_assert(maxFloatValue < static_cast<FloatType>(trueIntMax));
};

template<typename FloatType, typename IntType>
struct FloatToIntConversionLimits
    : std::conditional_t<(sizeof(IntType) < sizeof(FloatType)),
                         float_cast_detail::FloatToSmallIntConversionLimits<FloatType, IntType>,
                         float_cast_detail::FloatToLargeIntConversionLimits<FloatType, IntType>>
{};

} // namespace float_cast_detail
namespace float_cast {

template<typename IntType, typename FloatType>
NODISCARD constexpr bool can_cast_float_to_int(const FloatType floatValue) noexcept
{
    return float_cast_detail::isFinite<FloatType>(floatValue)
           && floatValue >= std::numeric_limits<IntType>::min()
           && floatValue
                  <= float_cast_detail::FloatToIntConversionLimits<FloatType, IntType>::maxFloatValue;
}

template<typename IntType, typename FloatType>
NODISCARD constexpr bool can_cast_float_to_int_roundtrip(const FloatType floatValue) noexcept
{
    if (!can_cast_float_to_int<IntType, FloatType>(floatValue)) {
        return false;
    }
    const auto intValue = static_cast<IntType>(floatValue);
    return utils::isSameFloat<FloatType>(floatValue, static_cast<FloatType>(intValue));
}

template<typename FloatType, typename IntType>
NODISCARD constexpr bool can_cast_int_to_float_roundtrip(const IntType intValue) noexcept
{
    const auto floatValue = static_cast<FloatType>(intValue);
    return can_cast_float_to_int_roundtrip<IntType, FloatType>(floatValue)
           && float_cast_detail::isSameIntValue(static_cast<IntType>(floatValue), intValue);
}

template<typename IntType, typename FloatType>
NODISCARD constexpr ConversionResult<IntType> convert_float_to_int(const FloatType floatValue) noexcept
{
    if (!float_cast_detail::isFinite<FloatType>(floatValue)) {
        return CastErrorEnum::NotFinite;
    }
    if (floatValue < std::numeric_limits<IntType>::min()) {
        return CastErrorEnum::TooSmall;
    }
    if (floatValue
        > float_cast_detail::FloatToIntConversionLimits<FloatType, IntType>::maxFloatValue) {
        return CastErrorEnum::TooBig;
    }
    return ConversionResult<IntType>{static_cast<IntType>(floatValue)};
}

template<typename IntType, typename FloatType>
NODISCARD constexpr ConversionResult<IntType> convert_float_to_int_roundtrip(
    const FloatType floatValue) noexcept
{
    auto result = convert_float_to_int<IntType, FloatType>(floatValue);
    if (result
        && !utils::isSameFloat<FloatType>(floatValue, static_cast<FloatType>(result.get_value()))) {
        return CastErrorEnum ::RoundTripFailure;
    }
    return result;
}

template<typename FloatType, typename IntType>
NODISCARD constexpr ConversionResult<FloatType> convert_int_to_float_roundtrip(
    const IntType intValue) noexcept
{
    if (!can_cast_int_to_float_roundtrip<FloatType, IntType>(intValue)) {
        // safe, even if it truncates mantissa bits.
        const auto floatValue = static_cast<FloatType>(intValue);
        auto tmp = convert_float_to_int<IntType, FloatType>(floatValue); // might throw
        if (!tmp) {
            return tmp.get_error();
        }
        // we know that the result is not valid.
        return CastErrorEnum::RoundTripFailure;
    }
    return ConversionResult<FloatType>{static_cast<FloatType>(intValue)};
}

template<typename IntType, typename FloatType>
NODISCARD IntType checked_cast_float_to_int(const FloatType floatValue)
{
    const auto result = convert_float_to_int<IntType, FloatType>(floatValue);
    if (!result) {
        throw CastErrorException{result.get_error()};
    }
    return result.get_value();
}

template<typename IntType, typename FloatType>
NODISCARD IntType checked_cast_float_to_int_roundtrip(const FloatType floatValue)
{
    const auto result = convert_float_to_int_roundtrip<IntType, FloatType>(floatValue);
    if (!result) {
        throw CastErrorException{result.get_error()};
    }
    return result.get_value();
}

template<typename FloatType, typename IntType>
NODISCARD FloatType checked_cast_int_to_float_roundtrip(const IntType intValue)
{
    const auto result = convert_int_to_float_roundtrip<FloatType, IntType>(intValue);
    if (!result) {
        throw CastErrorException{result.get_error()};
    }
    return result.get_value();
}

} // namespace float_cast

namespace test {
extern void test_float_cast();
} // namespace test
