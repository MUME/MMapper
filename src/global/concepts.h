#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include <concepts>
#include <string_view>

#define MM_TYPE_IDENTITY(_x) std::type_identity_t<_x>

namespace concepts {

template<typename T>
concept IsTriviallyCopyable = std::is_trivially_copyable_v<T>;

namespace detail {

namespace cpp11 {

template<typename T>
struct CharacterType : std::false_type
{};
template<typename T>
struct ArithmeticIntegral : std::false_type
{};

#define XFOREACH_CHAR_TYPES(X) \
    X(char) \
    X(char8_t) \
    X(char16_t) \
    X(char32_t)

#define X_DEFINE(_type) \
    template<> \
    struct CharacterType<_type> : std::true_type \
    {};
XFOREACH_CHAR_TYPES(X_DEFINE)
#undef X_DEFINE

#define XFOREACH_ARITH_INT_TYPES(X) \
    X(signed char) \
    X(signed short int) \
    X(signed int) \
    X(signed long int) \
    X(signed long long) \
    X(unsigned char) \
    X(unsigned short int) \
    X(unsigned int) \
    X(unsigned long int) \
    X(unsigned long long)

#define X_DEFINE(_type) \
    template<> \
    struct ArithmeticIntegral<_type> : std::true_type \
    {};
XFOREACH_ARITH_INT_TYPES(X_DEFINE)
#undef X_DEFINE
} // namespace cpp11

// X_v<T> and X_t<T> templates as shortcuts for X<T>::value and X<T>::type.
namespace cpp17 {
template<typename T>
inline constexpr bool IsCharacterType_v = cpp11::CharacterType<T>::value;
template<typename T>
inline constexpr bool IsIntegralNumeric_v = cpp11::ArithmeticIntegral<T>::value;
} // namespace cpp17

} // namespace detail

template<typename T>
concept IsBoolean = std::is_same_v<bool, T>;

template<typename T>
concept IsCharacter = detail::cpp17::IsCharacterType_v<T>;

// caution: this differs from std::integral because we purposely exclude boolean and character types.
template<typename T>
concept IsIntegralNumeric = detail::cpp17::IsIntegralNumeric_v<T>;

template<typename T>
concept IsUnsignedIntegralNumeric = IsIntegralNumeric<T> and std::is_unsigned_v<T>;

template<typename T>
concept IsSignedIntegralNumeric = IsIntegralNumeric<T> and std::is_signed_v<T>;

template<typename T>
concept IsEnum = std::is_enum_v<T>;

template<typename T>
concept IsUnsignedEnum = IsEnum<T> and IsUnsignedIntegralNumeric<std::underlying_type_t<T>>;

template<typename T>
concept IsSignedEnum = IsEnum<T> and IsSignedIntegralNumeric<std::underlying_type_t<T>>;

template<typename T>
concept IsEnumRef = IsEnum<std::remove_reference_t<T>>;

template<typename T>
concept IsEnumOrEnumRef = IsEnum<T> or IsEnumRef<T>;

template<typename T>
concept IsFloatingPoint = std::floating_point<T>;

template<typename T>
concept IsNumeric = IsIntegralNumeric<T> or IsFloatingPoint<T>;

template<typename T>
concept IsPointer = std::is_pointer_v<T>;

template<typename T>
concept IsValue = std::is_same_v<T, std::decay_t<T>>;

template<typename T>
concept IsLValueRef = std::is_lvalue_reference_v<T>;

template<typename T>
concept IsConstLValueRef = IsLValueRef<T> and std::is_const_v<std::remove_reference_t<T>>;

template<typename T>
concept IsValueOrConstLValueRef = IsValue<T> or IsConstLValueRef<T>;

} // namespace concepts
