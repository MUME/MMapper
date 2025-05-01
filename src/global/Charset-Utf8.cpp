// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "Charset.h"
#include "Consts.h"
#include "TextUtils.h"
#include "entities.h"

#include <iostream>
#include <sstream>

// https://en.wikipedia.org/wiki/UTF-8

using namespace char_consts;

namespace charset {
namespace conversion {

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
    //                    C2..C3    80..BF  (hex)
    //
    // 0x80 becomes         C2        80
    // 0xFF becomes         C3        BF
    static constexpr const auto SIX_BIT_MASK = (1u << 6u) - 1u;
    static_assert(SIX_BIT_MASK == 63);
    const auto hi = static_cast<char>(0xC0u | (uc >> 6u));          // C2..C3
    const auto lo = static_cast<char>(0x80u | (uc & SIX_BIT_MASK)); // 80..BF
    const char buf[3]{hi, lo, C_NUL};
    os.write(buf, 2);
}

void latin1ToUtf8(std::ostream &os, std::string_view sv)
{
    assert(isAscii(sv) || !isValidUtf8(sv));
    if ((false)) {
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

std::string latin1ToUtf8(std::string_view sv)
{
    if (isAscii(sv)) {
        return std::string{sv};
    }
    std::ostringstream oss;
    latin1ToUtf8(oss, sv);
    return std::move(oss).str();
}

} // namespace conversion

namespace conversion {
namespace conversion_detail {
static constexpr size_t UTF8_BITS_PER_BYTE = 6;

NODISCARD static inline constexpr uint8_t u8_bottom_bits(const size_t N) noexcept
{
    assert(0 <= N && N < 8);
    return static_cast<uint8_t>((1u << N) - 1u);
}
NODISCARD static inline constexpr uint8_t u8_top_bits(const size_t N) noexcept
{
    assert(0 <= N && N < 8);
    return static_cast<uint8_t>(~u8_bottom_bits(8 - N));
}

template<size_t N>
NODISCARD static inline constexpr uint8_t B() noexcept
{
    static_assert(0 < N && N < 8);
    return u8_bottom_bits(N);
}
static_assert(B<1>() == 0x01);
static_assert(B<2>() == 0x03);
static_assert(B<3>() == 0x07);
static_assert(B<4>() == 0x0F);
static_assert(B<5>() == 0x1F);
static_assert(B<6>() == 0x3F);
static_assert(B<7>() == 0x7F);

template<size_t N>
NODISCARD static inline constexpr uint8_t T() noexcept
{
    static_assert(0 < N && N < 8);
    return u8_top_bits(N);
}
static_assert(T<1>() == 0x80);
static_assert(T<2>() == 0xC0);
static_assert(T<3>() == 0xE0);
static_assert(T<4>() == 0xF0);
static_assert(T<5>() == 0xF8);
static_assert(T<6>() == 0xFC);
static_assert(T<7>() == 0xFE);

template<typename T>
NODISCARD constexpr bool is_utf8_continuation(T uc) noexcept = delete;
template<>
NODISCARD constexpr bool is_utf8_continuation(const uint8_t uc) noexcept
{
    return (uc & 0xC0u) == 0x80u;
}
template<>
NODISCARD constexpr bool is_utf8_continuation(const char c) noexcept
{
    return is_utf8_continuation(static_cast<uint8_t>(c));
}

struct NODISCARD Utf8Range final
{
    char32_t lo = 0, hi = 0;
    NODISCARD constexpr bool contains(char32_t codepoint) const noexcept
    {
        return lo <= codepoint && codepoint <= hi;
    }
};
static constexpr std::array<Utf8Range, 5> utf8_ranges{Utf8Range{},
                                                      {0x0u, 0x7Fu},
                                                      {0x80u, 0x07FFu},
                                                      {0x800u, 0xFFFFu},
                                                      {0x10000u, 0x10FFFFu}};

static_assert(utf8_ranges[0].lo == utf8_ranges[0].hi);
static_assert(utf8_ranges[1].lo == 0);
static_assert(utf8_ranges[1].hi + 1 == utf8_ranges[2].lo);
static_assert(utf8_ranges[2].hi + 1 == utf8_ranges[3].lo);
static_assert(utf8_ranges[3].hi + 1 == utf8_ranges[4].lo);
static_assert(utf8_ranges[4].hi == MAX_UNICODE_CODEPOINT);

static constexpr char32_t utf8_1byte_bits = 7;
static constexpr char32_t utf8_2byte_bits = 5 + UTF8_BITS_PER_BYTE;
static constexpr char32_t utf8_3byte_bits = 4 + 2 * UTF8_BITS_PER_BYTE;
static constexpr char32_t utf8_4byte_bits = 3 + 3 * UTF8_BITS_PER_BYTE;

static_assert(utf8_1byte_bits == 7);
static_assert(utf8_2byte_bits == 11);
static_assert(utf8_3byte_bits == 16);
static_assert(utf8_4byte_bits == 21);

static constexpr char32_t utf8_1byte_num_codepoints = 1u << utf8_1byte_bits;
static constexpr char32_t utf8_2byte_num_codepoints = 1u << utf8_2byte_bits;
static constexpr char32_t utf8_3byte_num_codepoints = 1u << utf8_3byte_bits;
static constexpr char32_t utf8_4byte_num_codepoints = 1u << utf8_4byte_bits;

static constexpr char32_t max_theoretical_codepoint = utf8_4byte_num_codepoints - 1;

static_assert(utf8_ranges[1].hi == utf8_1byte_num_codepoints - 1);
static_assert(utf8_ranges[2].lo == utf8_1byte_num_codepoints);
static_assert(utf8_ranges[2].hi == utf8_2byte_num_codepoints - 1);
static_assert(utf8_ranges[3].lo == utf8_2byte_num_codepoints);
static_assert(utf8_ranges[3].hi == utf8_3byte_num_codepoints - 1);
static_assert(utf8_ranges[4].lo == utf8_3byte_num_codepoints);
static_assert(utf8_ranges[4].hi < max_theoretical_codepoint);

enum class NODISCARD CodePointErrorEnum : uint8_t {
    Success,
    // error: string-view was empty
    Empty,
    // error: first byte was invalid
    InvalidPrefix,
    // error: not enough bytes
    Truncated,
    // error: byte 2 is an invalid continuation
    InvalidContinuation2,
    // error: byte 3 is an invalid continuation
    InvalidContinuation3,
    // error: byte 4 is an invalid continuation
    InvalidContinuation4,
    // error: too many bytes for the codepoint (e.g. 2-byte U+0, etc)
    OverLongEncoding,
    // error: codepoint greater than U+10FFFF
    TooLarge,
    // error: invalid codepoint in the range U+D800 to U+DFFF.
    Utf16Surrogate,
};

struct NODISCARD OptCodepoint final
{
    size_t num_bytes = 0;
    char32_t codepoint = 0;
    CodePointErrorEnum error = CodePointErrorEnum::Success;

    explicit constexpr OptCodepoint() noexcept = default;
    explicit constexpr OptCodepoint(const CodePointErrorEnum error_) noexcept
        : error{error_}
    {}
    explicit constexpr OptCodepoint(const size_t num_bytes_,
                                    const char32_t codepoint_,
                                    const CodePointErrorEnum error_) noexcept
        : num_bytes{num_bytes_}
        , codepoint{codepoint_}
        , error{error_}
    {}
    explicit constexpr OptCodepoint(const size_t num_bytes_, const char32_t codepoint_) noexcept
        : OptCodepoint{num_bytes_, codepoint_, CodePointErrorEnum::Success}
    {}

    NODISCARD constexpr bool valid() const noexcept
    {
        return num_bytes != 0 && error == CodePointErrorEnum::Success;
    }
    NODISCARD explicit constexpr operator bool() const noexcept { return valid(); }
    NODISCARD constexpr bool operator==(const std::nullopt_t) const { return !valid(); }
    NODISCARD constexpr bool operator==(const OptCodepoint other) const
    {
        return num_bytes == other.num_bytes && codepoint == other.codepoint && error == other.error;
    }
};

static constexpr OptCodepoint Utf8ErrEmpty{CodePointErrorEnum::Empty};
static constexpr OptCodepoint Utf8ErrInvalidPrefix{CodePointErrorEnum::InvalidPrefix};
static constexpr OptCodepoint UtfErrTruncated{CodePointErrorEnum::Truncated};
static constexpr OptCodepoint UtfErrInvalidContinuation2{CodePointErrorEnum::InvalidContinuation2};
static constexpr OptCodepoint UtfErrInvalidContinuation3{CodePointErrorEnum::InvalidContinuation3};
static constexpr OptCodepoint UtfErrInvalidContinuation4{CodePointErrorEnum::InvalidContinuation4};

static constexpr std::array<OptCodepoint, 4> UtfErrInvalidContinuations{Utf8ErrInvalidPrefix,
                                                                        UtfErrInvalidContinuation2,
                                                                        UtfErrInvalidContinuation3,
                                                                        UtfErrInvalidContinuation4};

NODISCARD constexpr uint8_t get_num_units(const uint8_t uc) noexcept
{
    // checking the first two bits (the xx in xx yyy zzz)
    switch (uc >> 6u) {
    case 0b0'0:
    case 0b0'1:
        return 1; // ASCII
    case 0b10:
        return 0; // continuation byte
    }

    // checking the next three bits (the yy in 11 yyy zzz)
    //  11 0yy zzz: 2-byte
    //  11 10y zzz: 3-byte
    //  11 110 zzz: 4-byte
    //  11 111 zzz: 5 or more leading bits
    switch ((uc >> 3u) & 0b111u) {
    case 0b0'00:
    case 0b0'01:
    case 0b0'10:
    case 0b0'11:
        return 2;
    case 0b10'0:
    case 0b10'1:
        return 3;
    case 0b110:
        return 4;
    default:
        return 0; // 5 or more leading ones = invalid
    }
}

static_assert(get_num_units(0x7F) == 1); // ASCII
static_assert(get_num_units(0x80) == 0); // continuation
static_assert(get_num_units(0xC0) == 2);
static_assert(get_num_units(0xE0) == 3);
static_assert(get_num_units(0xF0) == 4);
static_assert(get_num_units(0xF8) == 0); // invalid; would be 5
static_assert(get_num_units(0xFC) == 0); // invalid; would be 6
static_assert(get_num_units(0xFE) == 0); // invalid; BOM confusion
static_assert(get_num_units(0xFF) == 0); // invalid; BOM confusion

class NODISCARD Utf8MultiByteDecoder final
{
private:
    uint8_t m_size = 2;

private:
    NODISCARD constexpr uint8_t get_expected_pattern() const noexcept
    {
        return u8_top_bits(m_size);
    }
    NODISCARD constexpr uint8_t get_top_mask() const noexcept { return u8_top_bits(m_size + 1); }
    NODISCARD constexpr uint8_t get_bottom_mask() const noexcept
    {
        return u8_bottom_bits(7 - m_size);
    }

private:
    NODISCARD constexpr bool top_bits_match(const uint8_t uc) const noexcept
    {
        return (uc & get_top_mask()) == get_expected_pattern();
    }
    NODISCARD constexpr uint8_t get_bottom_bits(const uint8_t uc) const noexcept
    {
        return uc & get_bottom_mask();
    }

private:
    // 1-byte: 0aaaaaaa:                          aaaaaaa                      ( 7 bits)
    // 2-byte: 110aaaaa 10bbbbbb:                   aaaaa bbbbbb               (11 bits)
    // 3-byte: 1110aaaa 10bbbbbb 10cccccc:           aaaa bbbbbb cccccc        (16 bits)
    // 4-byte: 11110aaa 10bbbbbb 10cccccc 10dddddd:   aaa bbbbbb cccccc dddddd (21 bits)
    NODISCARD constexpr OptCodepoint try_get_codepoint(const std::string_view sv) noexcept
    {
        if (sv.empty()) {
            return Utf8ErrEmpty;
        }

        const auto uc = static_cast<uint8_t>(sv.front());
        if (utf8_ranges[1].contains(uc)) {
            // ASCII: 0xxxxxxx (7 bits)
            return OptCodepoint{1, static_cast<char32_t>(uc)};
        }

        if (is_utf8_continuation(uc)) {
            // 10xxxxxx
            return Utf8ErrInvalidPrefix;
        }

        const auto size = get_num_units(uc);
        assert(size < utf8_ranges.size());
        if (size < 2) {
            return Utf8ErrInvalidPrefix;
        }

        // important side-effect
        m_size = size;
        return try_get_codepoint_part2(sv);
    }

    NODISCARD constexpr OptCodepoint try_get_codepoint_part2(const std::string_view sv) const noexcept
    {
        const size_t size = m_size;
        if (size > sv.size()) {
            // should we try to report the expected size?
            // there would be a lot of cases (6 total):
            //
            //   expected 2 but got 1;
            //   expected 3 but got 1 or 2;
            //   expected 4 but got 1, 2, or 3.
            //
            // except this error is reported before we know if any
            // of the continuations are valid, so this test would
            // need to be moved into the loop.
            return UtfErrTruncated;
        }

        const auto uc = static_cast<uint8_t>(sv.front());
        if (!top_bits_match(uc)) {
            abort();
        }

        char32_t codepoint = get_bottom_bits(uc);
        for (size_t i = 1; i < size; ++i) {
            const auto next = static_cast<uint8_t>(sv[i]);
            if (!is_utf8_continuation(next)) {
                return UtfErrInvalidContinuations[i];
            }

            // continuation bytes: 10xxxxxx
            codepoint <<= UTF8_BITS_PER_BYTE;
            codepoint |= next & B<UTF8_BITS_PER_BYTE>();
        }

        return OptCodepoint{size, codepoint};
    }

public:
    NODISCARD static constexpr OptCodepoint try_decode(const std::string_view sv) noexcept
    {
        Utf8MultiByteDecoder dec;
        return dec.try_get_codepoint(sv);
    }
};

NODISCARD constexpr bool is16Bit(const uint32_t n) noexcept
{
    return (n & 0xFFFFu) == n;
}

NODISCARD constexpr bool isSurrogate(const char32_t codepoint) noexcept
{
    return is16Bit(codepoint) && utf16_detail::is_utf16_surrogate(static_cast<uint16_t>(codepoint));
}

NODISCARD constexpr OptCodepoint try_match_utf8(const std::string_view sv) noexcept
{
    OptCodepoint opt = Utf8MultiByteDecoder::try_decode(sv);
    if (!opt) {
        return opt;
    }

    if (isSurrogate(opt.codepoint)) {
        opt.error = CodePointErrorEnum::Utf16Surrogate;
    } else if (const Utf8Range range = utf8_ranges[opt.num_bytes]; !range.contains(opt.codepoint)) {
        opt.error = (opt.codepoint < range.lo) ? CodePointErrorEnum::OverLongEncoding
                                               : CodePointErrorEnum::TooLarge;
    }

    return opt;
}

static_assert(try_match_utf8("") == Utf8ErrEmpty);
static_assert(try_match_utf8("\x80") == Utf8ErrInvalidPrefix);
static_assert(try_match_utf8("\xF8") == Utf8ErrInvalidPrefix);
static_assert(try_match_utf8("\xFC") == Utf8ErrInvalidPrefix);
static_assert(try_match_utf8("\xFE") == Utf8ErrInvalidPrefix);
static_assert(try_match_utf8("\xFF") == Utf8ErrInvalidPrefix);
static_assert(try_match_utf8("\xC0") == UtfErrTruncated);
static_assert(try_match_utf8("\xCF") == UtfErrTruncated);
static_assert(try_match_utf8("\xD0") == UtfErrTruncated);
static_assert(try_match_utf8("\xDF") == UtfErrTruncated);
static_assert(try_match_utf8("\xE0") == UtfErrTruncated);
static_assert(try_match_utf8("\xEF") == UtfErrTruncated);
static_assert(try_match_utf8("\xF0") == UtfErrTruncated);
static_assert(try_match_utf8("\xF7") == UtfErrTruncated);
static_assert(try_match_utf8("\xC2\x7F") == UtfErrInvalidContinuation2);
static_assert(try_match_utf8("\xC2\xC0") == UtfErrInvalidContinuation2);

// over-long
static_assert(try_match_utf8("\xC0\x80")
              == OptCodepoint{2, 0, CodePointErrorEnum::OverLongEncoding});
static_assert(try_match_utf8("\xE0\x80\x80")
              == OptCodepoint{3, 0, CodePointErrorEnum::OverLongEncoding});
static_assert(try_match_utf8("\xF0\x80\x80\x80")
              == OptCodepoint{4, 0, CodePointErrorEnum::OverLongEncoding});

// 1 byte
static constexpr std::array<char, 2> zero{0, 0};
static_assert(try_match_utf8(std::string_view{zero.data(), 1}) == OptCodepoint{1, 0x0});
static_assert(try_match_utf8("\x1") == OptCodepoint{1, 0x1});
static_assert(try_match_utf8("\x7F") == OptCodepoint{1, 0x7F});

// 2 bytes
static_assert(try_match_utf8("\xC0\x80")
              == OptCodepoint{2, 0, CodePointErrorEnum::OverLongEncoding});
static_assert(try_match_utf8("\xC1\xBF")
              == OptCodepoint{2, 0x7F, CodePointErrorEnum::OverLongEncoding});
static_assert(try_match_utf8("\xC2\x80") == OptCodepoint{2, 0x80});
static_assert(try_match_utf8("\xDF\xBF") == OptCodepoint{2, 0x7Ff});

// 3 bytes
static_assert(try_match_utf8("\xE0\x7F\x7F") == UtfErrInvalidContinuation2);
static_assert(try_match_utf8("\xE0\x80\x7F") == UtfErrInvalidContinuation3);
static_assert(try_match_utf8("\xE0\x80\x80")
              == OptCodepoint{3, 0, CodePointErrorEnum::OverLongEncoding});
static_assert(try_match_utf8("\xE0\x9F\xBF")
              == OptCodepoint{3, 0x7Ff, CodePointErrorEnum::OverLongEncoding});
static_assert(try_match_utf8("\xE0\xA0\x80") == OptCodepoint{3, 0x800});
// U+D800-U+DFFF
static_assert(try_match_utf8("\xED\x9F\xBF") == OptCodepoint{3, utf16_detail::FIRST_SURROGATE - 1});
static_assert(try_match_utf8("\xED\xA0\x80")
              == OptCodepoint{3, utf16_detail::FIRST_SURROGATE, CodePointErrorEnum::Utf16Surrogate});
static_assert(try_match_utf8("\xED\xBF\xBF")
              == OptCodepoint{3, utf16_detail::LAST_SURROGATE, CodePointErrorEnum::Utf16Surrogate});
static_assert(try_match_utf8("\xEE\x80\x80") == OptCodepoint{3, utf16_detail::LAST_SURROGATE + 1});
static_assert(try_match_utf8("\xEF\xBB\xBF") == OptCodepoint{3, byte_order_mark});
static_assert(try_match_utf8("\xEF\xBF\xBF") == OptCodepoint{3, 0xFFFF});

// 4 bytes
static_assert(try_match_utf8("\xF0\x7F\x7F\x7F") == UtfErrInvalidContinuation2);
static_assert(try_match_utf8("\xF0\x80\x7F\x7F") == UtfErrInvalidContinuation3);
static_assert(try_match_utf8("\xF0\x80\x80\x7F") == UtfErrInvalidContinuation4);
static_assert(try_match_utf8("\xF0\x80\x80\x80")
              == OptCodepoint{4, 0, CodePointErrorEnum::OverLongEncoding});
static_assert(try_match_utf8("\xF0\x8F\xBF\xBF")
              == OptCodepoint{4, 0xFFFFu, CodePointErrorEnum::OverLongEncoding});
static_assert(try_match_utf8("\xF0\x90\x80\x80") == OptCodepoint{4, 0x10000});
static_assert(try_match_utf8("\xF0\x9F\x91\x8D") == OptCodepoint{4, thumbs_up});
static_assert(try_match_utf8("\xF4\x8F\xBF\xBF") == OptCodepoint{4, MAX_UNICODE_CODEPOINT});
static_assert(try_match_utf8("\xF4\x90\x80\x80")
              == OptCodepoint{4, MAX_UNICODE_CODEPOINT + 1, CodePointErrorEnum::TooLarge});

} // namespace conversion_detail

NODISCARD std::optional<char32_t> try_match_utf8(std::string_view sv) noexcept
{
    if (const auto opt = conversion_detail::try_match_utf8(sv)) {
        return opt.codepoint;
    }
    return std::nullopt;
}

NODISCARD std::optional<char32_t> try_pop_utf8(std::string_view &sv) noexcept
{
    if (const auto opt = conversion_detail::try_match_utf8(sv)) {
        sv.remove_prefix(opt.num_bytes);
        return opt.codepoint;
    }
    return std::nullopt;
}

} // namespace conversion

namespace charset_detail {

NODISCARD static constexpr bool is7bit(const char c) noexcept
{
    return (static_cast<uint8_t>(c) & 0x80u) == 0u;
}
NODISCARD static constexpr bool is7bit(const std::string_view sv) noexcept
{
    // NOLINTNEXTLINE (can't use std::all_of() in constexpr in c++17)
    for (const char c : sv) {
        if (!is7bit(c)) {
            return false;
        }
    }
    return true;
}

NODISCARD static constexpr Utf8ValidationEnum validateUtf8(std::string_view sv) noexcept
{
    if (is7bit(sv)) {
        return Utf8ValidationEnum::Valid;
    }

    bool containsInvalidEncodings = false;
    while (!sv.empty()) {
        auto opt = conversion::conversion_detail::try_match_utf8(sv);
        if (opt.num_bytes == 0) {
            return Utf8ValidationEnum::ContainsErrors;
        }
        switch (opt.error) {
        case conversion::conversion_detail::CodePointErrorEnum::Success:
            break;
        case conversion::conversion_detail::CodePointErrorEnum::Empty:
        case conversion::conversion_detail::CodePointErrorEnum::InvalidPrefix:
        case conversion::conversion_detail::CodePointErrorEnum::Truncated:
        case conversion::conversion_detail::CodePointErrorEnum::InvalidContinuation2:
        case conversion::conversion_detail::CodePointErrorEnum::InvalidContinuation3:
        case conversion::conversion_detail::CodePointErrorEnum::InvalidContinuation4:
            return Utf8ValidationEnum::ContainsErrors;
        case conversion::conversion_detail::CodePointErrorEnum::OverLongEncoding:
        case conversion::conversion_detail::CodePointErrorEnum::TooLarge:
        case conversion::conversion_detail::CodePointErrorEnum::Utf16Surrogate:
            containsInvalidEncodings = true;
            break;
        }
        sv.remove_prefix(opt.num_bytes);
    }
    return containsInvalidEncodings ? Utf8ValidationEnum::ContainsInvalidEncodings
                                    : Utf8ValidationEnum::Valid;
}

NODISCARD static constexpr bool isValidUtf8(std::string_view sv)
{
    return validateUtf8(sv) == Utf8ValidationEnum::Valid;
}
NODISCARD static constexpr bool hasInvalidEncodings(std::string_view sv)
{
    return validateUtf8(sv) == Utf8ValidationEnum::ContainsInvalidEncodings;
}
NODISCARD static constexpr bool hasErrors(std::string_view sv)
{
    return validateUtf8(sv) == Utf8ValidationEnum::ContainsErrors;
}

static_assert(isValidUtf8("\x00")); // U+0000 (ascii 0x00)
static_assert(isValidUtf8("\x7F")); // U+007f (ascii 0x7F)
//
static_assert(hasErrors("\x80"));
static_assert(hasErrors("\xBF"));
//
static_assert(hasInvalidEncodings("\xC0\x80")); // invalid over-long encoding
static_assert(hasInvalidEncodings("\xC1\x80")); // invalid over-long encoding
//
static_assert(isValidUtf8("\xC2\x80")); // U+  80 (latin1 0x80)
static_assert(isValidUtf8("\xC2\xBF")); // U+  BF (latin1 0xBF)
static_assert(isValidUtf8("\xC3\x80")); // U+  C0 (latin1 0xC0)
static_assert(isValidUtf8("\xC3\xBF")); // U+  FF (latin1 0xFF)
static_assert(isValidUtf8("\xC4\x80")); // U+0100
static_assert(isValidUtf8("\xDF\x80")); // U+07C0
static_assert(isValidUtf8("\xDF\xBF")); // U+07FF
//
static_assert(hasInvalidEncodings("\xE0\x80\x80")); // U+0800 (* not all in this range are valid)
static_assert(isValidUtf8("\xE1\x80\x80"));         // U+1000
static_assert(isValidUtf8("\xEC\x80\x80"));         // U+C000
static_assert(isValidUtf8("\xED\x80\x80"));         // U+D000 (* not all in this range are valid)
static_assert(hasInvalidEncodings("\xED\xA0\x80")); // U+D800 (start of surrogate range)
static_assert(hasInvalidEncodings("\xED\xBF\xBF")); // U+DFFF (end of surrogate range)
static_assert(isValidUtf8("\xEE\x80\x80"));         // U+E000
static_assert(isValidUtf8("\xEF\x80\x80"));         // U+F000
static_assert(isValidUtf8("\xEF\xBF\xBF"));         // U+FFFF
//
static_assert(hasInvalidEncodings("\xF0\x80\x80\x80")); // U+ 10000 (* not all in this range are valid)
static_assert(isValidUtf8("\xF1\x80\x80\x80"));         // U+ 40000
static_assert(isValidUtf8("\xF2\x80\x80\x80"));         // U+ 80000
static_assert(isValidUtf8("\xF3\x80\x80\x80"));         // U+ C0000
static_assert(isValidUtf8("\xF4\x80\x80\x80")); // U+100000 (* not all in this range are valid)
//
static_assert(hasInvalidEncodings("\xF5\x80\x80\x80")); // U+140000
static_assert(hasInvalidEncodings("\xF6\x80\x80\x80")); // U+180000
static_assert(hasInvalidEncodings("\xF7\x80\x80\x80")); // U+1C0000
//
static_assert(hasErrors("\xF8\x80\x80\x80\x80"));     // illegal 5-byte sequence
static_assert(hasErrors("\xFC\x80\x80\x80\x80\x80")); // illegal 6-byte sequence
static_assert(hasErrors("\xFE"));                     // not valid anywhere
static_assert(hasErrors("\xFF"));                     // not valid anywhere
} // namespace charset_detail

NODISCARD Utf8ValidationEnum validateUtf8(const std::string_view sv) noexcept
{
    return charset_detail::validateUtf8(sv);
}

NODISCARD bool isValidUtf8(const std::string_view sv) noexcept
{
    return charset_detail::isValidUtf8(sv);
}

namespace conversion {
namespace conversion_detail {

template<size_t N>
NODISCARD static inline constexpr uint8_t extract_part(const char32_t codepoint)
{
    return ((codepoint >> (N * UTF8_BITS_PER_BYTE)) & B<UTF8_BITS_PER_BYTE>());
}

template<size_t N>
NODISCARD static inline constexpr uint8_t utf8_make_first_byte(const char32_t codepoint)
{
    return T<N + 1>() | (extract_part<N>(codepoint) & B<UTF8_BITS_PER_BYTE - N>());
}

template<size_t N>
NODISCARD static inline constexpr uint8_t utf8_make_cont_byte(const char32_t codepoint)
{
    return T<1>() | extract_part<N>(codepoint);
}

NODISCARD static constexpr OptionalEncodedUtf8Codepoint encode_utf8_2bytes(
    const char32_t codepoint) noexcept
{
    if (codepoint > utf8_ranges[2].hi) {
        std::abort();
    }

    // 2 bytes: 110xxxxx 10xxxxxx
    const uint8_t a = utf8_make_first_byte<1>(codepoint);
    const uint8_t b = utf8_make_cont_byte<0>(codepoint);
    return OptionalEncodedUtf8Codepoint{a, b};
}

NODISCARD static constexpr OptionalEncodedUtf8Codepoint encode_utf8_3bytes(
    const char32_t codepoint) noexcept
{
    if (codepoint > utf8_ranges[3].hi) {
        std::abort();
    }

    // 3 bytes: 1110xxxx 10xxxxxx 10xxxxxx
    const uint8_t a = utf8_make_first_byte<2>(codepoint);
    const uint8_t b = utf8_make_cont_byte<1>(codepoint);
    const uint8_t c = utf8_make_cont_byte<0>(codepoint);
    return OptionalEncodedUtf8Codepoint{a, b, c};
}

NODISCARD static constexpr OptionalEncodedUtf8Codepoint encode_utf8_4bytes(
    const char32_t codepoint) noexcept
{
    // we permit codepoints greater than U+10FFFF,
    // so this function can be used to construct unit tests.
    if (codepoint > max_theoretical_codepoint) {
        std::abort();
    }

    // 4 bytes: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    const uint8_t a = utf8_make_first_byte<3>(codepoint);
    const uint8_t b = utf8_make_cont_byte<2>(codepoint);
    const uint8_t c = utf8_make_cont_byte<1>(codepoint);
    const uint8_t d = utf8_make_cont_byte<0>(codepoint);
    return OptionalEncodedUtf8Codepoint{a, b, c, d};
}

NODISCARD constexpr OptionalEncodedUtf8Codepoint try_encode_utf8(const char32_t codepoint) noexcept
{
    if (utf8_ranges[1].contains(codepoint)) {
        return OptionalEncodedUtf8Codepoint{static_cast<uint8_t>(codepoint)};
    }

    if (utf8_ranges[2].contains(codepoint)) {
        return encode_utf8_2bytes(codepoint);
    }

    if (utf8_ranges[3].contains(codepoint)) {
        if (utf16_detail::is_utf16_surrogate(static_cast<uint16_t>(codepoint))) {
            return OptionalEncodedUtf8Codepoint{};
        }
        return encode_utf8_3bytes(codepoint);
    }

    if (utf8_ranges[4].contains(codepoint)) {
        return encode_utf8_4bytes(codepoint);
    }

    return OptionalEncodedUtf8Codepoint{};
}

NODISCARD static constexpr OptionalEncodedUtf8Codepoint try_encode_utf8_unchecked2(
    const char32_t codepoint) noexcept
{
    if (codepoint > utf8_ranges[2].hi) {
        return OptionalEncodedUtf8Codepoint{};
    }
    return encode_utf8_2bytes(codepoint);
}
NODISCARD static constexpr OptionalEncodedUtf8Codepoint try_encode_utf8_unchecked3(
    const char32_t codepoint) noexcept
{
    if (codepoint > utf8_ranges[3].hi) {
        return OptionalEncodedUtf8Codepoint{};
    }
    return encode_utf8_3bytes(codepoint);
}
NODISCARD static constexpr OptionalEncodedUtf8Codepoint try_encode_utf8_unchecked4(
    const char32_t codepoint) noexcept
{
    if (codepoint > max_theoretical_codepoint) {
        return OptionalEncodedUtf8Codepoint{};
    }
    return encode_utf8_4bytes(codepoint);
}

NODISCARD static constexpr OptionalEncodedUtf8Codepoint try_encode_utf8_unchecked(
    const char32_t codepoint, const size_t bytes) noexcept
{
    switch (bytes) {
    case 2:
        return try_encode_utf8_unchecked2(codepoint);
    case 3:
        return try_encode_utf8_unchecked3(codepoint);
    case 4:
        return try_encode_utf8_unchecked4(codepoint);
    default:
        return OptionalEncodedUtf8Codepoint{};
    }
}

static_assert(try_encode_utf8(0).size() == 1);
static_assert(try_encode_utf8(0).rvalue().front() == 0);
static_assert(try_encode_utf8('x') == "x");
static_assert(try_encode_utf8(0x7F).size() == 1);
static_assert(try_encode_utf8(0x7F) == "\x7F");
static_assert(try_encode_utf8(0x80).size() == 2);
static_assert(try_encode_utf8(0x80) == "\xC2\x80");
static_assert(try_encode_utf8(0x7Ff).size() == 2);
static_assert(try_encode_utf8(0x7Ff) == "\xDF\xBF");
static_assert(try_encode_utf8(0x800).size() == 3);
static_assert(try_encode_utf8(0x800) == "\xE0\xA0\x80");
static_assert(try_encode_utf8(utf16_detail::FIRST_SURROGATE - 1).size() == 3);
static_assert(try_encode_utf8(utf16_detail::FIRST_SURROGATE) == std::nullopt);
static_assert(try_encode_utf8(utf16_detail::LAST_SURROGATE) == std::nullopt);
static_assert(try_encode_utf8(utf16_detail::LAST_SURROGATE + 1).size() == 3);
static_assert(try_encode_utf8(byte_order_mark) == "\xEF\xBB\xBF");
static_assert(try_encode_utf8(0xFFFF).size() == 3);
static_assert(try_encode_utf8(0xFFFF) == "\xEF\xBF\xBF");
static_assert(try_encode_utf8(0x10000).size() == 4);
static_assert(try_encode_utf8(0x10000) == "\xF0\x90\x80\x80");
static_assert(try_encode_utf8(thumbs_up) == "\xF0\x9F\x91\x8D");
static_assert(try_encode_utf8(MAX_UNICODE_CODEPOINT).size() == 4);
static_assert(try_encode_utf8(MAX_UNICODE_CODEPOINT) == "\xF4\x8F\xBF\xBF");
static_assert(try_encode_utf8(MAX_UNICODE_CODEPOINT + 1) == std::nullopt);

namespace { // anonymous
constexpr auto overlong = try_encode_utf8_unchecked(0x7Fu, 2);
static_assert(overlong == "\xC1\xBF");
static_assert(try_match_utf8(overlong.value())
              == OptCodepoint{2, 0x7fu, CodePointErrorEnum::OverLongEncoding});
} // namespace

namespace { // anonymous
static_assert(!try_encode_utf8_unchecked(utf16_detail::FIRST_SURROGATE, 2));

constexpr auto invalid_surrogate3 = try_encode_utf8_unchecked(utf16_detail::FIRST_SURROGATE, 3);
static_assert(invalid_surrogate3 == "\xED\xA0\x80");

constexpr auto invalid_surrogate = try_encode_utf8_unchecked(utf16_detail::FIRST_SURROGATE, 4);
static_assert(invalid_surrogate == "\xF0\x8D\xA0\x80");

// it's both overlong and a surrogate, but the fact that it's a surrogate is the "more important"
// reason why it's not valid.
static_assert(try_match_utf8(invalid_surrogate.value())
              == OptCodepoint{4, utf16_detail::FIRST_SURROGATE, CodePointErrorEnum::Utf16Surrogate});
} // namespace

namespace { // anonymous
static_assert(!try_encode_utf8_unchecked(MAX_UNICODE_CODEPOINT + 1, 3));
constexpr auto too_big = try_encode_utf8_unchecked(MAX_UNICODE_CODEPOINT + 1, 4);
static_assert(too_big == "\xF4\x90\x80\x80");
static_assert(try_match_utf8(too_big.value())
              == OptCodepoint{4, MAX_UNICODE_CODEPOINT + 1, CodePointErrorEnum::TooLarge});
} // namespace

NODISCARD static constexpr bool utf8_check_roundtrip(const OptCodepoint ocp) noexcept
{
    if (const auto result = try_encode_utf8(ocp.codepoint)) {
        if (result.lvalue().length() != ocp.num_bytes) {
            return false;
        }
        const auto rtt = try_match_utf8(result.lvalue());
        return ocp.valid() && rtt == ocp;
    } else {
        return !ocp.valid();
    }
}

// 1-byte: U+0 to U+7F
static_assert(utf8_check_roundtrip(OptCodepoint{1, utf8_ranges[1].lo}));
static_assert(utf8_check_roundtrip(OptCodepoint{1, utf8_ranges[1].hi}));

// 2-byte: U+80 to U+7FF
static_assert(utf8_check_roundtrip(OptCodepoint{2, utf8_ranges[2].lo}));
static_assert(utf8_check_roundtrip(OptCodepoint{2, utf8_ranges[2].hi}));

// 3-byte: U+800 to U+FFFF
static_assert(utf8_check_roundtrip(OptCodepoint{3, utf8_ranges[3].lo}));
static_assert(utf8_check_roundtrip(OptCodepoint{3, utf8_ranges[3].hi}));

// 4-byte U+10000 to U+10FFFF
static_assert(utf8_check_roundtrip(OptCodepoint{4, utf8_ranges[4].lo}));
static_assert(utf8_check_roundtrip(OptCodepoint{4, utf8_ranges[4].hi}));
static_assert(MAX_UNICODE_CODEPOINT == utf8_ranges[4].hi);

// misc
static_assert(utf8_check_roundtrip(OptCodepoint{3, byte_order_mark}));
static_assert(utf8_check_roundtrip(OptCodepoint{4, thumbs_up}));

// note: 3-byte codepoints in the range of the utf16 surrogate pairs aren't valid:
static_assert(utf8_check_roundtrip(OptCodepoint{3, utf16_detail::FIRST_SURROGATE - 1}));
static_assert(utf8_check_roundtrip(
    OptCodepoint{3, utf16_detail::FIRST_SURROGATE, CodePointErrorEnum::Utf16Surrogate}));
static_assert(utf8_check_roundtrip(
    OptCodepoint{3, utf16_detail::LAST_SURROGATE, CodePointErrorEnum::Utf16Surrogate}));
static_assert(utf8_check_roundtrip(OptCodepoint{3, utf16_detail::LAST_SURROGATE + 1}));

// note: 4-byte codepoints greater than U+10FFFF aren't valid:
static_assert(
    utf8_check_roundtrip(OptCodepoint{4, MAX_UNICODE_CODEPOINT + 1, CodePointErrorEnum::TooLarge}));
static_assert(
    utf8_check_roundtrip(OptCodepoint{4, max_theoretical_codepoint, CodePointErrorEnum::TooLarge}));

} // namespace conversion_detail

NODISCARD OptionalEncodedUtf8Codepoint try_encode_utf8(const char32_t codepoint) noexcept
{
    return conversion_detail::try_encode_utf8(codepoint);
}

NODISCARD OptionalEncodedUtf8Codepoint try_encode_utf8_unchecked(const char32_t codepoint,
                                                                 const size_t bytes) noexcept
{
    return conversion_detail::try_encode_utf8_unchecked(codepoint, bytes);
}

std::string utf8ToAscii(const std::string_view sv)
{
    assert(isValidUtf8(sv));
    if (isAscii(sv)) {
        return std::string{sv};
    }
    std::ostringstream oss;
    utf8ToAscii(oss, sv);
    return std::move(oss).str();
}
std::string utf8ToLatin1(const std::string_view sv)
{
    assert(isValidUtf8(sv));
    if (isAscii(sv)) {
        return std::string{sv};
    }
    std::ostringstream oss;
    utf8ToLatin1(oss, sv);
    return std::move(oss).str();
}

void utf32toUtf8(std::ostream &os, const char32_t codepoint)
{
    if (auto tmp = try_encode_utf8(codepoint)) {
        os << tmp.value();
    } else {
        os << charset_detail::DEFAULT_UNMAPPED_CHARACTER;
    }
}

NODISCARD std::string utf32toUtf8(const char32_t codepoint)
{
    if (auto tmp = try_encode_utf8(codepoint)) {
        return std::string{tmp.value()};
    } else {
        const char buf[2]{charset_detail::DEFAULT_UNMAPPED_CHARACTER, C_NUL};
        return std::string{static_cast<const char *>(buf), size_t{1u}};
    }
}

} // namespace conversion
} // namespace charset
