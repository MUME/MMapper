#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "ConfigEnums.h"
#include "Consts.h"
#include "macros.h"
#include "utils.h"

#include <array>
#include <iostream>
#include <optional>
#include <string>

#include <QString>

namespace charset::ascii {

// ASCII 00 to 1F, and 7F only; Latin1 control codes 80 to 9F don't count.
NODISCARD static inline bool isCntrl(const char c)
{
    const unsigned char uc = static_cast<unsigned char>(c);
    return (uc <= 0x1F) || (uc == 0x7F);
}

// ASCII only
NODISCARD static inline bool isDigit(const char c)
{
    return std::isdigit(static_cast<uint8_t>(c));
}

// ASCII only; Latin1 letters don't count.
NODISCARD static inline bool isLower(const char c)
{
    return (c >= 'a') && (c <= 'z');
}

// ASCII only; Latin1 punctuations don't count.
NODISCARD static inline bool isPunct(const char c)
{
    const unsigned char uc = static_cast<unsigned char>(c);
    // 33–47    !"#$%&'()*+,-./
    // 58–64    :;<=>?@
    // 91–96    [\]^_`
    // 123–126  {|}~
    return ((uc >= char_consts::C_EXCLAMATION) && (uc <= char_consts::C_SLASH))
           || ((uc >= char_consts::C_COLON) && (uc <= char_consts::C_AT_SIGN))
           || ((uc >= char_consts::C_OPEN_BRACKET) && (uc <= char_consts::C_BACK_TICK))
           || ((uc >= char_consts::C_OPEN_CURLY) && (uc <= char_consts::C_TILDE));
}

// ASCII only; Latin1 NBSP doesn't count.
NODISCARD static inline bool isSpace(const char c)
{
    switch (c) {
    case char_consts::C_SPACE:
    case char_consts::C_TAB:
    case char_consts::C_NEWLINE:
    case char_consts::C_VERTICAL_TAB:
    case char_consts::C_FORM_FEED:
    case char_consts::C_CARRIAGE_RETURN:
        return true;
    default:
        return false;
    }
}

// ASCII only; Latin1 letters don't count.
NODISCARD static inline bool isUpper(const char c)
{
    return (c >= 'A') && (c <= 'Z');
}

} // namespace charset::ascii

// temporary alias for charset::ascii
namespace ascii {
using namespace charset::ascii;
}

namespace charset {
namespace charset_detail {
static inline constexpr const char DEFAULT_UNMAPPED_CHARACTER = char_consts::C_QUESTION_MARK;
static inline constexpr const size_t NUM_ASCII_CODEPOINTS = 128;
static inline constexpr const size_t NUM_LATIN1_CODEPOINTS = 256;
} // namespace charset_detail

NODISCARD extern bool isAscii(char c) noexcept;
NODISCARD extern bool isAscii(std::string_view sv) noexcept;
NODISCARD extern bool isPrintLatin1(char c) noexcept;

namespace conversion {
NODISCARD extern char latin1ToAscii(char c) noexcept;

ALLOW_DISCARD extern std::string &latin1ToAsciiInPlace(std::string &str) noexcept;
NODISCARD extern std::string latin1ToAscii(std::string_view sv);
} // namespace conversion
} // namespace charset

namespace mmqt {
// CAUTION: This doesn't handle surrogate pairs.
NODISCARD QChar simple_unicode_translit(QChar qc) noexcept;
inline void simple_unicode_translit_in_place(QChar &qc) noexcept
{
    qc = simple_unicode_translit(qc);
}

NODISCARD QString toAscii(const QString &str);
NODISCARD QString toLatin1(const QString &str);

// REVISIT: should these functions correctly handle surrogates or not?
// Currently they do, but some other "in place" functions do not.

ALLOW_DISCARD QString &toAsciiInPlace(QString &str);
ALLOW_DISCARD QString &toLatin1InPlace(QString &str);
} // namespace mmqt

namespace charset {
namespace conversion {
extern void latin1ToUtf8(std::ostream &os, char c);

extern void latin1ToAscii(std::ostream &os, std::string_view sv);
extern void latin1ToUtf8(std::ostream &os, std::string_view sv);

NODISCARD extern std::string latin1ToUtf8(std::string_view sv);

// Converts input string_view sv from latin1 to the specified encoding
// by writing "raw bytes" to the output stream os.
extern void convertFromLatin1(std::ostream &os, CharacterEncodingEnum encoding, std::string_view sv);

extern void utf8ToAscii(std::ostream &os, std::string_view sv);
extern void utf8ToLatin1(std::ostream &os, std::string_view sv);

extern void convert(std::ostream &os,
                    std::string_view sv,
                    CharacterEncodingEnum from,
                    CharacterEncodingEnum to);

} // namespace conversion

enum class NODISCARD Utf8ValidationEnum : uint8_t {
    // Valid: Fully valid UTF-8.
    Valid,
    // ContainsInvalidEncodings: This can be recognized as "encoded as UTF-8,"
    // but it contains invalid encodings,  such as over-long representations,
    // codepoints in the surrogate range, or codepoints greater than U+10FFFF.
    //
    // This is a "permissive" option that can be corrected by replacing each
    // invalid encoding with an fixed substitution codepoint (e.g. '?').
    ContainsInvalidEncodings,
    // ContainsErrors: Contains errors such as invalid prefixes, truncated codepoints,
    // and out-of-sequence continuations.
    //
    // This means the bytes are either:
    //   (a) definitely not intended to be UTF-8, or
    //   (b) the data has been truncated / corrupted.
    ContainsErrors,
};

NODISCARD Utf8ValidationEnum validateUtf8(std::string_view sv) noexcept;
NODISCARD bool isValidUtf8(std::string_view sv) noexcept;

namespace conversion {
NODISCARD inline constexpr char16_t to_char16(const char c) noexcept
{
    return static_cast<char16_t>(static_cast<uint8_t>(c));
}

NODISCARD std::optional<char32_t> try_match_utf8(std::string_view sv) noexcept;
NODISCARD std::optional<char32_t> try_match_utf16(std::u16string_view sv) noexcept;

NODISCARD std::optional<uint8_t> try_pop_latin1(std::string_view &sv) noexcept;
NODISCARD std::optional<char32_t> try_pop_utf8(std::string_view &sv) noexcept;
NODISCARD std::optional<char32_t> try_pop_utf16(std::u16string_view &sv) noexcept;
} // namespace conversion

// This performs "simple" unicode to latin1 transliteration as opposed to "unfriendly"
// which replaces out-of-range codepoints with with invalid (e.g. '?'), when converting
// from unicode to Latin-1 or ASCII.
//
// Example: "simple" transliteration replaces "smart quotes" like U+2018 (aka "lsquo"
// or "left single quotation mark") with a regular ASCII single quote ('\'') instead
// of the invalid codepoint substitution (e.g. '?').
//
// Unicode input:
//  * utf8 input:   "\xE2\x80\x98text\xE2\x80\x99" (aka u8"\u2018text\u2019") (10 bytes), or
//  * utf16 input: u"\u2018text\u2019" (12 bytes)
//
// Latin-1 or ASCII output:
//  * simple output:     "'text'" (6 bytes), vs
//  * unfriendly output: "?text?" (6 bytes).
NODISCARD char16_t simple_unicode_translit(char16_t codepoint) noexcept;
NODISCARD char32_t simple_unicode_translit(char32_t codepoint) noexcept;

inline void simple_unicode_translit_in_place(char16_t &codepoint) noexcept
{
    codepoint = simple_unicode_translit(codepoint);
}
inline void simple_unicode_translit_in_place(char32_t &codepoint) noexcept
{
    codepoint = simple_unicode_translit(codepoint);
}

namespace conversion {

namespace conversion_detail {
enum class NODISCARD BasicCharsetEnum : uint8_t { Ascii, Latin1 };
enum class NODISCARD BasicTranslitOptionEnum : uint8_t {
    // Unicode out of the Latin1 range is replaced with the specified "invalid" codepoint.
    Unfriendly,
    // Unicode out of the Latin1 range can be replaced with a single Latin1 codepoint
    Simple,
};

template<BasicCharsetEnum charset, BasicTranslitOptionEnum use_translit>
class NODISCARD BasicCharsetInserter final
{
private:
    std::ostream *m_os = nullptr;

public:
    explicit BasicCharsetInserter(std::ostream &os) noexcept
        : m_os{&os}
    {}

private:
    NODISCARD static char32_t maybe_transit_unicode(const char32_t codepoint) noexcept
    {
        if constexpr (use_translit == BasicTranslitOptionEnum::Simple) {
            return simple_unicode_translit(codepoint);
        }
        return codepoint;
    }

    NODISCARD static char to_latin1(const char32_t codepoint) noexcept
    {
        if (codepoint > charset_detail::NUM_LATIN1_CODEPOINTS) {
            return charset_detail::DEFAULT_UNMAPPED_CHARACTER;
        }
        return static_cast<char>(static_cast<uint8_t>(codepoint));
    }

    NODISCARD static char maybe_ascii(const char c) noexcept
    {
        if constexpr (charset == BasicCharsetEnum::Ascii) {
            if (!isAscii(c)) {
                return latin1ToAscii(c);
            }
        }
        return c;
    }

    NODISCARD std::ostream &get_ostream() { return deref(m_os); }

public:
    void append_char(const char c) { get_ostream() << maybe_ascii(c); }
    void append_codepoint(const char32_t codepoint)
    {
        if (codepoint <= char_consts::C_DELETE
            && charset::ascii::isCntrl(static_cast<char>(codepoint))) {
            append_char(charset_detail::DEFAULT_UNMAPPED_CHARACTER);
            return;
        }
        append_char(to_latin1(maybe_transit_unicode(codepoint)));
    }
    void operator()(const char32_t codepoint) { append_codepoint(codepoint); }
};

using InsertAscii = BasicCharsetInserter<BasicCharsetEnum::Ascii, BasicTranslitOptionEnum::Simple>;
using InsertAsciiUnfriendly
    = BasicCharsetInserter<BasicCharsetEnum::Ascii, BasicTranslitOptionEnum::Unfriendly>;
using InsertLatin1 = BasicCharsetInserter<BasicCharsetEnum::Latin1, BasicTranslitOptionEnum::Simple>;
using InsertLatin1Unfriendly
    = BasicCharsetInserter<BasicCharsetEnum::Latin1, BasicTranslitOptionEnum::Unfriendly>;

} // namespace conversion_detail

using conversion_detail::InsertAscii;
using conversion_detail::InsertAsciiUnfriendly;
using conversion_detail::InsertLatin1;
using conversion_detail::InsertLatin1Unfriendly;

} // namespace conversion

// This refers to "simple" unicode to latin1 transliteration,
// as performed by simple_unicode_translit().
enum class NODISCARD EquivTranslitOptionsEnum : uint8_t { None, Left, Right, Both };

template<typename Char>
NODISCARD bool are_equivalent(Char left, Char right, const EquivTranslitOptionsEnum opts) noexcept
{
    static_assert(std::is_same_v<Char, char16_t> || std::is_same_v<Char, char32_t>);

    switch (opts) {
    case EquivTranslitOptionsEnum::None:
        break;
    case EquivTranslitOptionsEnum::Left:
        simple_unicode_translit_in_place(left);
        break;
    case EquivTranslitOptionsEnum::Right:
        simple_unicode_translit_in_place(right);
        break;
    case EquivTranslitOptionsEnum::Both:
        simple_unicode_translit_in_place(left);
        simple_unicode_translit_in_place(right);
        break;
    }

    return left == right;
}

NODISCARD bool are_equivalent_utf8(
    std::u16string_view left,
    std::string_view right,
    EquivTranslitOptionsEnum opts = EquivTranslitOptionsEnum::None) noexcept;

namespace conversion {
namespace conversion_detail {

template<typename Helper_>
class NODISCARD UnicodeIterator final
{
public:
    using StringView = typename Helper_::StringView;

private:
    StringView m_sv;
    char32_t m_invalid = charset_detail::DEFAULT_UNMAPPED_CHARACTER;

public:
    explicit UnicodeIterator(const StringView sv, char32_t invalid) noexcept
        : m_sv{sv}
        , m_invalid{invalid}
    {}

private:
    NODISCARD bool has_value() const noexcept { return !m_sv.empty(); }

    NODISCARD char32_t get_current() const noexcept(noexcept(Helper_::try_match(m_sv)))
    {
        if (!has_value()) {
            std::abort();
        }
        if (auto opt = Helper_::try_match(m_sv)) {
            return opt.value();
        } else {
            return m_invalid;
        }
    }
    void advance() noexcept(noexcept(Helper_::pop(m_sv)))
    {
        if (!has_value()) {
            std::abort();
        }
        Helper_::pop(m_sv);
    }

public:
    NODISCARD bool operator!=(std::nullptr_t) const noexcept { return has_value(); }
    NODISCARD char32_t operator*() const noexcept(noexcept(get_current())) { return get_current(); }
    UnicodeIterator &operator++() noexcept(noexcept(advance()))
    {
        advance();
        return *this;
    }
};

struct NODISCARD Utf8IteratorHelper final
{
    using StringView = std::string_view;
    NODISCARD static std::optional<char32_t> try_match(const StringView sv) noexcept
    {
        return try_match_utf8(sv);
    }
    static void pop(StringView &sv) noexcept
    {
        if (!try_pop_utf8(sv)) {
            // this can happen for invalid leading bytes (including unexpected continuation bytes),
            // truncated codepoints (missing required continuations), codepoints in the utf16
            // surrogate range, and out of bounds codepoints (over-long, or values above U+10FFFF).
            //
            // perhaps we should only report one invalid codepoint for every
            // truncated codepoint, but it's an error anyway, so it probably
            // doesn't matter.
            //
            // FIXME: Actually it does matter; if someone passes in a truncated string that
            // contains a partial utf8 codepoint, then we probably don't want to report multiple
            // invalid codepoints.
            sv.remove_prefix(1);
        }
    }
};

struct NODISCARD Utf16IteratorHelper final
{
    using StringView = std::u16string_view;
    NODISCARD static std::optional<char32_t> try_match(const StringView sv) noexcept
    {
        return try_match_utf16(sv);
    }
    static void pop(StringView &sv) noexcept
    {
        if (!conversion::try_pop_utf16(sv)) {
            // this can only happen for out of order surrogates;
            // there are no out of bounds utf16 codepoints.
            // The full range is U+0 to U+10FFFF,
            // with missing codepoints in the surrogate range U+D800 to U*DFFF
            // which cannot be represented, so it's impossible to decode them
            // as "over-long" codepoints that can occur with utf8.
            sv.remove_prefix(1);
        }
    }
};

using Utf8Iterator = UnicodeIterator<Utf8IteratorHelper>;
using Utf16Iterator = UnicodeIterator<Utf16IteratorHelper>;

} // namespace conversion_detail

struct NODISCARD Utf8Iterable final
{
    using Iterator = conversion_detail::Utf8Iterator;
    std::string_view sv;
    char32_t invalid = charset_detail::DEFAULT_UNMAPPED_CHARACTER;
    NODISCARD Iterator begin() const noexcept { return Iterator{sv, invalid}; }
    NODISCARD std::nullptr_t end() const noexcept { return nullptr; } // NOLINT
};

struct NODISCARD Utf16Iterable final
{
    using Iterator = conversion_detail::Utf16Iterator;
    std::u16string_view sv;
    char32_t invalid = charset_detail::DEFAULT_UNMAPPED_CHARACTER;
    NODISCARD Iterator begin() const noexcept { return Iterator{sv, invalid}; }
    NODISCARD std::nullptr_t end() const noexcept { return nullptr; } // NOLINT
};

} // namespace conversion

template<typename Callback>
void foreach_codepoint_ascii(const std::string_view sv, Callback &&callback)
{
    assert(isAscii(sv));
    for (const char c : sv) {
        callback(static_cast<uint8_t>(c));
    }
}

template<typename Callback>
void foreach_codepoint_latin1(std::string_view sv, Callback &&callback)
{
    for (const char c : sv) {
        callback(static_cast<uint8_t>(c));
    }
}

template<typename Callback>
void foreach_codepoint_utf8(std::string_view sv,
                            Callback &&callback,
                            const char32_t invalid = charset_detail::DEFAULT_UNMAPPED_CHARACTER)
{
    if (isAscii(sv)) {
        foreach_codepoint_ascii(sv, std::forward<Callback>(callback));
        return;
    }

    for (const char32_t codepoint : conversion::Utf8Iterable{sv, invalid}) {
        callback(simple_unicode_translit(codepoint));
    }
}

template<typename Callback>
void foreach_codepoint_utf8_unfriendly(
    std::string_view sv,
    Callback &&callback,
    const char32_t invalid = charset_detail::DEFAULT_UNMAPPED_CHARACTER)
{
    if (isAscii(sv)) {
        foreach_codepoint_ascii(sv, std::forward<Callback>(callback));
        return;
    }

    for (const char32_t codepoint : conversion::Utf8Iterable{sv, invalid}) {
        callback(codepoint);
    }
}

namespace conversion {

namespace conversion_detail {
template<typename StringViewType_, typename CtorCharType_, size_t MaxUnits_>
class NODISCARD OptionalEncodedCodepoint final
{
    using CtorCharType = CtorCharType_;
    using StringViewType = StringViewType_;
    using StorageCharType = typename StringViewType::value_type;
    static_assert(sizeof(CtorCharType) == sizeof(StorageCharType));
    static inline constexpr size_t MaxUnits = MaxUnits_;
    static_assert(MaxUnits <= 4);

private:
    using Array = std::array<StorageCharType, MaxUnits + 1>;
    Array m_units{};
    uint8_t m_size = 0;

public:
    template<typename... Chars>
    explicit constexpr OptionalEncodedCodepoint(Chars... units) noexcept
    {
        static_assert(sizeof...(units) <= MaxUnits);
        static_assert((std::is_same_v<Chars, CtorCharType> && ...));
        m_units = Array{static_cast<StorageCharType>(units)...};
        m_size = sizeof...(units);
    }

    void reset()
    {
        if (empty()) {
            return;
        }
        m_units.fill(StorageCharType{0});
        m_size = 0;
    }
    NODISCARD constexpr size_t size() const noexcept { return m_size; }
    NODISCARD constexpr bool empty() const noexcept { return size() == 0; }
    NODISCARD constexpr bool has_value() const noexcept { return !empty(); }
    NODISCARD constexpr explicit operator bool() const noexcept { return has_value(); }
    NODISCARD constexpr StringViewType value() const &&noexcept = delete;
    NODISCARD constexpr StringViewType value() const &noexcept { return lvalue(); }
    NODISCARD constexpr StringViewType rvalue() const &&noexcept { return lvalue(); }
    NODISCARD constexpr StringViewType lvalue() const &
    {
        if (!has_value()) {
            throw std::runtime_error("invalid optional");
        }
        return StringViewType{m_units.data(), size()};
    }

    NODISCARD constexpr bool operator==(std::nullopt_t) const noexcept { return !has_value(); }
    NODISCARD constexpr bool operator==(StringViewType rhs) const
    {
        return has_value() && lvalue() == rhs;
    }
};

template<typename Helper_>
class NODISCARD StringBuilder final
{
public:
    using StringType = typename Helper_::String;
    using CharType = typename StringType::value_type;
    using ViewType = std::basic_string_view<CharType>;

private:
    StringType m_units;
    CharType m_unknown = static_cast<CharType>(
        static_cast<uint8_t>(charset_detail::DEFAULT_UNMAPPED_CHARACTER));

public:
    explicit StringBuilder() = default;
    explicit StringBuilder(const CharType unknown_codepoint) { set_unknown(unknown_codepoint); }

public:
    NODISCARD size_t size() const noexcept { return m_units.size(); }
    NODISCARD bool empty() const noexcept { return m_units.empty(); }
    NODISCARD StringType steal_buffer() { return std::exchange(m_units, {}); }

public:
    NODISCARD const StringType &str() const &noexcept { return m_units; }
    NODISCARD StringType str() && { return steal_buffer(); }

public:
    NODISCARD ViewType get_string_view() const && = delete;
    NODISCARD ViewType get_string_view() const &noexcept { return m_units; }

public:
    void set_unknown(const CharType codepoint)
    {
        if (!Helper_::isValid(codepoint)) {
            throw std::runtime_error("invalid codepoint");
        }
        m_unknown = codepoint;
    }
    void swap_buffer(std::string &other)
    {
        using std::swap;
        swap(m_units, other);
    }
    void clear() { m_units.clear(); }
    void reserve(const size_t units) { m_units.reserve(units); }

public:
    NODISCARD bool try_append(const char32_t codepoint)
    {
        if (const auto opt = Helper_::tryEncode(codepoint)) {
            m_units += opt.value();
            return true;
        }
        return false;
    }
    void append(const char32_t codepoint)
    {
        if (!try_append(codepoint)) {
            m_units.push_back(m_unknown);
        }
    }
    StringBuilder &operator+=(const char32_t codepoint)
    {
        append(codepoint);
        return *this;
    }
    StringBuilder &operator+=(const std::u32string_view codepoints)
    {
        for (const char32_t codepoint : codepoints) {
            append(codepoint);
        }
        return *this;
    }
};

} // namespace conversion_detail

using OptionalEncodedUtf8Codepoint
    = conversion_detail::OptionalEncodedCodepoint<std::string_view, uint8_t, 4>;

NODISCARD OptionalEncodedUtf8Codepoint try_encode_utf8(char32_t codepoint) noexcept;
NODISCARD OptionalEncodedUtf8Codepoint try_encode_utf8_unchecked(char32_t codepoint,
                                                                 size_t bytes) noexcept;

namespace conversion_detail {
template<BasicTranslitOptionEnum use_translit>
struct NODISCARD Latin1StringBuilderHelper final
{
    using String = std::string;
    NODISCARD static bool isValid(const char) noexcept { return true; }
    NODISCARD static std::optional<char> tryEncode(char32_t codepoint) noexcept
    {
        if constexpr (use_translit == BasicTranslitOptionEnum::Simple) {
            simple_unicode_translit_in_place(codepoint);
        }
        if (codepoint > charset_detail::NUM_LATIN1_CODEPOINTS) {
            return charset_detail::DEFAULT_UNMAPPED_CHARACTER;
        }
        return static_cast<char>(static_cast<uint8_t>(codepoint));
    }
};
struct NODISCARD Utf8StringBuilderHelper final
{
    using String = std::string;
    NODISCARD static bool isValid(const char c) noexcept { return isAscii(c); }
    NODISCARD static auto tryEncode(const char32_t codepoint) noexcept
    {
        return try_encode_utf8(codepoint);
    }
};

using Latin1StringBuilderUnfriendly
    = StringBuilder<Latin1StringBuilderHelper<BasicTranslitOptionEnum::Unfriendly>>;
using Latin1StringBuilder = StringBuilder<Latin1StringBuilderHelper<BasicTranslitOptionEnum::Simple>>;
using Utf8StringBuilder = StringBuilder<Utf8StringBuilderHelper>;

} // namespace conversion_detail

using conversion_detail::Latin1StringBuilder;
using conversion_detail::Latin1StringBuilderUnfriendly;
using conversion_detail::Utf8StringBuilder;

inline void utf8ToAscii(std::ostream &os, const std::string_view sv)
{
    foreach_codepoint_utf8(sv, InsertAscii{os});
}

inline void utf8ToLatin1(std::ostream &os, const std::string_view sv)
{
    foreach_codepoint_utf8(sv, InsertLatin1{os});
}

NODISCARD std::string utf8ToAscii(std::string_view sv);
NODISCARD std::string utf8ToLatin1(std::string_view sv);
} // namespace conversion

namespace utf16_detail {
static inline constexpr uint16_t HI_SURROGATE_MIN = 0xD800u;
static inline constexpr uint16_t HI_SURROGATE_MAX = 0xDBFFu;
static inline constexpr uint16_t LO_SURROGATE_MIN = 0xDC00u;
static inline constexpr uint16_t LO_SURROGATE_MAX = 0xDFFFu;
static_assert(HI_SURROGATE_MAX == HI_SURROGATE_MIN + 0x3FFu);
static_assert(HI_SURROGATE_MAX + 1 == LO_SURROGATE_MIN);
static_assert(LO_SURROGATE_MAX == LO_SURROGATE_MIN + 0x3FFu);

static inline constexpr uint32_t SURROGATE_OFFSET = 0x10000u;
static inline constexpr char32_t FIRST_SURROGATE = HI_SURROGATE_MIN;
static inline constexpr char32_t LAST_SURROGATE = LO_SURROGATE_MAX;

// d800: 1101 1000 0000 0000
// dbff: 1101 1011 1111 1111
//              xx xxxx xxxx (10 bits)
// dc00: 1101 1100 0000 0000
// dfff: 1101 1111 1111 1111
//              yy yyyy yyyy (10 bits)
static inline constexpr uint16_t BITS_PER_SURROGATE = 10;
static inline constexpr uint16_t BOTTOM_TEN_BITS = (1u << BITS_PER_SURROGATE) - 1;

NODISCARD static inline constexpr bool is_utf16_hi_surrogate(const uint16_t hi) noexcept
{
    return HI_SURROGATE_MIN <= hi && hi <= HI_SURROGATE_MAX;
}
NODISCARD static inline constexpr bool is_utf16_lo_surrogate(const uint16_t lo) noexcept
{
    return LO_SURROGATE_MIN <= lo && lo <= LO_SURROGATE_MAX;
}
NODISCARD static inline constexpr bool is_utf16_surrogate(const uint16_t word) noexcept
{
    return HI_SURROGATE_MIN <= word && word <= LO_SURROGATE_MAX;
}

NODISCARD static inline constexpr bool is_utf16_surrogate(const char16_t codepoint) noexcept
{
    return is_utf16_surrogate(static_cast<uint16_t>(codepoint));
}

NODISCARD static inline constexpr bool is_utf16_surrogate(const char32_t codepoint) noexcept
{
    return FIRST_SURROGATE <= codepoint && codepoint <= LAST_SURROGATE;
}

} // namespace utf16_detail

namespace conversion::utf16 {

NODISCARD static bool containsSurrogates(const std::u16string_view sv) noexcept
{
    return std::any_of(std::begin(sv), std::end(sv), [](char16_t c) -> bool {
        return utf16_detail::is_utf16_surrogate(c);
    });
}

using OptionalEncodedUtf16Codepoint
    = conversion_detail::OptionalEncodedCodepoint<std::u16string_view, uint16_t, 2>;

NODISCARD OptionalEncodedUtf16Codepoint try_encode_utf16(char32_t codepoint) noexcept;

namespace conversion_utf16_detail {
struct NODISCARD Utf16StringBuilderHelper final
{
    using String = std::u16string;

    NODISCARD static bool isValid(const char16_t unit) noexcept
    {
        return !utf16_detail::is_utf16_surrogate(unit);
    }
    NODISCARD static auto tryEncode(const char32_t codepoint) noexcept
    {
        return try_encode_utf16(codepoint);
    }
};
} // namespace conversion_utf16_detail
using Utf16StringBuilder
    = conversion_detail::StringBuilder<conversion_utf16_detail::Utf16StringBuilderHelper>;

NODISCARD inline std::u16string latin1ToUtf16(const std::string_view sv)
{
    Utf16StringBuilder sb;
    foreach_codepoint_latin1(sv, [&sb](const uint32_t codepoint) { sb += codepoint; });
    return sb.steal_buffer();
}

NODISCARD inline std::u16string utf8ToUtf16(const std::string_view sv)
{
    Utf16StringBuilder sb;
    foreach_codepoint_utf8(sv, [&sb](const uint32_t codepoint) { sb += codepoint; });
    return sb.steal_buffer();
}

} // namespace conversion::utf16

namespace conversion::utf16 {

template<typename Callback>
void foreach_codepoint_utf16_no_surroages(const std::u16string_view sv, Callback &&callback)
{
    assert(!containsSurrogates(sv));
    for (const char16_t codepoint : sv) {
        callback(codepoint);
    }
}

template<typename Callback>
void foreach_codepoint_utf16(const std::u16string_view sv,
                             Callback &&callback,
                             const char32_t invalid = charset_detail::DEFAULT_UNMAPPED_CHARACTER)
{
    auto callback2 = [&callback](char32_t codepoint) {
        callback(simple_unicode_translit(codepoint));
    };

    if (!containsSurrogates(sv)) {
        foreach_codepoint_utf16_no_surroages(sv, callback2);
        return;
    }

    for (const char32_t codepoint : conversion::Utf16Iterable{sv, invalid}) {
        callback2(codepoint);
    }
}

template<typename Callback>
void foreach_codepoint_utf16_unfriendly(
    const std::u16string_view sv,
    Callback &&callback,
    const char32_t invalid = charset_detail::DEFAULT_UNMAPPED_CHARACTER)
{
    if (!containsSurrogates(sv)) {
        foreach_codepoint_utf16_no_surroages(sv, std::forward<Callback>(callback));
        return;
    }

    for (const char32_t codepoint : conversion::Utf16Iterable{sv, invalid}) {
        callback(codepoint);
    }
}

NODISCARD inline std::string utf16ToLatin1(const std::u16string_view sv)
{
    Latin1StringBuilder sb;
    foreach_codepoint_utf16(sv, [&sb](const uint32_t codepoint) { sb += codepoint; });
    return sb.steal_buffer();
}

NODISCARD inline std::string utf16ToUtf8(const std::u16string_view sv)
{
    Utf8StringBuilder sb;
    foreach_codepoint_utf16(sv, [&sb](const uint32_t codepoint) { sb += codepoint; });
    return sb.steal_buffer();
}

inline void utf16ToAscii(std::ostream &os, const std::u16string_view sv)
{
    foreach_codepoint_utf16(sv, InsertAscii{os});
}

inline void utf16ToLatin1(std::ostream &os, const std::u16string_view sv)
{
    foreach_codepoint_utf16(sv, InsertLatin1{os});
}

NODISCARD inline std::string utf16ToAscii(const std::u16string_view sv)
{
    auto result = utf16ToLatin1(sv);
    latin1ToAsciiInPlace(result);
    return result;
}

} // namespace conversion::utf16

namespace conversion {
using namespace utf16; // similar to inline namespace
} // namespace conversion

} // namespace charset

namespace charset::conversion {
extern void utf32toUtf8(std::ostream &os, char32_t codepoint);
NODISCARD extern std::string utf32toUtf8(char32_t codepoint);

} // namespace charset::conversion

namespace test {
extern void testCharset();
} // namespace test

using charset::isAscii;
using charset::isPrintLatin1;
using charset::isValidUtf8;
