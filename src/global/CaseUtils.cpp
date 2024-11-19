// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "CaseUtils.h"

#include "Charset.h"

#include <cstdint>
#include <sstream>

// 0xD7u is multiplication sign.
// 0xF7u is division sign.
// 0xDF is lowercase sharp S; uppercase is unicode U+1E9E.
// 0xFF is lowercase y with two dots; uppercase is unicode U+0178.

NODISCARD static bool isToggleableUpperLatin1NonAscii(const char c)
{
    const auto uc = static_cast<uint8_t>(c);
    return uc >= 0xC0u && uc <= 0xDEu && uc != 0xD7u;
}

NODISCARD static bool isToggleableLowerLatin1NonAscii(const char c)
{
    const auto uc = static_cast<uint8_t>(c);
    return uc >= 0xE0u && uc <= 0xFEu && uc != 0xF7u;
}

char toLowerLatin1(const char c)
{
    const auto uc = static_cast<uint8_t>(c);
    if (isToggleableUpperLatin1NonAscii(c)) {
        // handle accented Latin-1 chars
        return static_cast<char>(uc + 0x20u);
    }
    return static_cast<char>(std::tolower(uc));
}

char toUpperLatin1(const char c)
{
    const auto uc = static_cast<uint8_t>(c);
    if (isToggleableLowerLatin1NonAscii(c)) {
        // handle accented Latin-1 chars
        return static_cast<char>(uc - 0x20u);
    }
    return static_cast<char>(std::toupper(uc));
}

NODISCARD bool isLowerLatin1(const char c)
{
    if (isToggleableLowerLatin1NonAscii(c)) {
        return true;
    }
    const auto uc = static_cast<uint8_t>(c);
    return std::islower(uc);
}

NODISCARD bool isUpperLatin1(const char c)
{
    if (isToggleableUpperLatin1NonAscii(c)) {
        return true;
    }
    const auto uc = static_cast<uint8_t>(c);
    return std::isupper(uc);
}

bool containsLowerLatin1(const std::string_view sv)
{
    for (char c : sv) {
        if (isLowerLatin1(c)) {
            return true;
        }
    }
    return false;
}

bool containsUpperLatin1(const std::string_view sv)
{
    for (char c : sv) {
        if (isUpperLatin1(c)) {
            return true;
        }
    }
    return false;
}

bool areEqualAsLowerLatin1(const std::string_view a, const std::string_view b)
{
    const size_t size = a.size();
    if (b.size() != size) {
        return false;
    }

    for (size_t i = 0; i < size; ++i) {
        if (toLowerLatin1(a[i]) != toLowerLatin1(b[i])) {
            return false;
        }
    }
    return true;
}

std::string toLowerLatin1(std::string str)
{
    if (containsUpperLatin1(str)) {
        for (char &c : str) {
            c = toLowerLatin1(c);
        }
    }
    return str;
}

std::string toUpperLatin1(std::string str)
{
    if (containsLowerLatin1(str)) {
        for (char &c : str) {
            c = toUpperLatin1(c);
        }
    }
    return str;
}

std::string toLowerLatin1(const std::string_view sv)
{
    return toLowerLatin1(std::string(sv));
}

std::string toUpperLatin1(const std::string_view sv)
{
    return toUpperLatin1(std::string(sv));
}

//////////////////////////////////

NODISCARD static char toChar(const char32_t codepoint)
{
    assert(codepoint < charset::charset_detail::NUM_LATIN1_CODEPOINTS);
    return static_cast<char>(static_cast<uint8_t>(codepoint));
}
NODISCARD static char32_t toCodepoint(const char c)
{
    return static_cast<uint8_t>(c);
}

NODISCARD char32_t toLowerUtf8(char32_t codepoint)
{
    if (codepoint < charset::charset_detail::NUM_LATIN1_CODEPOINTS) {
        return toCodepoint(toLowerLatin1(toChar(codepoint)));
    }
    return codepoint;
}

NODISCARD char32_t toUpperUtf8(char32_t codepoint)
{
    if (codepoint < charset::charset_detail::NUM_LATIN1_CODEPOINTS) {
        return toCodepoint(toUpperLatin1(toChar(codepoint)));
    }
    return codepoint;
}

NODISCARD bool isLowerUtf8(char32_t codepoint)
{
    if (codepoint < charset::charset_detail::NUM_LATIN1_CODEPOINTS) {
        return isLowerLatin1(toChar(codepoint));
    }
    return true;
}

NODISCARD bool isUpperUtf8(char32_t codepoint)
{
    if (codepoint < charset::charset_detail::NUM_LATIN1_CODEPOINTS) {
        return isUpperLatin1(toChar(codepoint));
    }
    return true;
}

NODISCARD bool containsLowerUtf8(std::string_view sv)
{
    if (isAscii(sv)) {
        return containsLowerLatin1(sv);
    }

    for (char32_t codepoint : charset::conversion::Utf8Iterable{sv}) {
        if (isLowerUtf8(codepoint)) {
            return true;
        }
    }
    return false;
}
NODISCARD bool containsUpperUtf8(std::string_view sv)
{
    if (isAscii(sv)) {
        return containsUpperLatin1(sv);
    }

    for (char32_t codepoint : charset::conversion::Utf8Iterable{sv}) {
        if (isUpperUtf8(codepoint)) {
            return true;
        }
    }
    return false;
}
NODISCARD bool areEqualAsLowerUtf8(const std::string_view a, const std::string_view b)
{
    if (a.size() != b.size()) {
        return false;
    }

    if (isAscii(a) && isAscii(b)) {
        return areEqualAsLowerLatin1(a, b);
    }

    charset::conversion::Utf8Iterable iterable_a{a};
    charset::conversion::Utf8Iterable iterable_b{b};

    auto ait = iterable_a.begin();
    const auto aend = iterable_a.end();
    auto bit = iterable_b.begin();
    const auto bend = iterable_b.end();

    while (true) {
        const auto aok = ait != aend;
        const auto bok = bit != bend;
        if (!aok || !bok) {
            return aok == bok;
        }

        const char32_t ac = toLowerUtf8(*ait);
        const char32_t bc = toLowerUtf8(*bit);
        if (ac != bc) {
            return false;
        }
        ++ait;
        ++bit;
    }
}

NODISCARD std::string toLowerUtf8(std::string str)
{
    if (!containsUpperUtf8(str)) {
        return str;
    }

    charset::conversion::Utf8StringBuilder result;
    for (const char32_t codepoint : charset::conversion::Utf8Iterable{str}) {
        result += toLowerUtf8(codepoint);
    }
    return result.steal_buffer();
}

NODISCARD std::string toUpperUtf8(std::string str)
{
    if (!containsLowerUtf8(str)) {
        return str;
    }

    charset::conversion::Utf8StringBuilder result;
    for (const char32_t codepoint : charset::conversion::Utf8Iterable{str}) {
        result += toUpperUtf8(codepoint);
    }
    return result.steal_buffer();
}

NODISCARD std::string toLowerUtf8(const std::string_view sv)
{
    return toLowerUtf8(std::string{sv});
}
NODISCARD std::string toUpperUtf8(const std::string_view sv)
{
    return toUpperUtf8(std::string{sv});
}
