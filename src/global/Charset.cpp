// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "Charset.h"

#include "ConfigConsts.h"
#include "Consts.h"
#include "TextUtils.h"
#include "entities.h"
#include "parserutils.h"
#include "string_view_utils.h"
#include "tests.h"

#include <array>
#include <ostream>
#include <sstream>

#include <QDebug>

using namespace char_consts;
using namespace string_consts;

#define XSEMI() ;

// clang-format off

// refer to:
// https://en.wikipedia.org/wiki/ASCII
// https://en.wikipedia.org/wiki/Latin-1
// https://en.wikipedia.org/wiki/UTF-8
// https://en.wikipedia.org/wiki/Windows-1252

// X(_win, _unicode, _name)
#define XFOREACH_WINDOWS_125x(X, XSEP) \
    X(0x82u, 0x201Au, "sbquo")  XSEP() \
    X(0x84u, 0x201Eu, "bdquo")  XSEP() \
    X(0x8Bu, 0x2039u, "lsaquo") XSEP() \
    X(0x91u, 0x2018u, "lsquo")  XSEP() \
    X(0x92u, 0x2019u, "rsquo")  XSEP() \
    X(0x93u, 0x201Cu, "ldquo")  XSEP() \
    X(0x94u, 0x201Du, "rdquo")  XSEP() \
    X(0x95u, 0x2022u, "bull")   XSEP() \
    X(0x96u, 0x2013u, "ndash")  XSEP() \
    X(0x97u, 0x2014u, "mdash")  XSEP() \
    X(0x9Bu, 0x203Au, "rsaquo")

// note: "iconv -t LATIN1//TRANSLIT" converts 0x201B to C_SQUOTE instead of C_BACK_TICK.

// X(_unicode, _ascii, _name)
#define XFOREACH_UNICODE_TRANSLIT(X, XSEP) \
    X(0x2013u, C_MINUS_SIGN,   "ndash")  XSEP() \
    X(0x2014u, C_MINUS_SIGN,   "mdash")  XSEP() \
    X(0x2018u, C_SQUOTE,       "lsquo")  XSEP() \
    X(0x2019u, C_SQUOTE,       "rsquo")  XSEP() \
    X(0x201Au, C_SQUOTE,       "sbquo")  XSEP() \
    X(0x201Bu, C_SQUOTE,       "single high reversed quotation mark") XSEP() \
    X(0x201Cu, C_DQUOTE,       "ldquo")  XSEP() \
    X(0x201Du, C_DQUOTE,       "rdquo")  XSEP() \
    X(0x201Eu, C_DQUOTE,       "bdquo")  XSEP() \
    X(0x201Fu, C_DQUOTE,       "double high reversed quotation mark") XSEP() \
    X(0x2022u, C_ASTERISK,     "bull")   XSEP() \
    X(0x2039u, C_LESS_THAN,    "lsaquo") XSEP() \
    X(0x203Au, C_GREATER_THAN, "rsaquo")

// note: left and right double angle quotes are part of Latin1 (0xAB and 0xBB),
// and they're converted to C_OPEN_ANGLE and C_CLOSE_ANGLE.

// clang-format on

namespace { // anonymous

template<typename T>
NODISCARD constexpr bool is_latin_control(const T uc)
{
    return std::is_unsigned_v<T>           //
           && (uc >= 0x80u && uc < 0xA0u); // Latin1 control character
}
template<typename T>
NODISCARD constexpr bool is_noncontrol_ascii(const T ascii)
{
    // non-control ASCII
    return std::is_same_v<char, T>                                                 //
           && (static_cast<uint8_t>(ascii) & 0x7Fu) == static_cast<uint8_t>(ascii) //
           && static_cast<uint8_t>(ascii) >= 0x20u;                                //
}
template<typename T>
NODISCARD constexpr bool is_16bit_unicode(const T uc)
{
    return std::is_unsigned_v<T>   //
           && (uc & 0xFFFFu) == uc // 16-bit unicode
           && !charset::utf16_detail::is_utf16_surrogate(static_cast<char16_t>(uc));
}

#define XCASE(_win, _unicode, _name) \
    static_assert(is_latin_control(_win)); \
    static_assert(is_16bit_unicode(_unicode))
XFOREACH_WINDOWS_125x(XCASE, XSEMI);
#undef XCASE

#define XCASE(_unicode, _ascii, _name) \
    static_assert(is_noncontrol_ascii(_ascii)); \
    static_assert(is_16bit_unicode(_unicode))
XFOREACH_UNICODE_TRANSLIT(XCASE, XSEMI);
#undef XCASE

} // namespace

namespace charset {
namespace charset_detail {

static_assert(NUM_ASCII_CODEPOINTS == 128);
static_assert(NUM_LATIN1_CODEPOINTS == 256);
static constexpr const size_t IDX_NBSP = 160;
static_assert(IDX_NBSP == charset::conversion::to_char16(C_NBSP));

static inline constexpr const char LATIN1_CONTROL_CODE_REPLACEMENT = 'z';
// Taken from MUME's HELP LATIN to convert from Latin-1 to US-ASCII
constexpr const std::array g_latin1ToAscii = {/*160*/
                                              ' ',  '!', 'c', 'L', '$', 'Y', '|', 'P', '"', 'C',
                                              'a',  '<', ',', '-', 'R', '-', 'd', '+', '2', '3',
                                              '\'', 'u', 'P', '*', ',', '1', 'o', '>', '4', '2',
                                              '3',  '?', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'C',
                                              'E',  'E', 'E', 'E', 'I', 'I', 'I', 'I', 'D', 'N',
                                              'O',  'O', 'O', 'O', 'O', '*', 'O', 'U', 'U', 'U',
                                              'U',  'Y', 'T', 's', 'a', 'a', 'a', 'a', 'a', 'a',
                                              'a',  'c', 'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
                                              'd',  'n', 'o', 'o', 'o', 'o', 'o', '/', 'o', 'u',
                                              'u',  'u', 'u', 'y', 't', 'y'};

static_assert(
    std::is_same_v<char, std::remove_const_t<std::remove_reference_t<decltype(g_latin1ToAscii[0])>>>);
static_assert(g_latin1ToAscii.size() == NUM_LATIN1_CODEPOINTS - IDX_NBSP);
static_assert(g_latin1ToAscii.front() == C_SPACE);
static_assert(g_latin1ToAscii.back() == 'y');

NODISCARD inline constexpr size_t getIndex(const char c) noexcept
{
    return static_cast<size_t>(static_cast<uint8_t>(c));
}

NODISCARD inline constexpr bool isAscii(const char c) noexcept
{
    return getIndex(c) < NUM_ASCII_CODEPOINTS;
}

// widens
NODISCARD static inline constexpr char16_t windows125x_to_unicode(const uint8_t c) noexcept
{
#define XCASE(_win, _unicode, _name) \
    case (_win): \
        return static_cast<char16_t>(_unicode)

    switch (c) {
        XFOREACH_WINDOWS_125x(XCASE, XSEMI);
    default:
        return c;
    }

#undef XCASE
}

// widens
NODISCARD static inline constexpr char16_t windows125x_to_unicode(const char c) noexcept
{
    return windows125x_to_unicode(static_cast<uint8_t>(c));
}

NODISCARD static inline constexpr char16_t windows125x_to_unicode(const char16_t c) noexcept
{
    if ((c & 0xFFu) == c) {
        return windows125x_to_unicode(static_cast<uint8_t>(c));
    }
    return c;
}

MAYBE_UNUSED NODISCARD static inline constexpr char32_t windows125x_to_unicode(char32_t c) noexcept
{
    if ((c & 0xFFFFu) == c) {
        return windows125x_to_unicode(static_cast<char16_t>(c));
    }
    return c;
}

// converts non-latin1 unicode to latin1/ascii
//
// NOTE: This does an up-convert (8-bit to 16-bit) followed by a down-convert (16-bit to 8-bit).
// If the windows table converts to a unicode value that doesn't have a transliteration to ASCII,
// then the codepoint won't be modified here, but it may still be converted to the
// LATIN1_CONTROL_CODE_REPLACEMENT if the code is converted to ASCII, but it'll remain as a bogus
// codepoint if we're converting to Latin-1 or simply filtering unicode.
NODISCARD static constexpr char16_t simple_unicode_translit(const char16_t input) noexcept
{
#define XCASE(_unicode, _ascii, _name) \
    case (_unicode): \
        return conversion::to_char16(_ascii)

    const auto maybe_bigger = windows125x_to_unicode(input);
    switch (maybe_bigger) {
        XFOREACH_UNICODE_TRANSLIT(XCASE, XSEMI);
    default:
        return input;
    }

#undef XCASE
}

NODISCARD static constexpr char32_t simple_unicode_translit(const char32_t codepoint) noexcept
{
    if ((codepoint & 0xFFFFu) == codepoint) {
        return simple_unicode_translit(static_cast<char16_t>(codepoint));
    }
    return codepoint;
}

// convert Windows 125x control codes to unicode, and then back to ASCII.
NODISCARD static inline constexpr char windows125x_to_ascii(const char c) noexcept
{
    const char16_t c2 = simple_unicode_translit(charset::conversion::to_char16(c));
    if (const auto u = static_cast<uint16_t>(c2); u < 0x80u) {
        return static_cast<char>(static_cast<uint8_t>(u));
    }
    return c;
}

NODISCARD inline constexpr char latin1ToAscii(const char c) noexcept
{
    if (isAscii(c)) {
        return c;
    }

    const auto i = getIndex(c);
    if (i >= IDX_NBSP && i < NUM_LATIN1_CODEPOINTS) {
        return g_latin1ToAscii[static_cast<size_t>(i - IDX_NBSP)];
    } else if (const char fixed = windows125x_to_ascii(c); isAscii(fixed)) {
        return fixed;
    } else {
        return LATIN1_CONTROL_CODE_REPLACEMENT;
    }
}

// regular ASCII remains unchanged for U+00 to U+7F.
static_assert(latin1ToAscii('X') == 'X');
static_assert(latin1ToAscii('x') == 'x');

static_assert(latin1ToAscii(char_consts::C_DELETE) == char_consts::C_DELETE);
// windows 125x transliterations for U+80 to U+9F.
static_assert(latin1ToAscii('\x80') == LATIN1_CONTROL_CODE_REPLACEMENT);
static_assert(latin1ToAscii('\x8B') == C_LESS_THAN);
static_assert(latin1ToAscii('\x91') == C_SQUOTE);
static_assert(latin1ToAscii('\x95') == C_ASTERISK);
static_assert(latin1ToAscii('\x9B') == C_GREATER_THAN);
static_assert(latin1ToAscii('\x9F') == LATIN1_CONTROL_CODE_REPLACEMENT);
// standard latin1 transliteration for U+A0 to U+FF.
static_assert(latin1ToAscii(char_consts::C_NBSP) == char_consts::C_SPACE);
static_assert(latin1ToAscii('\xAB') == C_LESS_THAN);
static_assert(latin1ToAscii('\xBB') == C_GREATER_THAN);
static_assert(latin1ToAscii('\xFF') == 'y');

} // namespace charset_detail

NODISCARD bool isAscii(const char c) noexcept
{
    return charset_detail::isAscii(c);
}
NODISCARD bool isAscii(const std::string_view sv) noexcept
{
    return std::all_of(sv.begin(), sv.end(), [](char c) { return isAscii(c); });
}

namespace conversion {
NODISCARD char latin1ToAscii(const char c) noexcept
{
    return charset_detail::latin1ToAscii(c);
}

std::string &latin1ToAsciiInPlace(std::string &str) noexcept
{
    for (char &c : str) {
        if (!charset::isAscii(c)) {
            c = latin1ToAscii(c);
        }
    }
    return str;
}
} // namespace conversion
} // namespace charset

namespace mmqt {

NODISCARD static bool containsAnySurrogates(const QString &str)
{
    return std::any_of(str.begin(), str.end(), [](QChar qc) -> bool { return qc.isSurrogate(); });
}

NODISCARD QChar simple_unicode_translit(const QChar qc) noexcept
{
    return QChar{charset::simple_unicode_translit(static_cast<char16_t>(qc.unicode()))};
}

ALLOW_DISCARD QString &toLatin1InPlace(QString &str)
{
    if (containsAnySurrogates(str)) {
        // This allocates a new string
        const auto sv = mmqt::as_u16string_view(str);
        const auto latin1 = charset::conversion::utf16::utf16ToLatin1(sv);
        str = mmqt::toQStringLatin1(latin1);
        return str;
    }

    for (QChar &qc : str) {
        mmqt::simple_unicode_translit_in_place(qc);
        qc = QLatin1Char{qc.toLatin1()};
    }
    return str;
}

ALLOW_DISCARD QString &toAsciiInPlace(QString &str)
{
    toLatin1InPlace(str);

    // NOTE: 128 (0x80) was not converted to 'z' before.
    for (QChar &qc : str) {
        // c++17 if statement with initializer
        if (const char ch = qc.toLatin1(); !isAscii(ch)) {
            qc = QLatin1Char{charset::conversion::latin1ToAscii(ch)};
        }
    }
    return str;
}

NODISCARD QString toAscii(const QString &str)
{
    QString copy = str;
    toAsciiInPlace(copy);
    return copy;
}

NODISCARD QString toLatin1(const QString &str)
{
    QString copy = str;
    toLatin1InPlace(copy);
    return copy;
}

} // namespace mmqt

namespace charset {
namespace conversion {
NODISCARD std::string latin1ToAscii(const std::string_view sv)
{
    std::string tmp{sv};
    latin1ToAsciiInPlace(tmp);
    return tmp;
}

void latin1ToAscii(std::ostream &os, const std::string_view sv)
{
    for (char c : sv) {
        if (!charset::isAscii(c)) {
            c = latin1ToAscii(c);
        }
        os << c;
    }
}
} // namespace conversion

namespace charset_detail {
NODISCARD static constexpr bool isPrintLatin1(const char c)
{
    const auto uc = static_cast<uint8_t>(c);
    return uc >= ((uc < 0x7Fu) ? 0x20u : 0xA0u);
}
static_assert(!isPrintLatin1(C_ESC));
} // namespace charset_detail

NODISCARD bool isPrintLatin1(const char c) noexcept
{
    return charset_detail::isPrintLatin1(c);
}

NODISCARD char16_t simple_unicode_translit(const char16_t codepoint) noexcept
{
    return charset_detail::simple_unicode_translit(codepoint);
}

NODISCARD char32_t simple_unicode_translit(const char32_t codepoint) noexcept
{
    return charset_detail::simple_unicode_translit(codepoint);
}

namespace conversion {
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
        charset::conversion::latin1ToUtf8(os, sv);
        return;
    }

    std::abort();
}

void convert(std::ostream &os,
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

NODISCARD std::optional<uint8_t> try_pop_latin1(std::string_view &sv) noexcept
{
    if (sv.empty()) {
        return std::nullopt;
    }
    auto result = sv.front();
    sv.remove_prefix(1);
    return result;
}
} // namespace conversion

NODISCARD bool are_equivalent_utf8(std::u16string_view left,
                                   std::string_view right,
                                   const EquivTranslitOptionsEnum opts) noexcept
{
    while (!left.empty() && !right.empty()) {
        const auto a = charset::conversion::try_pop_utf16(left);
        if (!a.has_value()) {
            return false;
        }
        const auto b = charset::conversion::try_pop_utf8(right);
        if (!b.has_value()) {
            return false;
        }
        if (!are_equivalent(a.value(), b.value(), opts)) {
            return false;
        }
    }
    return left.empty() && right.empty();
}

namespace charset_detail {
static_assert(windows125x_to_ascii('\x91') == C_SQUOTE);
static_assert(windows125x_to_unicode('\x91') == char_consts::UNICODE_LSQUO);
static_assert(simple_unicode_translit(static_cast<char16_t>(char_consts::UNICODE_LSQUO))
              == C_SQUOTE);
static_assert(simple_unicode_translit(static_cast<char16_t>(0x91u)) == C_SQUOTE);
static_assert(simple_unicode_translit(static_cast<char16_t>(0x97u)) == C_MINUS_SIGN);

} // namespace charset_detail
} // namespace charset

namespace { // anonymous
template<typename Callback>
NODISCARD size_t countCharsMatching(Callback &&callback)
{
    size_t totalMatching = 0;
    for (size_t i = 0; i < 256; ++i) {
        if (callback(static_cast<char>(i))) {
            ++totalMatching;
        }
    }
    return totalMatching;
}

void testIsLowerIsUpper()
{
    using ascii::isLower;
    using ascii::isUpper;
    constexpr size_t NUM_LETTERS = 26;
    TEST_ASSERT(countCharsMatching(isLower) == NUM_LETTERS);
    TEST_ASSERT(countCharsMatching(isUpper) == NUM_LETTERS);

    static const auto isEither = [](const char c) -> bool { return isLower(c) || isUpper(c); };
    TEST_ASSERT(countCharsMatching(isEither) == 2 * NUM_LETTERS);

    static_assert('a' + NUM_LETTERS == 'z' + 1);
    for (char c = 'a'; c <= 'z'; ++c) {
        TEST_ASSERT(isLower(c));
    }

    static_assert('A' + NUM_LETTERS == 'Z' + 1);
    for (char c = 'A'; c <= 'Z'; ++c) {
        TEST_ASSERT(isUpper(c));
    }
}

void testIsCntrl()
{
    using ascii::isCntrl;
    using namespace char_consts;

    TEST_ASSERT(countCharsMatching(isCntrl) == 33);
    {
        // 00 to 1F, and 7F
        for (size_t i = 0; i < 32; ++i) {
            TEST_ASSERT(isCntrl(static_cast<char>(i)));
        }
        TEST_ASSERT(isCntrl(C_DELETE));
    }
    {
        // not many have names...
        const std::array<char, 10> expected{
            C_ALERT,
            C_BACKSPACE,
            C_CARRIAGE_RETURN,
            C_DELETE,
            C_ESC,
            C_FORM_FEED,
            C_NEWLINE,
            C_NUL,
            C_TAB,
            C_VERTICAL_TAB,
        };
        TEST_ASSERT(expected.size() == 10);

        std::array<bool, 256> seen{};
        size_t num_seen = 0;
        for (const char c : expected) {
            const auto uc = static_cast<uint8_t>(c);
            TEST_ASSERT(isCntrl(c));
            TEST_ASSERT(!seen[uc]);
            seen[uc] = true;
            ++num_seen;
        }
        TEST_ASSERT(num_seen == expected.size());
    }
}

void testIsPunct()
{
    using ascii::isPunct;
    using namespace char_consts;

    TEST_ASSERT(countCharsMatching(isPunct) == 32);

    const std::array<char, 32> expected{
        C_AMPERSAND,     C_ASTERISK,      C_AT_SIGN,     C_BACKSLASH,    C_BACK_TICK,
        C_CARET,         C_CLOSE_BRACKET, C_CLOSE_CURLY, C_CLOSE_PARENS, C_COLON,
        C_COMMA,         C_DOLLAR_SIGN,   C_DQUOTE,      C_EQUALS,       C_EXCLAMATION,
        C_GREATER_THAN,  C_LESS_THAN,     C_MINUS_SIGN,  C_OPEN_BRACKET, C_OPEN_CURLY,
        C_OPEN_PARENS,   C_PERCENT_SIGN,  C_PERIOD,      C_PLUS_SIGN,    C_POUND_SIGN,
        C_QUESTION_MARK, C_SEMICOLON,     C_SLASH,       C_SQUOTE,       C_TILDE,
        C_UNDERSCORE,    C_VERTICAL_BAR,
    };
    std::array<bool, 256> seen{};
    size_t num_seen = 0;
    for (const char c : expected) {
        const auto uc = static_cast<uint8_t>(c);
        TEST_ASSERT(isPunct(c));
        TEST_ASSERT(!seen[uc]);
        seen[uc] = true;
        ++num_seen;
    }
    TEST_ASSERT(num_seen == 32);
}

void testIsSpace()
{
    using ascii::isSpace;
    using namespace char_consts;

    TEST_ASSERT(countCharsMatching(isSpace) == 6);

    // expected
    TEST_ASSERT(isSpace(C_CARRIAGE_RETURN)); // '\r'
    TEST_ASSERT(isSpace(C_FORM_FEED));       // '\f'
    TEST_ASSERT(isSpace(C_NEWLINE));         // '\n'
    TEST_ASSERT(isSpace(C_SPACE));           //
    TEST_ASSERT(isSpace(C_TAB));             // '\t'
    TEST_ASSERT(isSpace(C_VERTICAL_TAB));    // '\v'

    // NOTE: this might be "unexpected" since its name is non-breaking space;
    // if you need to test for NBSP, you'll need to use a different
    // function (e.g. look for nbsp_aware::isAnySpace()).
    TEST_ASSERT(!isSpace(C_NBSP));
}

void testAsciiCharTypes()
{
    testIsLowerIsUpper();
    testIsCntrl();
    testIsPunct();
    testIsSpace();
}

} // namespace

namespace { // anonymous

enum class NODISCARD EncEnum : uint8_t { Latin1, Utf8 };

void compare_different_utf8(const std::u16string_view s16, const std::string_view s8)
{
    if (charset::are_equivalent_utf8(s16, s8)) {
        qFatal("test failed: strings are equivalent");
    }
}

void compare_same_utf8(const std::u16string_view s16, const std::string_view s8)
{
    if (!charset::are_equivalent_utf8(s16, s8)) {
        qFatal("test failed: strings are not equivalent");
    }
}

void compare_same_utf8_translit_right(const std::u16string_view s16, const std::string_view s8)
{
    if (!charset::are_equivalent_utf8(s16, s8, charset::EquivTranslitOptionsEnum::Right)) {
        qFatal("test failed: strings are not equivalent");
    }
}

void check_utf16(std::u16string_view sv,
                 const size_t size,
                 const std::initializer_list<char32_t> expect)
{
    if (sv.size() != size) {
        qFatal("test failed: wrong size");
    }
    for (const auto c : expect) {
        const auto got = charset::conversion::try_pop_utf16(sv);
        if (!got.has_value() || got.value() != c) {
            qFatal("test failed: wrong codepoint");
        }
    }
    if (!sv.empty()) {
        qFatal("test failed: wrong number of codepoints");
    }
}
void check_utf8(std::string_view sv, const size_t size, const std::initializer_list<char32_t> expect)
{
    if (sv.size() != size) {
        qFatal("test failed: wrong size");
    }
    for (const auto c : expect) {
        const auto got = charset::conversion::try_pop_utf8(sv);
        if (!got.has_value() || got.value() != c) {
            qFatal("test failed: wrong codepoint");
        }
    }
    if (!sv.empty()) {
        qFatal("test failed: wrong number of codepoints");
    }
}
void check_latin1(std::string_view sv,
                  const size_t size,
                  const std::initializer_list<char32_t> expect)
{
    if (sv.size() != size) {
        qFatal("test failed: wrong size");
    }
    for (const auto c : expect) {
        const auto got = charset::conversion::try_pop_latin1(sv);
        if (!got.has_value() || got.value() != c) {
            qFatal("test failed: wrong codepoint");
        }
    }
    if (!sv.empty()) {
        qFatal("test failed: wrong number of codepoints");
    }
}

void testStrings()
{
    using namespace charset;

    constexpr std::string_view ydots_latin1 = "\xFF";
    constexpr std::u16string_view ydots_utf16 = u"\u00FF";
    constexpr std::string_view thumbs_up8 = "\xF0\x9F\x91\x8D";
    constexpr std::u16string_view thumbs_up16 = u"\xD83D\xDC4D";
    constexpr std::string_view look_of_disapproval8 = "\xE0\xB2\xA0"
                                                      "\x5F"
                                                      "\xE0\xB2\xA0";
    constexpr std::u16string_view look_of_disapproval16 = u"\u0CA0"
                                                          "\u005F"

                                                          "\u0CA0";

    constexpr std::string_view foo8 = "\xE2\x80\x98" // lsquo
                                      "foo"
                                      "\xE2\x80\x99"; // rsquo

    constexpr std::u16string_view foo16 = u"\u2018" // lsquo
                                          "foo"
                                          "\u2019"; // rsquo

    constexpr std::string_view plain_foo_ascii = "'foo'";
    constexpr std::u16string_view plain_foo16 = u"'foo'";

    check_latin1(ydots_latin1, 1, {255});
    check_utf16(ydots_utf16, 1, {255});
    check_utf8(thumbs_up8, 4, {thumbs_up});
    check_utf16(thumbs_up16, 2, {thumbs_up});
    check_utf8(look_of_disapproval8, 7, {0xCA0, 0x5F, 0xCA0});
    check_utf16(look_of_disapproval16, 3, {0xCA0, 0x5F, 0xCA0});

    check_utf8(foo8, 9, {UNICODE_LSQUO, 'f', 'o', 'o', UNICODE_RSQUO});
    check_utf16(foo16, 5, {UNICODE_LSQUO, 'f', 'o', 'o', UNICODE_RSQUO});
    check_utf16(plain_foo16, 5, {C_SQUOTE, 'f', 'o', 'o', C_SQUOTE});

    compare_same_utf8(u"", "");

    // utf8
    compare_same_utf8(u"abc", "abc");
    compare_different_utf8(ydots_utf16, ydots_latin1);
    compare_different_utf8(u"abc", "ab");
    compare_different_utf8(u"abc", "abd");
    compare_same_utf8(thumbs_up16, thumbs_up8);
    compare_same_utf8(look_of_disapproval16, look_of_disapproval8);

    // utf8 with transliteration
    compare_same_utf8(foo16, foo8);
    compare_different_utf8(plain_foo16, foo8);
    compare_same_utf8_translit_right(plain_foo16, foo8);

    {
        TEST_ASSERT(charset::conversion::utf8ToAscii(foo8) == plain_foo_ascii);
        TEST_ASSERT(charset::conversion::utf8ToLatin1(foo8) == plain_foo_ascii);
        TEST_ASSERT(charset::conversion::utf16ToAscii(foo16) == plain_foo_ascii);
        TEST_ASSERT(charset::conversion::utf16ToLatin1(foo16) == plain_foo_ascii);
    }

    {
        charset::conversion::Utf16StringBuilder sb;
        sb += 0xCA0;
        sb += 0x5F;
        sb += 0xCA0;
        if (sb.get_string_view() != look_of_disapproval16) {
            throw std::runtime_error("test failed: error encoding utf16 string");
        }
        sb.clear();
        sb += thumbs_up;
        if (sb.get_string_view() != thumbs_up16) {
            throw std::runtime_error("test failed: error encoding utf16 string");
        }
        sb.clear();
        sb += MAX_UNICODE_CODEPOINT + 1;
        if (sb.get_string_view() != u"?") {
            throw std::runtime_error("test failed: error encoding invalid codepoint");
        }
        sb.clear();
        sb.set_unknown('x');
        sb += MAX_UNICODE_CODEPOINT + 1;
        if (sb.get_string_view() != u"x") {
            throw std::runtime_error("test failed: error encoding invalid codepoint");
        }
        sb.clear();
        sb += 0;
        if (sb.size() != 1 || sb.str().front() != 0) {
            throw std::runtime_error("test failed: error encoding utf16 NULL-codepoint");
        }
        sb.clear();
        std::vector<char32_t> arr{0xCA0, 0x5F, 0xCA0};
        sb += std::u32string_view{arr.data(), arr.size()};
        if (sb.get_string_view() != look_of_disapproval16) {
            throw std::runtime_error("test failed: error encoding utf16 string");
        }
    }

    if (charset::conversion::utf8ToUtf16(look_of_disapproval8) != look_of_disapproval16) {
        throw std::runtime_error("test failed: error encoding utf16 string");
    }

    if (charset::conversion::utf16ToUtf8(look_of_disapproval16) != look_of_disapproval8) {
        throw std::runtime_error("test failed: error encoding utf8 string");
    }

    {
        // swapped bytes outputs two invalid codepoints.
        const std::string input = "\x80\xC2";
        const auto output = charset::conversion::utf8ToUtf16(input);
        TEST_ASSERT(output == u"??");
    }
    {
        // over-long 4-byte codepoint for U+0 reports four invalid codepoints.
        const std::string input = "\xF0\x80\x80\x80xyz";
        const auto output = charset::conversion::utf8ToUtf16(input);
        TEST_ASSERT(output == u"????xyz");
    }
    {
        std::u16string input;
        // swapped bytes outputs two invalid codepoints.
        input += static_cast<char16_t>(charset::utf16_detail::LO_SURROGATE_MIN);
        input += static_cast<char16_t>(charset::utf16_detail::HI_SURROGATE_MIN);
        // and then the correct order outputs U+10000.
        input += static_cast<char16_t>(charset::utf16_detail::HI_SURROGATE_MIN);
        input += static_cast<char16_t>(charset::utf16_detail::LO_SURROGATE_MIN);
        const auto output = charset::conversion::utf16ToUtf8(input);
        TEST_ASSERT(output == "??\xF0\x90\x80\x80");

        charset::conversion::Utf8StringBuilder sb;
        sb += '?';
        sb += '?';
        sb += charset::utf16_detail::SURROGATE_OFFSET;
        TEST_ASSERT(output == sb.str());
    }
    {
        charset::conversion::Utf16StringBuilder sb;
        sb += MAX_UNICODE_CODEPOINT + 1;
        TEST_ASSERT(sb.str() == u"?");
        sb.clear();
        sb += charset::utf16_detail::FIRST_SURROGATE;
        TEST_ASSERT(sb.str() == u"?");
    }
    {
        size_t n = 0;
        charset::foreach_codepoint_utf8("", [&n](MAYBE_UNUSED auto codepoint) { ++n; });
        TEST_ASSERT(n == 0);
    }
    {
        size_t n = 0;
        charset::foreach_codepoint_utf8("x", [&n](MAYBE_UNUSED auto codepoint) { ++n; });
        TEST_ASSERT(n == 1);
    }
    {
        size_t n = 0;
        for (MAYBE_UNUSED auto ignored : charset::conversion::Utf8Iterable{""}) {
            ++n;
        }
        TEST_ASSERT(n == 0);
    }
    {
        size_t n = 0;
        for (MAYBE_UNUSED auto ignored : charset::conversion::Utf8Iterable{"x"}) {
            ++n;
        }
        TEST_ASSERT(n == 1);
    }
    {
        auto thing = charset::conversion::Utf8Iterable{};
        size_t n = 0;
        auto it = thing.begin();
        auto end = thing.end();
        for (; it != end; ++it) {
            MAYBE_UNUSED char32_t codepoint = *it;
            ++n;
        }
        TEST_ASSERT(n == 0);
    }
    {
        {
            size_t n = 0;
            for (MAYBE_UNUSED auto ignored : charset::conversion::Utf8Iterable{thumbs_up8}) {
                ++n;
            }
            TEST_ASSERT(n == 1);
        }
        {
            // The current design causes Utf8Iterable to multi-report sliced codepoints.
            constexpr auto invalid = charset::conversion::Utf8Iterable{}.invalid;
            size_t n = 0;
            std::vector<char32_t> codepoints;
            for (MAYBE_UNUSED auto codepoint :
                 charset::conversion::Utf8Iterable{thumbs_up8.substr(0, thumbs_up8.size() - 1)}) {
                TEST_ASSERT(codepoint == invalid);
                ++n;
            }
            TEST_ASSERT(n == 3);
        }
    }
}

void testStringsExtreme()
{
    using namespace charset;
    using namespace charset::conversion;
    static constexpr char invalid = '?';
    Utf8StringBuilder sb8{invalid};
    Utf16StringBuilder sb16{invalid};

    const auto expected_surrogates = charset::utf16_detail::LAST_SURROGATE
                                     - charset::utf16_detail::FIRST_SURROGATE + 1;

    qInfo() << "Verifying roundtrip encoding/decoding of all" << (MAX_UNICODE_CODEPOINT + 1)
            << "unicode codepoints (except for" << expected_surrogates
            << "invalid surrogate codepoints)...";

    size_t num_checked = 0;
    size_t num_surrogates = 0;
    for (char32_t i = 0; i <= MAX_UNICODE_CODEPOINT; ++i, sb8.clear(), sb16.clear()) {
        ++num_checked;
        const bool is_surrogate = charset::utf16_detail::is_utf16_surrogate(i);
        if (is_surrogate) {
            ++num_surrogates;
        }

        sb8 += i;
        sb16 += i;

        const auto sv8 = sb8.get_string_view();
        const auto sv16 = sb16.get_string_view();

        {
            const auto opt8 = try_encode_utf8(i);
            const auto opt16 = try_encode_utf16(i);

            TEST_ASSERT(is_surrogate ^ opt8.has_value());
            TEST_ASSERT(is_surrogate ^ opt16.has_value());

            if (!is_surrogate) {
                {
                    auto copy = opt8.value();
                    TEST_ASSERT(!copy.empty());
                    const auto got = try_pop_utf8(copy);
                    TEST_ASSERT(got == i && copy.empty());
                }
                {
                    auto copy = opt16.value();
                    TEST_ASSERT(!copy.empty());
                    const auto got = try_pop_utf16(copy);
                    TEST_ASSERT(got == i && copy.empty());
                }
            }
        }

        {
            auto copy = sv8;
            const auto got = try_pop_utf8(copy);
            TEST_ASSERT(copy.empty() && got == (is_surrogate ? invalid : i));
        }
        {
            auto copy = sv16;
            const auto got = try_pop_utf16(copy);
            TEST_ASSERT(copy.empty() && got == (is_surrogate ? invalid : i));
        }

        if (!are_equivalent_utf8(sv16, sv8)) {
            qFatal("test failed: strings are not equivalent");
        }
    }
    qInfo() << "Finished verifying" << num_checked << "codepoints. Verified that" << num_surrogates
            << "invalid surrogates are in fact invalid.";
    TEST_ASSERT(num_surrogates == expected_surrogates);
}

void testMmqtLatin1()
{
    // verify that surrogates are handled as expected.
    {
        const QString thumbsUpQstr{"\U0001F44D"};
        TEST_ASSERT(thumbsUpQstr.size() == 2);
        TEST_ASSERT(thumbsUpQstr.front() == char16_t(0xD83D));
        TEST_ASSERT(thumbsUpQstr.back() == char16_t(0xDC4D));
        TEST_ASSERT(thumbsUpQstr.front().isHighSurrogate());
        TEST_ASSERT(thumbsUpQstr.back().isLowSurrogate());

        // Qt's toLatin1() function ignores the fact that surrogates are a single codepoint.
        {
            const QByteArray ba = thumbsUpQstr.toLatin1(); // sanity checking
            TEST_ASSERT(ba == "??");
        }

        // but utf16ToLatin1() knows how to decode surrogates.
        {
            const auto utf16 = mmqt::as_u16string_view(thumbsUpQstr);

            // utf16ToLatin1() yields a single question mark, since it understands surrogates
            // and transforms them into a single codepoint.
            const auto latin1 = charset::conversion::utf16::utf16ToLatin1(utf16); // sanity checking
            TEST_ASSERT(latin1 == SV_QUESTION_MARK);
        }

        // and toLatin1InPlace() is expected to call utf16ToLatin1() when the string contains
        // a surrogate, so it will only report a single codepoint.
        {
            QString qs = thumbsUpQstr;
            mmqt::toLatin1InPlace(qs); // sanity checking
            TEST_ASSERT(qs == mmqt::QS_QUESTION_MARK);
        }
    }

    // Verify that both unicode and Windows-1252 codepoints are transliterated,
    // while ASCII and LATIN-1 are preserved.
    {
        const QString quotes{"\u2018\u0091x\u00A0y\u0092\u2019"};
        {
            auto qs = quotes;
            if ((false)) {
                for (QChar c : qs) {
                    qInfo() << +c.unicode();
                }
            }

            TEST_ASSERT(qs.size() == 7);

            mmqt::toLatin1InPlace(qs); // sanity checking
            TEST_ASSERT(qs.size() == 7);
            TEST_ASSERT(qs == "''x\u00A0y''");
        }
        {
            const auto utf16 = mmqt::as_u16string_view(quotes);
            const auto latin1 = charset::conversion::utf16::utf16ToLatin1(utf16); // sanity checking
            TEST_ASSERT(latin1.size() == 7);
            TEST_ASSERT(latin1 == "''x\xA0y''");
        }
    }
}

} // namespace

namespace test {
void testCharset()
{
    testAsciiCharTypes();
    testStrings();
    testMmqtLatin1();

    volatile bool use_extreme_roundtrip_test = false; // (this test is very slow)
    if (use_extreme_roundtrip_test) {
        testStringsExtreme();
    }
}
} // namespace test
