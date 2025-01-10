// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "Charset.h"

#include "Consts.h"
#include "TextUtils.h"
#include "parserutils.h"

#include <array>
#include <ostream>
#include <sstream>

namespace { // anonymous
namespace detail {

static constexpr const size_t IDX_NBSP = 160;
static constexpr const char LATIN1_UNDEFINED = 'z';
static constexpr const size_t NUM_ASCII_CODEPOINTS = 128;
static constexpr const size_t NUM_LATIN1_CODEPOINTS = 256;
// Taken from MUME's HELP LATIN to convert from Latin-1 to US-ASCII
static constexpr const std::array g_latin1ToAscii = {
    /*160*/
    ' ', '!', 'c', 'L', '$',  'Y', '|', 'P', '"', 'C', 'a', '<', ',', '-', 'R', '-',
    'd', '+', '2', '3', '\'', 'u', 'P', '*', ',', '1', 'o', '>', '4', '2', '3', '?',
    'A', 'A', 'A', 'A', 'A',  'A', 'A', 'C', 'E', 'E', 'E', 'E', 'I', 'I', 'I', 'I',
    'D', 'N', 'O', 'O', 'O',  'O', 'O', '*', 'O', 'U', 'U', 'U', 'U', 'Y', 'T', 's',
    'a', 'a', 'a', 'a', 'a',  'a', 'a', 'c', 'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
    'd', 'n', 'o', 'o', 'o',  'o', 'o', '/', 'o', 'u', 'u', 'u', 'u', 'y', 't', 'y'};

static_assert(
    std::is_same_v<char, std::remove_const_t<std::remove_reference_t<decltype(g_latin1ToAscii[0])>>>);
static_assert(std::size(g_latin1ToAscii) == NUM_LATIN1_CODEPOINTS - IDX_NBSP);

NODISCARD static inline constexpr size_t getIndex(const char c) noexcept
{
    return static_cast<size_t>(static_cast<uint8_t>(c));
}

NODISCARD static inline constexpr bool isAscii(const char c) noexcept
{
    return getIndex(c) < NUM_ASCII_CODEPOINTS;
}

NODISCARD static inline constexpr char latin1ToAscii(const char c) noexcept
{
    if (isAscii(c)) {
        return c;
    }

    const auto i = getIndex(c);
    if (i >= IDX_NBSP && i < NUM_LATIN1_CODEPOINTS) {
        return g_latin1ToAscii[static_cast<size_t>(i - IDX_NBSP)];
    } else {
        return LATIN1_UNDEFINED;
    }
}

static_assert(latin1ToAscii('X') == 'X');
static_assert(latin1ToAscii('x') == 'x');
static_assert(latin1ToAscii(char_consts::C_DELETE) == char_consts::C_DELETE);
static_assert(latin1ToAscii("\x80"[0]) == LATIN1_UNDEFINED);
static_assert(latin1ToAscii("\x9f"[0]) == LATIN1_UNDEFINED);
static_assert(latin1ToAscii(char_consts::C_NBSP) == char_consts::C_SPACE);
static_assert(latin1ToAscii("\xff"[0]) == 'y');

} // namespace detail
} // namespace

bool isAscii(const char c) noexcept
{
    return detail::isAscii(c);
}

char latin1ToAscii(const char c) noexcept
{
    return detail::latin1ToAscii(c);
}

bool isAscii(const std::string_view sv)
{
    for (const char c : sv) {
        if (!isAscii(c)) {
            return false;
        }
    }
    return true;
}

std::string &latin1ToAsciiInPlace(std::string &str)
{
    for (char &c : str) {
        if (!isAscii(c)) {
            c = latin1ToAscii(c);
        }
    }
    return str;
}

namespace mmqt {

QString &toLatin1InPlace(QString &str)
{
    constexpr uint16_t UNICODE_LSQUO = 0x2018;
    constexpr uint16_t UNICODE_RSQUO = 0x2019;
    constexpr uint16_t UNICODE_LDQUO = 0x201c;
    constexpr uint16_t UNICODE_RDQUO = 0x201d;

    for (QChar &qc : str) {
        switch (qc.unicode()) {
        case UNICODE_LSQUO:
        case UNICODE_RSQUO:
            qc = QC_SQUOTE;
            break;
        case UNICODE_LDQUO:
        case UNICODE_RDQUO:
            qc = QC_DQUOTE;
            break;
        }
    }

    return str;
}

QString &toAsciiInPlace(QString &str)
{
    toLatin1InPlace(str);

    // NOTE: 128 (0x80) was not converted to 'z' before.
    for (QChar &qc : str) {
        // c++17 if statement with initializer
        if (const char ch = qc.toLatin1(); !isAscii(ch)) {
            qc = latin1ToAscii(ch);
        }
    }
    return str;
}

} // namespace mmqt

std::string latin1ToAscii(const std::string_view sv)
{
    std::string tmp{sv};
    latin1ToAsciiInPlace(tmp);
    return tmp;
}

// REVISIT: move to global/Charset?
void latin1ToAscii(std::ostream &os, const std::string_view sv)
{
    for (char c : sv) {
        if (!isAscii(c)) {
            c = latin1ToAscii(c);
        }
        os << c;
    }
}

bool isPrintLatin1(const char c)
{
    const auto uc = static_cast<uint8_t>(c);
    return uc >= ((uc < 0x7f) ? 0x20 : 0xa0);
}

void latin1ToUtf8(std::ostream &os, const char c)
{
    const auto uc = static_cast<uint8_t>(c);
    // U+0000 to U+007F: 0xxxxxxx (7 bits)
    if (uc < 0x80u) {
        os << c;
        return;
    }

    // U+0080 .. U+07FF: 110xxxxx  10xxxxxx (11 bits)
    //
    // but we only care about a smaller subset:
    // U+0080 .. U+00FF: 1100001x  10xxxxxx (7 bits)
    //                    c2..c3    80..bf  (hex)
    //
    // 0x80 becomes         c2        80
    // 0xff becomes         c3        bf
    static constexpr const auto SIX_BIT_MASK = (1u << 6u) - 1u;
    static_assert(SIX_BIT_MASK == 63);
    char buf[3];
    buf[0] = char(0xc0u | (uc >> 6u));          // c2..c3
    buf[1] = char(0x80u | (uc & SIX_BIT_MASK)); // 80..bf
    buf[2] = char_consts::C_NUL;
    os.write(buf, 2);
}

void latin1ToUtf8(std::ostream &os, std::string_view sv)
{
    if constexpr (false) {
        if (isAscii(sv)) {
            os << sv;
            return;
        }
        for (const char c : sv) {
            latin1ToUtf8(os, c);
        }
    } else {
        const auto asc = static_cast<bool (*)(char)>(isAscii);
        while (!sv.empty()) {
            const auto beg = sv.begin();
            const auto end = sv.end();
            const auto first_non_ascii = std::find_if_not(beg, end, asc);
            if (first_non_ascii != beg) {
                // ascii
                const auto len = static_cast<size_t>(first_non_ascii - beg);
                os << sv.substr(0, len);
                sv.remove_prefix(len);
            } else {
                // non-ascii
                assert(first_non_ascii == beg);
                assert(first_non_ascii != end);
                const auto it = std::find_if(std::next(first_non_ascii), end, asc);
                const auto len = static_cast<size_t>(it - beg);
                for (char c : sv.substr(0, len)) {
                    latin1ToUtf8(os, c);
                }
                sv.remove_prefix(len);
            }
        }
    }
}

void convertFromLatin1(std::ostream &os,
                       const CharacterEncodingEnum encoding,
                       const std::string_view sv)
{
    switch (encoding) {
    case CharacterEncodingEnum::ASCII:
        latin1ToAscii(os, sv);
        return;
    case CharacterEncodingEnum::LATIN1:
        os << sv;
        return;
    case CharacterEncodingEnum::UTF8:
        latin1ToUtf8(os, sv);
        return;
    }

    std::abort();
}

void charset::utf8ToAscii(std::ostream &os, const std::string_view sv)
{
    // REVISIT: This can be made a lot more efficient, but it's not likely to be called.
    QString s = mmqt::toQStringUtf8(sv);
    auto tmp = mmqt::toStdStringLatin1(s);
    latin1ToAsciiInPlace(tmp);
    os << tmp;
}

void charset::utf8ToLatin1(std::ostream &os, const std::string_view sv)
{
    // REVISIT: This can be made a lot more efficient, but it's not likely to be called.
    QString s = mmqt::toQStringUtf8(sv);
    os << mmqt::toStdStringLatin1(s);
}

std::string latin1ToUtf8(const std::string_view sv)
{
    if (isAscii(sv)) {
        return std::string{sv};
    }

    std::ostringstream oss;
    ::latin1ToUtf8(oss, sv);
    return std::move(oss).str();
}

std::string charset::utf8ToLatin1(const std::string_view sv)
{
    if (isAscii(sv)) {
        return std::string{sv};
    }

    std::ostringstream oss;
    utf8ToLatin1(oss, sv);
    return std::move(oss).str();
}

void charset::convert(std::ostream &os,
                      std::string_view sv,
                      const CharacterEncodingEnum from,
                      const CharacterEncodingEnum to)
{
    switch (to) {
    case CharacterEncodingEnum::LATIN1:
        switch (from) {
        case CharacterEncodingEnum::ASCII:
        case CharacterEncodingEnum::LATIN1:
            os << sv;
            break;
        case CharacterEncodingEnum::UTF8:
            utf8ToLatin1(os, sv);
            break;
        }
        break;
    case CharacterEncodingEnum::UTF8:
        switch (from) {
        case CharacterEncodingEnum::ASCII:
        case CharacterEncodingEnum::LATIN1:
            latin1ToUtf8(os, sv);
            break;
        case CharacterEncodingEnum::UTF8:
            os << sv;
            break;
        }
        break;
    case CharacterEncodingEnum::ASCII:
        switch (from) {
        case CharacterEncodingEnum::ASCII:
        case CharacterEncodingEnum::LATIN1:
            latin1ToAscii(os, sv);
            break;
        case CharacterEncodingEnum::UTF8:
            utf8ToAscii(os, sv);
            break;
        }
        break;
    }
}

namespace { // anonymous

namespace detail {
// This is a lenient tester; it just recognizes the format but doesn't check actual codepoints.
// The goal is to detect accidentally passing latin1 to utf8.
NODISCARD static constexpr bool isProbablyUtf8(const std::string_view sv)
{
    constexpr uint8_t allZeros = 0;
    constexpr uint8_t topBit = 0b1000'0000;
    constexpr uint8_t top2Bits = 0b1100'0000;
    constexpr uint8_t top3Bits = 0b1110'0000;
    constexpr uint8_t top4Bits = 0b1111'0000;
    constexpr uint8_t top5Bits = 0b1111'1000;
    constexpr uint8_t top6Bits = 0b1111'1100;
    constexpr uint8_t top7Bits = 0b1111'1110;
    static_assert(topBit == 0x80);
    static_assert(top2Bits == 0xc0);
    static_assert(top3Bits == 0xe0);
    static_assert(top4Bits == 0xf0);
    static_assert(top5Bits == 0xf8);
    static_assert(top6Bits == 0xfc);
    static_assert(top7Bits == 0xfe);

    size_t multiByteExpected = 0;
    for (const char c : sv) {
        const auto uc = static_cast<uint8_t>(c);
        if (multiByteExpected) {
            if ((uc & top2Bits) != topBit) {
                return false;
            }
            // 10xxxxxx
            --multiByteExpected;
            continue;
        }

        if ((uc & topBit) == allZeros) {
            // U+0000 to U+007F
            // 0xxxxxxx
            continue;
        }

        if ((uc & top2Bits) == topBit) {
            return false;
        }

        if ((uc & top3Bits) == top2Bits) {
            // U+0080 to U+07FF
            // 110xxxxx 10xxxxxx
            //
            // c?
            // d?
            //
            // note: technically uc == c0 ||  uc == c1 are considered invalid because
            // they are 2-byte encodings of 7-bit ascii codepoints.

            if ((uc & top7Bits) == top2Bits) {
                return false;
            }

            multiByteExpected = 1;
            continue;
        }

        if ((uc & top4Bits) == top3Bits) {
            // U+0800 to U+FFFF
            // 1110xxxx 10xxxxxx 10xxxxxx
            multiByteExpected = 2;
            continue;
        }

        if ((uc & top5Bits) == top4Bits) {
            // U+10000 to U+10FFFF
            // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

            if ((uc & 0x7u) == 0x4u) {
                // These may or may not be invalid,
                // but we'd have to check the next byte to know.
                // Note: 0xF4 is used for U+100000 to U+13FFFF,
                // but only code points up to U+10FFFF are valid.
            }

            if ((uc & 0x7u) >= 0x5u) {
                // U+140000 and beyond
                return false;
            }

            multiByteExpected = 3;
            continue;
        }

        // As of March 2021, there are no valid 5 or 6-byte utf8 values.
        return false;
    }
    return !multiByteExpected;
}

static_assert(isProbablyUtf8("\x00")); // U+0000 (ascii 0x00)
static_assert(isProbablyUtf8("\x7f")); // U+007f (ascii 0x7f)
//
static_assert(!isProbablyUtf8("\x80"));
static_assert(!isProbablyUtf8("\xbf"));
//
static_assert(!isProbablyUtf8("\xc0\x80")); // invalid over-long encoding
static_assert(!isProbablyUtf8("\xc1\x80")); // invalid over-long encoding
//
static_assert(isProbablyUtf8("\xc2\x80")); // U+  80 (latin1 0x80)
static_assert(isProbablyUtf8("\xc2\xbf")); // U+  BF (latin1 0xbf)
static_assert(isProbablyUtf8("\xc3\x80")); // U+  C0 (latin1 0xc0)
static_assert(isProbablyUtf8("\xc3\xbf")); // U+  FF (latin1 0xff)
static_assert(isProbablyUtf8("\xc4\x80")); // U+0100
static_assert(isProbablyUtf8("\xdf\x80")); // U+07C0
static_assert(isProbablyUtf8("\xdf\xbf")); // U+07FF
//
static_assert(isProbablyUtf8("\xe0\x80\x80")); // U+0800 (* not all in this range are valid)
static_assert(isProbablyUtf8("\xe1\x80\x80")); // U+1000
static_assert(isProbablyUtf8("\xec\x80\x80")); // U+C000
static_assert(isProbablyUtf8("\xed\x80\x80")); // U+D000 (* not all in this range are valid)
static_assert(isProbablyUtf8("\xee\x80\x80")); // U+E000
static_assert(isProbablyUtf8("\xef\x80\x80")); // U+F000
static_assert(isProbablyUtf8("\xef\xbf\xbf")); // U+FFFF
//
static_assert(isProbablyUtf8("\xf0\x80\x80\x80")); // U+ 10000 (* not all in this range are valid)
static_assert(isProbablyUtf8("\xf1\x80\x80\x80")); // U+ 40000
static_assert(isProbablyUtf8("\xf2\x80\x80\x80")); // U+ 80000
static_assert(isProbablyUtf8("\xf3\x80\x80\x80")); // U+ C0000
static_assert(isProbablyUtf8("\xf4\x80\x80\x80")); // U+100000 (* not all in this range are valid)
//
static_assert(!isProbablyUtf8("\xf5\x80\x80\x80")); // U+140000
static_assert(!isProbablyUtf8("\xf6\x80\x80\x80")); // U+180000
static_assert(!isProbablyUtf8("\xf7\x80\x80\x80")); // U+1C0000
//
static_assert(!isProbablyUtf8("\xf8\x80\x80\x80\x80"));     // illegal 5-byte sequence
static_assert(!isProbablyUtf8("\xfc\x80\x80\x80\x80\x80")); // illegal 6-byte sequence
static_assert(!isProbablyUtf8("\xfe"));                     // not valid anywhere
static_assert(!isProbablyUtf8("\xff"));                     // not valid anywhere

} // namespace detail
} // namespace

namespace charset {
// This is a lenient tester; it just recognizes the format but doesn't check actual codepoints.
// The goal is to detect accidentally passing latin1 to utf8.
NODISCARD bool isProbablyUtf8(const std::string_view sv)
{
    return detail::isProbablyUtf8(sv);
}

} // namespace charset
