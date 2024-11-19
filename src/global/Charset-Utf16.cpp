// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "Charset.h"
#include "entities.h"

// https://en.wikipedia.org/wiki/UTF-16

namespace charset::conversion {

namespace utf16::conversion_utf16_detail {

NODISCARD static constexpr char32_t compute_surrogate(const char16_t hi, const char16_t lo) noexcept
{
    using namespace utf16_detail;
    if (!is_utf16_hi_surrogate(hi) || !is_utf16_lo_surrogate(lo)) {
        std::abort();
    }

    // note: you must ADD the 0x10000 (not Bit-OR it).
    // 0001 0000 0000 0000 0000 (aka 0x10000)
    //                        +
    // xxxx xxxx xxyy yyyy yyyy
    // this gives codepoints in the range 0x10000 to 0x10FFFF.
    return SURROGATE_OFFSET
           + ((static_cast<uint32_t>(hi & BOTTOM_TEN_BITS) << BITS_PER_SURROGATE)
              | static_cast<uint32_t>(lo & BOTTOM_TEN_BITS));
}

static_assert(compute_surrogate(utf16_detail::HI_SURROGATE_MIN, utf16_detail::LO_SURROGATE_MIN)
              == utf16_detail::SURROGATE_OFFSET);
static_assert(compute_surrogate(utf16_detail::HI_SURROGATE_MAX, utf16_detail::LO_SURROGATE_MAX)
              == MAX_UNICODE_CODEPOINT);

struct NODISCARD OptCodepoint final
{
    uint8_t num_bytes = 0;
    char32_t codepoint = 0;

    explicit constexpr OptCodepoint() = default;
    explicit constexpr OptCodepoint(const uint8_t num_bytes_, const char32_t codepoint_)
        : num_bytes{num_bytes_}
        , codepoint{codepoint_}
    {}

    NODISCARD constexpr bool has_value() const noexcept { return num_bytes != 0; }
    NODISCARD constexpr explicit operator bool() const noexcept { return has_value(); }

    NODISCARD constexpr bool operator==(const OptCodepoint other) const noexcept
    {
        return num_bytes == other.num_bytes && codepoint == other.codepoint;
    }
};

NODISCARD static constexpr OptCodepoint try_match_utf16(const std::u16string_view sv) noexcept
{
    using namespace utf16_detail;
    if (sv.empty()) {
        return OptCodepoint{};
    }

    const auto uc = static_cast<uint16_t>(sv.front());
    if (!is_utf16_surrogate(uc)) {
        return OptCodepoint{1, static_cast<char32_t>(uc)};
    }

    if (sv.length() < 2) {
        return OptCodepoint{};
    }

    const auto hi = uc;
    const auto lo = static_cast<uint16_t>(sv[1]);
    if (!is_utf16_hi_surrogate(hi) || !is_utf16_lo_surrogate(lo)) {
        return OptCodepoint{};
    }

    const char32_t result = conversion_utf16_detail::compute_surrogate(hi, lo);
    return OptCodepoint{2, result};
}

static_assert(try_match_utf16(u"x") == OptCodepoint{1, 'x'});
static_assert(try_match_utf16(u"\xD83D\xDC4D") == OptCodepoint{2, char_consts::thumbs_up});
static_assert(try_match_utf16(u"\xD83D") == OptCodepoint{});       // only the first surrogate
static_assert(try_match_utf16(u"\xDC4D\xD83D") == OptCodepoint{}); // swapped order

} // namespace utf16::conversion_utf16_detail

NODISCARD std::optional<char32_t> try_match_utf16(const std::u16string_view sv) noexcept
{
    if (const auto opt = conversion_utf16_detail::try_match_utf16(sv)) {
        return opt.codepoint;
    }
    return std::nullopt;
}

NODISCARD std::optional<char32_t> try_pop_utf16(std::u16string_view &sv) noexcept
{
    if (const auto opt = conversion_utf16_detail::try_match_utf16(sv)) {
        sv.remove_prefix(opt.num_bytes);
        return opt.codepoint;
    }
    return std::nullopt;
}

namespace utf16::conversion_utf16_detail {
NODISCARD static constexpr OptionalEncodedUtf16Codepoint try_encode_utf16(
    const char32_t codepoint) noexcept
{
    using namespace utf16_detail;
    if (codepoint < SURROGATE_OFFSET) {
        const auto result = static_cast<uint16_t>(codepoint);
        if (is_utf16_surrogate(result)) {
            return OptionalEncodedUtf16Codepoint{};
        }
        return OptionalEncodedUtf16Codepoint{result};
    }

    if (codepoint > MAX_UNICODE_CODEPOINT) {
        return OptionalEncodedUtf16Codepoint{};
    }

    const uint32_t tmp = codepoint - SURROGATE_OFFSET;
    const uint16_t hi = HI_SURROGATE_MIN | ((tmp >> BITS_PER_SURROGATE) & BOTTOM_TEN_BITS);
    const uint16_t lo = LO_SURROGATE_MIN | (tmp & BOTTOM_TEN_BITS);
    return OptionalEncodedUtf16Codepoint{hi, lo};
}

static_assert(try_encode_utf16(0).size() == 1);
static_assert(try_encode_utf16(0).rvalue().front() == 0);
static_assert(try_encode_utf16('x') == u"x");
static_assert(try_encode_utf16(utf16_detail::FIRST_SURROGATE) == std::nullopt);
static_assert(try_encode_utf16(utf16_detail::LAST_SURROGATE) == std::nullopt);
static_assert(try_encode_utf16(char_consts::thumbs_up) == u"\xD83D\xDC4D"); // thumbs up
static_assert(try_encode_utf16(MAX_UNICODE_CODEPOINT).size() == 2);
static_assert(try_encode_utf16(MAX_UNICODE_CODEPOINT + 1) == std::nullopt);

} // namespace utf16::conversion_utf16_detail

NODISCARD OptionalEncodedUtf16Codepoint utf16::try_encode_utf16(const char32_t codepoint) noexcept
{
    return conversion_utf16_detail::try_encode_utf16(codepoint);
}

} // namespace charset::conversion
