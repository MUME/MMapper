// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "AnsiTextUtils.h"

#include "CharUtils.h"
#include "ConfigConsts.h"
#include "Consts.h"
#include "EnumIndexedArray.h"
#include "Flags.h"
#include "LineUtils.h"
#include "PrintUtils.h"
#include "logging.h"
#include "tests.h"
#include "utils.h"

#include <array>
#include <cassert>
#include <charconv>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <ostream>
#include <sstream>
#include <vector>

#include <QRegularExpression>
#include <QString>

static const volatile bool verbose_debugging = false;
static constexpr const int ANSI_RESET = 0;
static constexpr const int ANSI_REVERT_OFFSET = 20;

#define X_DECL(_n, _lower, _UPPER, _Snake) \
    MAYBE_UNUSED static constexpr const int ANSI_##_UPPER{_n}; \
    MAYBE_UNUSED static constexpr const int ANSI_##_UPPER##_OFF{ANSI_REVERT_OFFSET + (_n)};
XFOREACH_ANSI_STYLE(X_DECL)
#undef X_DECL

static constexpr const int ANSI_EXT_RGB = 2;
static constexpr const int ANSI_EXT_256 = 5;
static constexpr const int ANSI_FG_COLOR = 30;
static constexpr const int ANSI_FG_EXT = 38;
static constexpr const int ANSI_FG_DEFAULT = 39;
static constexpr const int ANSI_BG_COLOR = 40;
static constexpr const int ANSI_BG_EXT = 48;
static constexpr const int ANSI_BG_DEFAULT = 49;
static constexpr const int ANSI_UL_EXT = 58;
static constexpr const int ANSI_UL_DEFAULT = 59;
static constexpr const int ANSI_FG_COLOR_HI = 90;
static constexpr const int ANSI_BG_COLOR_HI = 100;

static constexpr const int ANSI256_HI_OFFSET = 8; // high colors
static constexpr const int ANSI256_RGB6_START = 16;
static constexpr const int ANSI256_RGB6_END = 231;
static constexpr const int ANSI256_GREY_START = ANSI256_RGB6_END + 1;
static constexpr const int ANSI256_GREY_END = static_cast<int>(std::numeric_limits<uint8_t>::max());

static_assert(ANSI256_RGB6_START == 2 * ANSI256_HI_OFFSET);
static_assert(ANSI256_RGB6_END - ANSI256_RGB6_START + 1 == 6 * 6 * 6); // 6-bit RGB color
static_assert(ANSI256_GREY_END - ANSI256_GREY_START + 1 == 24);        // 24 levels of greyscale

// values from https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_(Select_Graphic_Rendition)_parameters
namespace wiki {
constexpr std::array<uint8_t, 6> ansi_colors6{0x00, 0x57, 0x87, 0xAF, 0xD7, 0xFF};
static constexpr const std::array<int, 24> ansi_greys24{
    0x08, 0x12, 0x1C, 0x26, 0x30, 0x3A, 0x44, 0x4E, 0x58, 0x62, 0x6C, 0x76,
    0x80, 0x8A, 0x94, 0x9E, 0xA8, 0xB2, 0xBC, 0xC6, 0xD0, 0xDA, 0xE4, 0xEE,
};

} // namespace wiki

namespace mmqt {

/* visible */
const QRegularExpression weakAnsiRegex(R"(\x1B\[?[[:digit:];:]*[[:alpha:]]?)");

} // namespace mmqt

using namespace char_consts;
static constexpr const char C_ANSI_ESCAPE = C_ESC;

NODISCARD static int parsePositiveInt(const QStringView number)
{
    static constexpr int MAX = std::numeric_limits<int>::max();
    static_assert(MAX == 2147483647);
    static constexpr int LIMIT = MAX / 10;
    static_assert(LIMIT == 214748364);

    int n = 0;
    for (const QChar qc : number) {
        // bounds check for sanity
        if (qc.unicode() > 0xFF) {
            return -1;
        }

        const char c = qc.toLatin1();
        if (c < '0' || c > '9') {
            return -1;
        }

        const int digit = c - '0';

        // test if it's safe to multiply without integer overflow
        if (n > LIMIT) {
            return -1;
        }

        n *= 10;

        // test if it's safe to add without integer overflow.
        // e.g. This fails if n == 2147483640, and digit == 8,
        // since 2147483647 - 8 == 2147483639.
        if (n > MAX - digit) {
            return -1;
        }

        n += digit;
    }

    return n;
}

std::string_view to_string_view(const AnsiStyleFlagEnum flag, const bool uppercase)
{
#define X_CASE(_n, _lower, _UPPER, _Snake) \
    case AnsiStyleFlagEnum::_Snake: \
        return uppercase ? #_UPPER : #_lower;
    switch (flag) {
        XFOREACH_ANSI_STYLE(X_CASE)
    }
    assert(false);
    return "*error*";
#undef X_CASE
}

int getIndex16(const AnsiColor16Enum color)
{
#define X_CASE(n, lower, UPPER) \
    case AnsiColor16Enum::lower: \
        return (n); \
    case AnsiColor16Enum::UPPER: \
        return (n) + ANSI256_HI_OFFSET;
    switch (color) {
        XFOREACH_ANSI_COLOR_0_7(X_CASE) //
    default:
        break;
    }
    assert(false);
    return 0;
#undef X_CASE
}

bool isLow(const AnsiColor16Enum color)
{
    return getIndex16(color) < ANSI256_HI_OFFSET;
}

bool isHigh(const AnsiColor16Enum color)
{
    return !isLow(color);
}

AnsiColor16Enum toLow(const AnsiColor16Enum color)
{
#define X_CASE(n, lower, UPPER) \
    case AnsiColor16Enum::lower: \
    case AnsiColor16Enum::UPPER: \
        return AnsiColor16Enum::lower;
    switch (color) {
        XFOREACH_ANSI_COLOR_0_7(X_CASE) //
    default:
        break;
    }
    assert(false);
    return AnsiColor16Enum::black;
#undef X_CASE
}

AnsiColor16Enum toHigh(const AnsiColor16Enum color)
{
#define X_CASE(n, lower, UPPER) \
    case AnsiColor16Enum::lower: \
    case AnsiColor16Enum::UPPER: \
        return AnsiColor16Enum::UPPER;
    switch (color) {
        XFOREACH_ANSI_COLOR_0_7(X_CASE) //
    default:
        break;
    }
    assert(false);
    return AnsiColor16Enum::BLACK;
#undef X_CASE
}

AnsiString::AnsiString()
{
    // should fit in small string optimization
    m_buffer.reserve(8);
    assert(isEmpty());
}

void AnsiString::add_code_common(const bool useItuColon)
{
    if (m_buffer.empty()) {
        m_buffer += C_ANSI_ESCAPE;
        m_buffer += C_OPEN_BRACKET;
    } else {
        assert(m_buffer.back() == 'm');
        m_buffer.back() = useItuColon ? C_COLON : C_SEMICOLON;
    }
}

void AnsiString::add_empty_code(const bool useItuColon)
{
    add_code_common(useItuColon);
    m_buffer += 'm';
}

void AnsiString::add_code(const int code, const bool useItuColon)
{
    add_code_common(useItuColon);

    char tmp[16];
    std::snprintf(tmp, sizeof(tmp), "%dm", code);
    m_buffer += tmp;
}

/// e.g. ESC[1m -> ESC[0;1m
AnsiString AnsiString::copy_as_reset() const
{
    if (size() >= 3 && m_buffer[2] == '0') {
        assert(m_buffer[0] == C_ANSI_ESCAPE);
        assert(m_buffer[1] == C_OPEN_BRACKET);
        /* already starts with ESC[0 */
        return *this;
    }

    AnsiString result;
    result.add_code(ANSI_RESET);
    if (size() >= 2) {
        assert(m_buffer[0] == C_ANSI_ESCAPE);
        assert(m_buffer[1] == C_OPEN_BRACKET);
        assert(result.size() == 4);
        assert(result.m_buffer[0] == C_ANSI_ESCAPE);
        assert(result.m_buffer[1] == C_OPEN_BRACKET);
        assert(result.m_buffer[2] == '0');
        assert(result.m_buffer[3] == 'm');
        result.m_buffer.pop_back(); // 'm'
        result.m_buffer += C_SEMICOLON;
        result.m_buffer += std::string_view{m_buffer}.substr(2); /* skip ESC[ */
        assert(result.size() == size() + 2);
    }
    return result;
}

AnsiString AnsiString::get_reset_string()
{
    AnsiString reset;
    reset.add_code(ANSI_RESET);
    return reset;
}

void AnsiColorState::receive(const int n)
{
    switch (m_state) {
    case InternalStateEnum::Normal:
        state_normal(n);
        break;
    case InternalStateEnum::Ext:
        state_ext(n);
        break;
    case InternalStateEnum::Ext256:
        state_ext256(n);
        break;
    case InternalStateEnum::ExtRGB:
        state_extRGB(n);
        break;
    case InternalStateEnum::Fail:
        break;
    }
}

void AnsiColorState::receive_itu(const AnsiItuColorCodes &codes)
{
    if (codes.empty()) {
        assert(false);
        return;
    } else if (codes.overflowed()) {
        m_state = InternalStateEnum::Fail;
        return;
    } else if (m_state != InternalStateEnum::Normal) {
        m_state = InternalStateEnum::Fail;
        return;
    }

    switch (const int first = codes.front()) {
    case ANSI_UNDERLINE:
        if (codes.size() == 2) {
#define X_CASE(_n, _lower, _UPPER, _Snake) \
    case (_n): \
        m_raw.setUnderlineStyle(AnsiUnderlineStyle::_Snake); \
        return;
            switch (codes[1]) {
                XFOREACH_ANSI_UNDERLINE_TYPE(X_CASE)
            default:
                break;
            }
#undef X_CASE
            receive(ANSI_UNDERLINE);
            assert(m_state == InternalStateEnum::Normal);
        }
        break;

    case ANSI_FG_EXT:
    case ANSI_BG_EXT:
    case ANSI_UL_EXT:
        if (codes.size() < 2) {
            return;
        }
        switch (codes[1]) {
        case ANSI_EXT_256:
            if (codes.size() == 3) {
                for (int x : codes) {
                    receive(x);
                }
                assert(m_state == InternalStateEnum::Normal);
            } else {
                // ignored
            }
            return;

        case ANSI_EXT_RGB:
            // technically it's probably allowed to omit zeros,
            // but we're just going to ignore that.
            if (codes.size() == 5) {
                // invalid, but we'll accept it
                for (int x : codes) {
                    receive(x);
                }
                assert(m_state == InternalStateEnum::Normal);
                return;
            } else if (codes.size() == 6) {
                // codes[2] is actually an ignored color space
                receive(first);        //
                receive(ANSI_EXT_RGB); //
                receive(codes[3]);     // r
                receive(codes[4]);     // g
                receive(codes[5]);     // b
                assert(m_state == InternalStateEnum::Normal);
            } else {
                // ignored
            }
            return;
        default:
            // ignored
            return;
        }
        break;

    default:
        // ignored
        return;
    }
}

static AnsiColorVariant &getColorVariant(RawAnsi &raw, const int ext)
{
    switch (ext) {
    case ANSI_FG_EXT:
    case ANSI_FG_DEFAULT:
        return raw.fg;

    case ANSI_BG_EXT:
    case ANSI_BG_DEFAULT:
        return raw.bg;

    case ANSI_UL_EXT:
    case ANSI_UL_DEFAULT:
        return raw.ul;
    }
    std::abort();
}

static void resetColorVariant(RawAnsi &raw, const int ext)
{
    getColorVariant(raw, ext) = AnsiColorVariant{};
}

// https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_(Select_Graphic_Rendition)_parameters
void AnsiColorState::state_normal(const int n)
{
    switch (n) {
    case 0: // reset
        reset();
        break;

        // REVISIT: support ITU underline styles?

#define X_CASE(_number, _lower, _UPPER, _Snake) \
    case (_number): \
        m_raw.setFlag(AnsiStyleFlagEnum::_Snake); \
        break; \
    case ANSI_REVERT_OFFSET + (_number): \
        m_raw.removeFlag(AnsiStyleFlagEnum::_Snake); \
        break;
        XFOREACH_ANSI_STYLE_EXCEPT_UNDERLINE(X_CASE)
#undef X_CASE

    case ANSI_UNDERLINE:
        m_raw.setUnderline();
        break;
    case ANSI_UNDERLINE_OFF:
        m_raw.clearUnderline();
        break;

    case 6: // fast-blink
        // Note: 25 clears *both* 5 and 6, and 6 is rarely supported,
        // and 26 is proportional spacing (rather than "remove fast blink"),
        // so we can't include 6 in macros with the 1-9 styles.
        // Also, tracking it would add another bit.
        m_raw.setBlink();
        break;

#define X_CASE(X, lower, UPPER) case (ANSI_FG_COLOR + (X)):
        XFOREACH_ANSI_COLOR_0_7(X_CASE)
#undef X_CASE
        m_raw.fg = AnsiColor256{static_cast<uint8_t>(n - ANSI_FG_COLOR)};
        break;

#define X_CASE(X, lower, UPPER) case (ANSI_FG_COLOR_HI + (X)):
        XFOREACH_ANSI_COLOR_0_7(X_CASE)
#undef X_CASE
        m_raw.fg = AnsiColor256{static_cast<uint8_t>(n - ANSI_FG_COLOR_HI + ANSI256_HI_OFFSET)};
        break;

#define X_CASE(X, lower, UPPER) case (ANSI_BG_COLOR + (X)):
        XFOREACH_ANSI_COLOR_0_7(X_CASE)
#undef X_CASE
        m_raw.bg = AnsiColor256{static_cast<uint8_t>(n - ANSI_BG_COLOR)};
        break;
#define X_CASE(X, lower, UPPER) case (ANSI_BG_COLOR_HI + (X)):
        XFOREACH_ANSI_COLOR_0_7(X_CASE)
#undef X_CASE
        m_raw.bg = AnsiColor256{static_cast<uint8_t>(n - ANSI_BG_COLOR_HI + ANSI256_HI_OFFSET)};
        break;

    case ANSI_FG_EXT: // 256 or RGB foreground
    case ANSI_BG_EXT: // 256 or RGB background
    case ANSI_UL_EXT: // 256 or RGB underline
        resetColorVariant(m_raw, n);
        m_state = InternalStateEnum::Ext;
        m_stack.clear();
        m_stack.push(n);
        break;

    case ANSI_FG_DEFAULT:
    case ANSI_BG_DEFAULT:
    case ANSI_UL_DEFAULT:
        resetColorVariant(m_raw, n);
        break;

        // ignored cases; none of these have more than one parameter
        // 4 bits: 11 font choices
        // 1 bit: proportinal spacing
        // 2 bits: framed, encircled (these reset together)
        // 1 bit: overlined
        // 5 bits: ideograms (these reset together)
        // 2 bits: superscript, subscript (these would cancel but not reset if both are set)
        // ---
        // total: we could track these with another 15 bits
    case 10: // Primary (default) font
    case 11: // Alternate font
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20: // Blackletter font
    case 26: // Proportional spacing
    case 50: // Disable proportional spacing 	T.61 and T.416
    case 51: // Framed // Implemented as "emoji variation selector" in mintty.[34]
    case 52: // Encircled
    case 53: // Overlined
    case 54: // Neither framed nor encircled
    case 55: // Not overlined
    case 60: // Ideogram underline or right side line // Rarely supported
    case 61: // Ideogram double underline, or double line on the right side
    case 62: // Ideogram overline or left side line
    case 63: // Ideogram double overline, or double line on the left side
    case 64: // Ideogram stress marking
    case 65: // No ideogram attributes // Reset the effects of all of 60-64
    case 73: // Superscript // Implemented only in mintty[34]
    case 74: // Subscript
        break;

    default:
        m_state = InternalStateEnum::Fail;
        break;
    }
}

void AnsiColorState::state_ext(const int n)
{
    assert(m_stack.size() == 1);
    switch (n) {
    case ANSI_EXT_RGB:
        m_stack.push(n);
        m_state = InternalStateEnum::ExtRGB;
        break;
    case ANSI_EXT_256:
        m_stack.push(n);
        m_state = InternalStateEnum::Ext256;
        break;
    default:
        m_state = InternalStateEnum::Fail;
        break;
    }
}

// ESC[38;5;Nm
// ESC[48;5;Nm
// ESC[58;5;Nm
void AnsiColorState::state_ext256(const int n)
{
    assert(m_stack.size() == 2);
    if (!isClamped(n, 0, 255)) {
        m_state = InternalStateEnum::Fail;
        return;
    }

    m_stack.push(n);

    const auto arr = m_stack.getAndClear<3>();
    assert(arr[1] == ANSI_EXT_256);
    const auto color = AnsiColor256{arr[2]};
    switch (const int x = arr[0]) {
    case ANSI_FG_EXT:
    case ANSI_BG_EXT:
    case ANSI_UL_EXT:
        getColorVariant(m_raw, x) = color;
        break;
    default:
        assert(false);
        break;
    }

    m_state = InternalStateEnum::Normal;
}

// ESC[38;2;r;g;bm
// ESC[48;2;r;g;bm
// ESC[58;2;r;g;bm
void AnsiColorState::state_extRGB(const int n)
{
    assert(m_stack.size() < 5);
    if (!isClamped(n, 0, 255)) {
        m_state = InternalStateEnum::Fail;
        return;
    }

    m_stack.push(n);
    if (m_stack.size() < 5) {
        return;
    }

    const auto arr = m_stack.getAndClear<5>();
    assert(arr[1] == ANSI_EXT_RGB);
    AnsiColorRGB rgb{arr[2], arr[3], arr[4]};
    switch (const int x = arr[0]) {
    case ANSI_FG_EXT:
    case ANSI_BG_EXT:
    case ANSI_UL_EXT:
        getColorVariant(m_raw, x) = rgb;
        break;
    default:
        assert(false);
        break;
    }

    m_state = InternalStateEnum::Normal;
}

struct NODISCARD AnsiEmitter final
{
private:
    enum class NODISCARD WhichEnum : uint8_t { Fore, Back, Under };

private:
    AnsiString result;
    AnsiSupportFlags m_supportFlags;

public:
    explicit AnsiEmitter(const AnsiSupportFlags supportFlags)
        : m_supportFlags{supportFlags}
    {}

public:
    NODISCARD AnsiString getAnsiString(const RawAnsi &before, const RawAnsi &after)
    {
        auto reset = getAnsiString_internal({}, after).copy_as_reset();

        // we must reset if bold is removed because code 21 means double underline
        // to most modern terminals.
        const bool removesBold = before.hasBold() && !after.hasBold();

        if (!removesBold) {
            // if bold isn't removed, then we can just pick the shorter code.
            auto toggle = getAnsiString_internal(before, after);
            if (toggle.size() < reset.size()) {
                return toggle;
            }
        }

        return reset;
    }

private:
    NODISCARD AnsiString getAnsiString_internal(const RawAnsi &before, const RawAnsi &after)
    {
        result = AnsiString{};
        if (before == after) {
            return result;
        }

        if (after == RawAnsi{}) {
            return AnsiString::get_reset_string();
        }

        if (before.hasBold() && !after.hasBold()) {
            // we must reset if bold is removed.
            assert(false);
        }

        if (before.getFlags() != after.getFlags()) {
            const auto added = after.getFlags() & ~before.getFlags();
            const auto removed = before.getFlags() & ~after.getFlags();

            added.for_each([this, &after](AnsiStyleFlagEnum flag) {
                switch (flag) {
#define X_CASE(_number, _lower, _UPPER, _Snake) \
    case AnsiStyleFlagEnum::_Snake: \
        result.add_code(_number); \
        break;
                    XFOREACH_ANSI_STYLE_EXCEPT_UNDERLINE(X_CASE)
                case AnsiStyleFlagEnum::Underline:
                    switch (after.getUnderlineStyle()) {
                    case AnsiUnderlineStyle::None:
                        break;
                    case AnsiUnderlineStyle::Normal:
                    case AnsiUnderlineStyle::Double:
                    case AnsiUnderlineStyle::Curly:
                    case AnsiUnderlineStyle::Dotted:
                    case AnsiUnderlineStyle::Dashed:
                        result.add_code(ANSI_UNDERLINE);
                        if (supportsItuUnderline()) {
                            result.add_code(static_cast<uint8_t>(after.getUnderlineStyle()), true);
                        }
                        break;
                    }
                    break;
#undef X_CASE
                }
            });
            removed.for_each([this](AnsiStyleFlagEnum flag) {
                switch (flag) {
#define X_CASE(_number, _lower, _UPPER, _Snake) \
    case AnsiStyleFlagEnum::_Snake: \
        result.add_code(ANSI_REVERT_OFFSET + (_number)); \
        break;
                    XFOREACH_ANSI_STYLE(X_CASE)
#undef X_CASE
                }
            });
        }

        if (before.fg == after.fg) {
            /* nop */
        } else if (/*before.hasBackgroundColor() &&*/ !after.hasForegroundColor()) {
            result.add_code(ANSI_FG_DEFAULT);
        } else {
            emitColor(WhichEnum::Fore, after.fg);
        }

        if (before.bg == after.bg) {
            /* nop */
        } else if (!after.hasBackgroundColor()) {
            result.add_code(ANSI_BG_DEFAULT);
        } else {
            emitColor(WhichEnum::Back, after.bg);
        }

        if (before.ul == after.ul) {
            /* nop */
        } else if (!after.hasUnderlineColor()) {
            result.add_code(ANSI_UL_DEFAULT);
        } else {
            emitColor(WhichEnum::Under, after.ul);
        }

        return result;
    }

private:
    NODISCARD bool supports16() const
    {
        return m_supportFlags.contains(AnsiSupportFlagEnum::AnsiHI);
    }
    NODISCARD bool supports256() const
    {
        return m_supportFlags.contains(AnsiSupportFlagEnum::Ansi256);
    }
    NODISCARD bool supportsRGB() const
    {
        return m_supportFlags.contains(AnsiSupportFlagEnum::AnsiRGB);
    }
    NODISCARD bool supportsItu256() const
    {
        return m_supportFlags.contains(AnsiSupportFlagEnum::Itu256);
    }
    NODISCARD bool supportsItuRGB() const
    {
        return m_supportFlags.contains(AnsiSupportFlagEnum::ItuRGB);
    }
    NODISCARD bool supportsItuUnderline() const
    {
        return m_supportFlags.contains(AnsiSupportFlagEnum::ItuUnderline);
    }

    void emitColor(const WhichEnum which, const AnsiColorVariant var)
    {
        if (var.has256()) {
            emit256(which, var.get256());
            return;
        }

        if (var.hasRGB()) {
            emitRGB(which, var.getRGB());
            return;
        }

        assert(false);
    }

    void emitExtPrefix(const WhichEnum which)
    {
        switch (which) {
        case WhichEnum::Fore:
            result.add_code(ANSI_FG_EXT);
            break;
        case WhichEnum::Back:
            result.add_code(ANSI_BG_EXT);
            break;
        case WhichEnum::Under:
            result.add_code(ANSI_UL_EXT);
            break;
        }
    }

    static constexpr const int NUM_BASIC_ANSI_COLORS = 8;

    void emit_lo8(const WhichEnum which, const AnsiColor16Enum c)
    {
        assert(which != WhichEnum::Under);
        assert(isLow(c));
        const auto offset = which == WhichEnum::Fore ? ANSI_FG_COLOR : ANSI_BG_COLOR;
        result.add_code(offset + getIndex16(toLow(c)) % NUM_BASIC_ANSI_COLORS);
    }

    void emit_hi8(const WhichEnum which, const AnsiColor16Enum c)
    {
        assert(which != WhichEnum::Under);
        assert(isHigh(c));
        const auto offset = which == WhichEnum::Fore ? ANSI_FG_COLOR_HI : ANSI_BG_COLOR_HI;
        result.add_code(offset + getIndex16(toLow(c)) % NUM_BASIC_ANSI_COLORS);
    }

    void emit16(const WhichEnum which, const AnsiColor16Enum c)
    {
        if (which == WhichEnum::Under) {
            // underline color isn't supported in 8 or 16-color mode, and if we've gotten here,
            // then that means we've already checked 256 and RGB support.
            return;
        }

        if (isLow(c)) {
            emit_lo8(which, c);
            return;
        }

        if (!supports16()) {
            emit_lo8(which, toLow(c));
            return;
        }

        emit_hi8(which, c);
    }

    void emit256(const WhichEnum which, const AnsiColor256 c)
    {
        if (c.color < ANSI256_RGB6_START) {
            emit16(which, toAnsiColor16Enum(c));
            return;
        }

        if (supports256()) {
            const bool useColon = supportsItu256();
            emitExtPrefix(which);
            result.add_code(ANSI_EXT_256, useColon);
            result.add_code(c.color, useColon);
            return;
        }

        if (supportsRGB()) {
            emitRGB_with_support(which, toAnsiColorRGB(c));
            return;
        }

        emit16(which, toAnsiColor16Enum(c));
    }

    void emitRGB_with_support(const WhichEnum which, const AnsiColorRGB c)
    {
        assert(supportsRGB());
        const bool useColon = supportsItuRGB();
        emitExtPrefix(which);
        result.add_code(ANSI_EXT_RGB, useColon);
        if (useColon) {
            result.add_empty_code(true);
        }
        result.add_code(c.r, useColon);
        result.add_code(c.g, useColon);
        result.add_code(c.b, useColon);
    }

    void emitRGB(const WhichEnum which, const AnsiColorRGB c)
    {
        if (supportsRGB()) {
            emitRGB_with_support(which, c);
            return;
        }

        if (supports256()) {
            return emit256(which, toAnsiColor256(c));
        }

        emit16(which, toAnsiColor16Enum(c));
    }
};

AnsiString ansi_transition(const AnsiSupportFlags flags, const RawAnsi &before, const RawAnsi &after)
{
    return AnsiEmitter{flags}.getAnsiString(before, after);
}

AnsiString ansi_string(const AnsiSupportFlags flags, const RawAnsi &ansi)
{
    return ansi_transition(flags, RawAnsi{}, ansi);
}

void ansi_transition(std::ostream &os,
                     const AnsiSupportFlags flags,
                     const RawAnsi &before,
                     const RawAnsi &after)
{
    const AnsiString &string = ansi_transition(flags, before, after);
    os << string.c_str();
}

void ansi_string(std::ostream &os, const AnsiSupportFlags flags, const RawAnsi &ansi)
{
    const AnsiString &string = ansi_string(flags, ansi);
    os << string.c_str();
}

static void clamp255Inplace(int &n)
{
    assert(isClamped(n, 0, 255));
    n = std::clamp(n, 0, 255);
}

static uint8_t clamp255(const uint32_t n)
{
    assert((n & 0xFFu) == n);
    return static_cast<uint8_t>(n);
}

static uint8_t clamp255(const int32_t n)
{
    return clamp255(static_cast<uint32_t>(n));
}

std::string_view to_hex_color_string_view(AnsiColor16Enum ansi)
{
    // TODO: support switching between different color standards.
    static const std::string_view black("#2E3436");
    static const std::string_view BLACK("#555753");
    static const std::string_view red("#CC0000");
    static const std::string_view RED("#EF2929");
    static const std::string_view green("#4E9A06");
    static const std::string_view GREEN("#8AE234");
    static const std::string_view yellow("#C4A000");
    static const std::string_view YELLOW("#FCE94F");
    static const std::string_view blue("#3465A4");
    static const std::string_view BLUE("#729FCF");
    static const std::string_view magenta("#75507B");
    static const std::string_view MAGENTA("#AD7FA8");
    static const std::string_view cyan("#06989A");
    static const std::string_view CYAN("#34E2E2");
    static const std::string_view white("#D3D7CF");
    static const std::string_view WHITE("#EEEEEC");

#define X_CASE(n, lower, UPPER) \
    case AnsiColor16Enum::lower: \
        return lower; \
    case AnsiColor16Enum::UPPER: \
        return UPPER;

    switch (ansi) {
        XFOREACH_ANSI_COLOR_0_7(X_CASE)
    default:
        break;
    }

    assert(false);
    return "error:UNDEFINED";
#undef X_CASE
}

Color toColor(const AnsiColor16Enum ansi)
{
    struct NODISCARD AllAnsiColors final
    {
#define X_DEFINE_VALUES(n, lower, UPPER) \
    const Color lower = lookup(AnsiColor16Enum::lower); \
    const Color UPPER = lookup(AnsiColor16Enum::UPPER);
        XFOREACH_ANSI_COLOR_0_7(X_DEFINE_VALUES)
#undef X_DEFINE_VALUES

        NODISCARD static Color lookup(AnsiColor16Enum color)
        {
            const auto sv = to_hex_color_string_view(color);
            assert(!sv.empty() && sv.front() == C_POUND_SIGN);
            return Color::fromHex(sv.substr(1));
        }
    };

    // NOTE: This would have to be reset if the color standard changes.
    static const AllAnsiColors precomputed;

#define X_CASE(n, lower, UPPER) \
    case AnsiColor16Enum::lower: \
        return precomputed.lower; \
    case AnsiColor16Enum::UPPER: \
        return precomputed.UPPER;

    switch (ansi) {
        XFOREACH_ANSI_COLOR_0_7(X_CASE)
    default:
        break;
    }

    assert(false);
    return precomputed.white;
#undef X_CASE
}

QColor mmqt::toQColor(const AnsiColor16Enum ansi)
{
    const Color color = ::toColor(ansi);
    return static_cast<QColor>(color);
}

std::string_view to_string_view(const AnsiColor16Enum color)
{
#define X_CASE(n, lower, UPPER) \
    case AnsiColor16Enum::lower: \
        return #lower; \
    case AnsiColor16Enum::UPPER: \
        return #UPPER;

    switch (color) {
        XFOREACH_ANSI_COLOR_0_7(X_CASE)
    default:
        break;
    }
    assert(false);
    return "*error*";
#undef X_CASE
}

AnsiColorRGB toAnsiColorRGB(const Color &color)
{
    const uint8_t red = clamp255(color.getRed());
    const uint8_t green = clamp255(color.getGreen());
    const uint8_t blue = clamp255(color.getBlue());
    return AnsiColorRGB{red, green, blue};
}

AnsiColorRGB toAnsiColorRGB(const AnsiColor16Enum e)
{
    const auto c = toColor(e);
    return toAnsiColorRGB(c);
}

AnsiColorRGB toAnsiColorRGB(const AnsiColor256 ansiColor)
{
    const uint8_t ansi = ansiColor.color;

    // 232-255: grayscale from black to white in 24 steps
    if (ansi >= ANSI256_GREY_START) {
        if ((false)) {
            const auto c = static_cast<uint8_t>(
                wiki::ansi_greys24[static_cast<size_t>(ansi - ANSI256_GREY_START)]);
            return AnsiColorRGB{c, c, c};
        } else {
            const auto c = static_cast<uint8_t>((ansi - ANSI256_GREY_START) * 10 + 8);
            return AnsiColorRGB{c, c, c};
        }
    }

    // 16-231: 6 x 6 x 6 cube (216 colors): 16 + 36 * r + 6 * g + b
    if (ansi >= ANSI256_RGB6_START) {
        auto lut = [](int x) {
            assert(isClamped(x, 0, 6));
            return wiki::ansi_colors6[static_cast<size_t>(x)];
        };

        const int colors = ansi - ANSI256_RGB6_START;
        assert(isClamped(colors, 0, 6 * 6 * 6 - 1));
        const auto r = lut((colors / 36) % 6);
        const auto g = lut((colors / 6) % 6);
        const auto b = lut(colors % 6);
        return AnsiColorRGB{r, g, b};
    }

    static const auto lookup = [](const uint8_t x) -> AnsiColor16Enum {
#define X_CASE(n, lower, UPPER) \
    case (n): \
        return AnsiColor16Enum::lower; \
    case (n) + ANSI256_HI_OFFSET: \
        return AnsiColor16Enum::UPPER;
        switch (x) {
            XFOREACH_ANSI_COLOR_0_7(X_CASE)
        default:
            break;
        }
        assert(false);
        return AnsiColor16Enum::red;

#undef X_CASE
    };

#if !(defined(__GNUC__) && __GNUC__ <= 8)
    static_assert(lookup(0) == AnsiColor16Enum::black);
    static_assert(lookup(7) == AnsiColor16Enum::white);
    static_assert(lookup(8) == AnsiColor16Enum::BLACK);
    static_assert(lookup(15) == AnsiColor16Enum::WHITE);
#endif

    const auto e = lookup(ansi);
    return toAnsiColorRGB(e);
}

QColor mmqt::ansi256toRgb(const int ansi)
{
    if (ansi < 0 || ansi > 255) {
        assert(false);
        return mmqt::toQColor(AnsiColor16Enum::RED);
    }
    const AnsiColor256 c256{static_cast<uint8_t>(ansi & 0xFF)};
    const AnsiColorRGB c = ::toAnsiColorRGB(c256);
    return QColor{c.r, c.g, c.b};
}

int rgbToAnsi256(int r, int g, int b)
{
    // REVISIT: check for exact match to color table?

    clamp255Inplace(r);
    clamp255Inplace(g);
    clamp255Inplace(b);

    // we initially used
    // https://stackoverflow.com/questions/15682537/ansi-color-specific-rgb-sequence-bash
    // which uses Math.round(((r - 8) / 247) * 24) + 232,
    // but that doesn't match the values found in
    // https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_(Select_Graphic_Rendition)_parameters

    if (r == g && g == b) {
        static auto conv = [](const int n) -> int {
            if (n < 3) {
                return ANSI256_RGB6_START; // rgb6(0x0x0) = black
            } else if (n > 243) {
                return ANSI256_RGB6_END; // rgb6(5x5x5) = white
            }
            return ANSI256_GREY_START + std::clamp((n - 3) / 10, 0, 23);
        };

        auto test_conv = []() -> bool {
            const auto size = wiki::ansi_greys24.size();
            if (size != 24) {
                return false;
            }
            for (size_t i = 0; i < size; ++i) {
                if (wiki::ansi_greys24[i] != static_cast<int>(8 + 10 * i)) {
                    return false;
                }
                if (conv(wiki::ansi_greys24[i]) != ANSI256_GREY_START + static_cast<int>(i)) {
                    return false;
                }
            }
            return true;
        };
        static_assert(test_conv());

        //
        // 8 -> 0
        // 12 rounds down
        // 13 rounds up...
        // 18 -> 1
        // 23 rounds up

        // 0..2 = 3 black values
        static_assert(conv(0) == 16);
        static_assert(conv(2) == 16);
        //
        static_assert(conv(3) == 232);
        static_assert(conv(8) == 232);
        static_assert(conv(12) == 232);
        //
        static_assert(conv(13) == 233);
        static_assert(conv(18) == 233);
        static_assert(conv(22) == 233);
        //
        static_assert(conv(23) == 234);
        //
        // ...
        //
        static_assert(conv(223) == 254);
        static_assert(0xe4 == 228);
        static_assert(conv(228) == 254);
        static_assert(conv(232) == 254);
        //
        static_assert(conv(233) == 255);
        static_assert(0xEE == 238);
        static_assert(conv(238) == 255);
        static_assert(conv(243) == 255);

        // 244-255 = 12 white values
        static_assert(conv(244) == 231);
        static_assert(conv(255) == 231);
        return conv(r);
    }

    static auto shrink05 = [](int x) -> int {
        // NOTE: cutoffs between color values a and b were computed based on average position
        // in linear color space using the approximation
        //   `255 * sqrt( ( (a/255)^2 + (b/255)^2 ) / 2 )`
        // based on the notion that (sRGB/255)^2 is approximately linear.
        //
        // Feel free to pick better cutoffs if you feel like using the correct sRGB to linear,
        // or pick whatever you feel like, as long as the transform is roundtrip-stable.
        assert(isClamped(x, 0, 255));
        if (x < 157) {
            if (x < 62) {
                return 0;
            }
            if (x < 114) {
                return 1;
            }
            return 2;
        } else {
            if (x < 197) { // or 196?
                return 3;
            }
            if (x < 236) {
                return 4;
            }
            return 5;
        }
    };

    auto test_shrink05 = []() -> bool {
        const auto size = wiki::ansi_colors6.size();
        if (size != 6) {
            return false;
        }
        for (size_t i = 0; i < size; ++i) {
            if (shrink05(wiki::ansi_colors6[i]) != static_cast<int>(i)) {
                return false;
            }
        }
        return true;
    };
    static_assert(test_shrink05());

    const int red = shrink05(r);
    const int green = shrink05(g);
    const int blue = shrink05(b);

    return ANSI256_RGB6_START + 36 * red + 6 * green + blue;
}

AnsiColor256 toAnsiColor256(const AnsiColorRGB c)
{
    const auto n = rgbToAnsi256(c.r, c.g, c.b);
    return AnsiColor256{clamp255(n)};
}

// REVISIT: do we weant a version that writes to an ostream?
NODISCARD static std::string rgbToAnsi256String(const Color c, const AnsiColor16LocationEnum type)
{
    const AnsiColorRGB ansiRgb = toAnsiColorRGB(c);

    RawAnsi raw;
    switch (type) {
    case AnsiColor16LocationEnum::Foreground:
        raw.fg = ansiRgb;
        break;
    case AnsiColor16LocationEnum::Background:
        const auto hasWhiteText = (textColor(c) == Colors::white);
        raw.fg = AnsiColorVariant{hasWhiteText ? AnsiColor16Enum::white : AnsiColor16Enum::black};
        raw.bg = ansiRgb;
        break;
    }

    // note: Can change the support flags to allow ITU color,
    // and can even output as 24-bit ANSI.
    AnsiString str = ansi_string(ANSI_COLOR_SUPPORT_256, raw);
    const auto &s = str.getStdString();
    assert(!s.empty());
    assert(s.front() == C_ESC);
    return s.substr(1);
}

QString mmqt::rgbToAnsi256String(const QColor &qrgb, const AnsiColor16LocationEnum type)
{
    const auto rgb = static_cast<Color>(qrgb);
    assert(static_cast<int>(rgb.getRed()) == qrgb.red());
    assert(static_cast<int>(rgb.getGreen()) == qrgb.green());
    assert(static_cast<int>(rgb.getBlue()) == qrgb.blue());
    return toQStringUtf8(::rgbToAnsi256String(rgb, type));
}

NODISCARD static AnsiColor16Enum getClosestMatchInColorSpace(const AnsiColorRGB rgb)
{
    struct NODISCARD Table final
    {
    private:
        using Arr = EnumIndexedArray<AnsiColorRGB, AnsiColor16Enum, 16>;
        Arr arr;

    public:
        Table() { init(); }

    private:
        void init()
        {
            for (size_t i = 0; i < Arr::SIZE; ++i) {
                auto ansi = static_cast<AnsiColor16Enum>(i);
                arr[ansi] = toAnsiColorRGB(ansi);
            }
        }

    private:
        struct NODISCARD HSL final
        {
            int h = -1;
            int s = 0;
            int l = 0;

            explicit HSL(const int _h, const int _s, const int _l)
                : h{_h}
                , s{_s}
                , l{_l}
            {
                assert(isClamped(h, -1, 359) && isClamped(s, 0, 255) && isClamped(l, 0, 255));
            }

            NODISCARD bool hasHue() const { return h >= 0; }
        };

        NODISCARD static float distSquaredHSL(const HSL &a, const HSL &b)
        {
            static auto toFloat255 = [](int x) {
                assert(isClamped(x, 0, 255));
                return static_cast<float>(x) / 255;
            };
            static auto hueSat = [](const HSL &hsl) -> glm::vec2 {
                if (!hsl.hasHue()) {
                    return glm::vec2{0.f, 0.f};
                }
                assert(isClamped(hsl.h, 0, 359));
                const auto deg = static_cast<float>(hsl.h);
                const auto rad = glm::radians(deg);
                const auto result = toFloat255(hsl.s) * glm::vec2{glm::cos(rad), glm::sin(rad)};
                return result;
            };

            // note that hue wraps, so we'll use circular arithmetic,
            // where 180 degrees is the greatest difference.
            const auto hueDiffSat2 = glm::distance(hueSat(a), hueSat(b));
            assert(isClamped(hueDiffSat2, 0.f, 2.f));

            return 0.5f * hueDiffSat2                                 //
                   + glm::distance(toFloat255(a.l), toFloat255(b.l)); //
        }

        NODISCARD static float distSquaredHSL(const AnsiColorRGB a, const AnsiColorRGB b)
        {
            auto toHsl = [](const AnsiColorRGB x) -> HSL {
                int h = 0;
                int s = 0;
                int l = 0;
                const Color c = Color(x.r, x.g, x.b);
                const QColor qc = static_cast<QColor>(c);
                qc.getHsl(&h, &s, &l);
                return HSL{h, s, l};
            };
            const HSL &hsla = toHsl(a);
            const HSL &hslb = toHsl(b);
            auto dist2 = distSquaredHSL(hsla, hslb);
            return dist2;
        }

        NODISCARD static float distSquaredRGBlinear(const AnsiColorRGB a, const AnsiColorRGB b)
        {
            auto rgbvec = [](AnsiColorRGB x) -> glm::vec3 {
                const auto srgb = glm::vec3(x.r, x.g, x.b) / 255.f;
                const auto linear = srgb * srgb; // approximation of sRGB to linear
                return linear;
            };

            const auto av = rgbvec(a);
            const auto bv = rgbvec(b);
            const auto dist = glm::abs(av - bv);
            const float dist2 = glm::dot(dist, dist);
            return dist2;
        }

        NODISCARD static float distSquared(const AnsiColorRGB a, const AnsiColorRGB b)
        {
            // Explanation for why both have MAGENTA on the black to white axis:
            //
            // That just means MAGENTA is the least saturated of our ANSI colors,
            // and that BLACK and white are farther from the center than MAGENTA.
            //
            // If you view the color prism along the black-white axis, the colored corners
            // are saturated and relatively far from the center, but the chosen colors
            // don't lie exactly on those corners (they're closer), and the BRIGHT versions
            // are less saturated because they're closer to being grey / white.
            //
            // If you view the color prism from the side (from black to white),
            // then all the colored corners of the prism will project to a point
            // near the midpoint of the black to white axis.
            //
            // Taken together, this says that the least saturated color will be selected
            // at some point as we walk the greyscale values, unless BLACK or white
            // is somehow closer than all of the other colors.

            if ((false)) {
                // This method seems "worse" because it chooses colors that are
                // imperceptibly different than black (e.g. {1,0,0}) as the full color.

                //    On the black-red axis ...
                //    ... AnsiColorRGB{0, 0, 0} becomes black
                //    ... AnsiColorRGB{1, 0, 0} becomes red
                //    On the black-green axis ...
                //    ... AnsiColorRGB{0, 0, 0} becomes black
                //    ... AnsiColorRGB{0, 1, 0} becomes green
                //    ... AnsiColorRGB{0, 222, 0} becomes GREEN
                //    On the black-yellow axis ...
                //    ... AnsiColorRGB{0, 0, 0} becomes black
                //    ... AnsiColorRGB{1, 1, 0} becomes yellow
                //    ... AnsiColorRGB{254, 254, 0} becomes YELLOW
                //    On the black-blue axis ...
                //    ... AnsiColorRGB{0, 0, 0} becomes black
                //    ... AnsiColorRGB{0, 0, 114} becomes blue
                //    On the black-magenta axis ...
                //    ... AnsiColorRGB{0, 0, 0} becomes black
                //    ... AnsiColorRGB{122, 0, 122} becomes magenta
                //    ... AnsiColorRGB{250, 0, 250} becomes MAGENTA
                //    On the black-cyan axis ...
                //    ... AnsiColorRGB{0, 0, 0} becomes black
                //    ... AnsiColorRGB{0, 1, 1} becomes cyan
                //    ... AnsiColorRGB{0, 242, 242} becomes CYAN
                //    On the black-white axis ...
                //    ... AnsiColorRGB{0, 0, 0} becomes black
                //    ... AnsiColorRGB{64, 64, 64} becomes BLACK
                //    ... AnsiColorRGB{130, 130, 130} becomes MAGENTA
                //    ... AnsiColorRGB{173, 173, 173} becomes white
                //    ... AnsiColorRGB{222, 222, 222} becomes WHITE
                return distSquaredHSL(a, b);
            } else {
                // NOTE: our green (78, 154, 6) shows up on the yellow axis
                // because it has a very large red component,
                //
                // and our BLUE(114, 159, 207) shows up on the cyan axis
                // because it has a very large green component.

                //    On the black-red axis ...
                //    ... AnsiColorRGB{0, 0, 0} becomes black
                //    ... AnsiColorRGB{116, 0, 0} becomes BLACK
                //    ... AnsiColorRGB{152, 0, 0} becomes red
                //    ... AnsiColorRGB{223, 0, 0} becomes RED
                //    On the black-green axis ...
                //    ... AnsiColorRGB{0, 0, 0} becomes black
                //    ... AnsiColorRGB{0, 118, 0} becomes green
                //    ... AnsiColorRGB{0, 209, 0} becomes GREEN
                //    On the black-yellow axis ...
                //    ... AnsiColorRGB{0, 0, 0} becomes black
                //    ... AnsiColorRGB{83, 83, 0} becomes BLACK
                //    ... AnsiColorRGB{122, 122, 0} becomes green
                //    ... AnsiColorRGB{150, 150, 0} becomes yellow
                //    ... AnsiColorRGB{214, 214, 0} becomes YELLOW
                //    On the black-blue axis ...
                //    ... AnsiColorRGB{0, 0, 0} becomes black
                //    ... AnsiColorRGB{0, 0, 131} becomes blue
                //    ... AnsiColorRGB{0, 0, 239} becomes BLUE
                //    On the black-magenta axis ...
                //    ... AnsiColorRGB{0, 0, 0} becomes black
                //    ... AnsiColorRGB{87, 0, 87} becomes BLACK
                //    ... AnsiColorRGB{102, 0, 102} becomes magenta
                //    ... AnsiColorRGB{160, 0, 160} becomes MAGENTA
                //    On the black-cyan axis ...
                //    ... AnsiColorRGB{0, 0, 0} becomes black
                //    ... AnsiColorRGB{0, 88, 88} becomes BLACK
                //    ... AnsiColorRGB{0, 121, 121} becomes cyan
                //    ... AnsiColorRGB{0, 191, 191} becomes BLUE
                //    ... AnsiColorRGB{0, 195, 195} becomes CYAN
                //    On the black-white axis ...
                //    ... AnsiColorRGB{0, 0, 0} becomes black
                //    ... AnsiColorRGB{70, 70, 70} becomes BLACK
                //    ... AnsiColorRGB{106, 106, 106} becomes magenta
                //    ... AnsiColorRGB{139, 139, 139} becomes MAGENTA
                //    ... AnsiColorRGB{184, 184, 184} becomes white
                //    ... AnsiColorRGB{225, 225, 225} becomes WHITE
                return distSquaredRGBlinear(a, b);
            }
        }

    public:
        NODISCARD AnsiColor16Enum getClosest(const AnsiColorRGB rgb) const
        {
            constexpr bool earlyTest = true;
            if (earlyTest) {
                if (auto optIdx = arr.findIndexOf(rgb)) {
                    return *optIdx;
                }
            }

            auto it = std::min_element(std::begin(arr),
                                       std::end(arr),
                                       [rgb](const AnsiColorRGB a, const AnsiColorRGB b) -> float {
                                           bool less = distSquared(rgb, a) < distSquared(rgb, b);
                                           return less;
                                       });

            const auto result = static_cast<AnsiColor16Enum>(it - std::begin(arr));

            if (!earlyTest) {
                if (auto optIdx = arr.findIndexOf(rgb)) {
                    assert(result == *optIdx);
                }
            }
            return result;
        }
    };

    static const Table table;
    return table.getClosest(rgb);
}

AnsiColor16Enum toAnsiColor16Enum(const AnsiColor256 ansi256)
{
    if (ansi256.color < ANSI256_RGB6_START) {
#define X_CASE(N, lower, UPPER) \
    case (N): \
        return AnsiColor16Enum::lower; \
    case (N) + 8: \
        return AnsiColor16Enum::UPPER;
        switch (ansi256.color) {
            XFOREACH_ANSI_COLOR_0_7(X_CASE)
            //
        default:
            break;
        }
#undef X_CASE
        assert(false);
        std::abort();
    }

    const auto rgb = toAnsiColorRGB(ansi256);
    return getClosestMatchInColorSpace(rgb);
}

AnsiColor16Enum toAnsiColor16Enum(const AnsiColorRGB rgb)
{
    return getClosestMatchInColorSpace(rgb);
}

AnsiColor16 toAnsiColor16(AnsiColorVariant var)
{
    if (var.hasDefaultColor()) {
        return AnsiColor16{};
    } else if (var.has256()) {
        return AnsiColor16{toAnsiColor16Enum(var.get256())};
    } else if (var.hasRGB()) {
        return AnsiColor16{toAnsiColor16Enum(var.getRGB())};
    } else {
        std::abort();
    }
}

std::ostream &to_stream(std::ostream &os, const AnsiColor256 c)
{
    if (c.color < ANSI256_RGB6_START) {
        return os << AnsiColor16{toAnsiColor16Enum(c)};
    } else if (c.color < ANSI256_GREY_START) {
        const int x = c.color - ANSI256_RGB6_START;
        const int r = (x / 36) % 6;
        const int g = (x / 6) % 6;
        const int b = x % 6;
        return os << "AnsiColor256{" << static_cast<uint32_t>(c.color) << "} (aka rgb6{" << r << "x"
                  << g << "x" << b << "})";
    } else {
        int grey = c.color - ANSI256_GREY_START;
        return os << "AnsiColor256{" << static_cast<uint32_t>(c.color) << "} (aka grey24{" << grey
                  << "})";
    }
}

std::ostream &to_stream(std::ostream &os, const RawAnsi &raw)
{
    bool first = true;
    auto maybe_space = [&first, &os]() {
        if (first) {
            first = false;
        } else {
            os << ", ";
        }
    };

    for (const AnsiStyleFlagEnum flag : raw.getFlags()) {
        maybe_space();
        os << to_string_view(flag, false);
    }

    if (raw.hasForegroundColor()) {
        maybe_space();
        os << raw.fg;
    }
    if (raw.hasBackgroundColor()) {
        maybe_space();
        os << "on ";
        os << raw.bg;
    }
    const auto underlineStyle = raw.getUnderlineStyle();
    if (raw.hasUnderlineColor()) {
        maybe_space();
        os << "with ";
        os << raw.ul;
        if (underlineStyle != AnsiUnderlineStyle::None
            && underlineStyle != AnsiUnderlineStyle::Normal) {
            os << C_SPACE;
            os << toStringViewLowercase(underlineStyle);
        }
        os << " underline";
    } else if (underlineStyle != AnsiUnderlineStyle::None
               && underlineStyle != AnsiUnderlineStyle::Normal) {
        maybe_space();
        os << "with ";
        os << toStringViewLowercase(underlineStyle);
        os << " underline";
    }
    return os;
}

NODISCARD static bool isAnsiColorInner(char c)
{
    return isAscii(c) && (ascii::isDigit(c) || c == C_SEMICOLON || c == C_COLON);
}

size_t ansiCodeLen(const std::string_view input)
{
    assert(!input.empty());

    if (input.front() != char_consts::C_ESC) {
        return 0;
    }

    const auto len = input.length();
    for (size_t it = 1; it < len; ++it) {
        const char c = input[it];
        if (!isAscii(c) || ascii::isCntrl(c)) {
            return it;
        } else if (ascii::isLower(c) || ascii::isUpper(c)) {
            return it + 1;
        }
    }
    return len;
}

bool isAnsiColor(std::string_view ansi)
{
    if (ansi.length() < 3 || ansi.front() != C_ANSI_ESCAPE || ansi.at(1) != C_OPEN_BRACKET
        || ansi.back() != 'm') {
        return false;
    }

    ansi = ansi.substr(2, ansi.length() - 3);
    return std::all_of(ansi.begin(), ansi.end(), isAnsiColorInner);
}

std::optional<RawAnsi> ansi_parse(RawAnsi input_ansi, const std::string_view input)
{
    if (!isAnsiColor(input)) {
        return std::nullopt;
    }
    auto ansi = input;
    ansi.remove_prefix(2);
    ansi.remove_suffix(1);

    AnsiColorState state{input_ansi};
    foreachUtf8CharSingle(
        ansi,
        C_SEMICOLON,
        []() {
            // semicolon
        },
        [&state](std::string_view digitsOrColons) {
            AnsiItuColorCodes codes;
            foreachUtf8CharSingle(
                digitsOrColons,
                C_COLON,
                []() {
                    // colon
                },
                [&codes](std::string_view digits) {
                    if (digits.empty()) {
                        codes.push_back(0);
                        return;
                    }
                    const auto *const beg = digits.data();
                    const auto *const end = beg + static_cast<ptrdiff_t>(digits.size());
                    int result = 0;
                    auto [ptr, ec] = std::from_chars(beg, end, result);
                    if (ec != std::errc{} || ptr != end || result < 0 || result > 255) {
                        codes.push_back(-1);
                    } else {
                        codes.push_back(result);
                    }
                });
            if (codes.empty() || codes.overflowed()) {
                state.receive(-1);
            } else if (codes.size() == 1) {
                state.receive(codes.front());
            } else {
                state.receive_itu(codes);
            }
        });

    if (!state.hasCompleteState()) {
        return std::nullopt;
    }
    return state.getRawAnsi();
}

namespace mmqt {

bool containsAnsi(const QStringView str)
{
    return str.contains(C_ANSI_ESCAPE);
}

bool containsAnsi(const QString &str)
{
    return str.contains(C_ANSI_ESCAPE);
}

bool isAnsiColor(QStringView ansi)
{
    if (ansi.length() < 3 || ansi.front() != C_ANSI_ESCAPE || ansi.at(1) != C_OPEN_BRACKET
        || ansi.back() != 'm') {
        return false;
    }

    ansi = ansi.mid(2, ansi.length() - 3);
    for (const QChar c : ansi) {
        // REVISIT: This may be incorrect if it allows Unicode digits
        // that are not part of the LATIN1 subset.
        if (c != C_SEMICOLON && c != C_COLON && !c.isDigit()) {
            return false;
        }
    }

    return true;
}

bool isAnsiColor(const QString &ansi)
{
    return isAnsiColor(QStringView{ansi});
}

void AnsiColorParser::for_each(QStringView ansi) const
{
    using namespace mmqt;

    if (!isAnsiColor(ansi)) {
        // It's okay for this to be something that's not an ansi color.
        // For example, if someone types "\033[42am" in the editor
        // and then normalizes it, you'll get "\033[42a" as the escape code.
        return;
    }

    // NOTE: ESC[m is a special case alias for ESC[0m according to wikipedia,
    // but xterm also supports empty values like ESC[;42;m,
    // which gets interpreted as if you had used ESC[0;42;0m,
    // so mmapper will do the same.

    // ESC[...m -> ...
    ansi = ansi.mid(2, ansi.length() - 3);

    const int len = ansi.length();
    qsizetype pos = 0;

    const auto try_report_itu = [this, &ansi, &pos](const qsizetype end) {
        const auto substr = ansi.left(end);
        if (verbose_debugging) {
            qInfo() << "try_report_itu at" << end << substr;
        }
        AnsiItuColorCodes codes;
        while (pos < end) {
            const qsizetype idx = substr.indexOf(C_COLON, pos);
            if (idx < 0) {
                break;
            }
            codes.push_back(parsePositiveInt(substr.mid(pos, idx - pos)));
            pos = idx + 1;
        }

        if (pos != end) {
            codes.push_back(parsePositiveInt(substr.mid(pos, end - pos)));
        }

        if (codes.empty()) {
            return;
        }

        if (verbose_debugging) {
            auto &&info = qInfo();
            info << "reporting itu:";
            for (int i : codes) {
                info << i;
            }
        }
        report_itu(codes);
    };

    const auto try_report = [this, &ansi, &pos, &try_report_itu](const qsizetype end) -> void {
        if (verbose_debugging) {
            qInfo() << "try_report at" << end << ansi.mid(pos, end - pos);
        }

        if (end <= pos) {
            if (verbose_debugging) {
                qInfo() << "reporting (0)" << end;
            }
            report(0);
            return;
        }

        if (!ansi.mid(pos, end - pos).contains(C_COLON)) {
            const int n = parsePositiveInt(ansi.mid(pos, end - pos));
            if (verbose_debugging) {
                qInfo() << "reporting" << n;
            }
            report(n);
            return;
        }

        try_report_itu(end);
    };

    // FIXME: [ITU] This is wrong, but it's better than nothing.
    // the correct solution is to split by semicolons as a single unit,
    // and then split by colon, so "1:2;3:4:5;6" would be processed
    // as if it were [1, 2], [3, 4, 5], [6].
    //
    // But the catch is that you have to fall back to semicolon-separated
    // if the ANSI author used the older semicolon-only standard, which
    // effectively means the new colon standard doesn't exist unless you
    // can strictly require it.
    while (pos < len) {
        const qsizetype idx = ansi.indexOf(C_SEMICOLON, pos);
        if (idx < 0) {
            break;
        }

        try_report(idx);
        pos = idx + 1;
    }

    try_report(len);
}

std::optional<RawAnsi> parseAnsiColor(const RawAnsi before, const QStringView ansi)
{
    if (!isAnsiColor(ansi)) {
        return std::nullopt;
    }

    AnsiColorState colorState{before};
    bool valid = true;
    ansiForeachColorCode(
        ansi,
        [&colorState, &valid](int n) -> void {
            if (n < 0) {
                valid = false;
            } else {
                colorState.receive(n);
            }
        },
        [&colorState](const AnsiItuColorCodes &codes) -> void { colorState.receive_itu(codes); });

    if (!valid || !colorState.hasCompleteState()) {
        return std::nullopt;
    }

    return colorState.getRawAnsi();
}

std::optional<RawAnsi> parseAnsiColor(const RawAnsi before, const QString &ansi)
{
    return parseAnsiColor(before, QStringView{ansi});
}

bool isValidAnsiColor(const QStringView ansi)
{
    return parseAnsiColor(RawAnsi(), ansi).has_value();
}

bool isValidAnsiColor(const QString &ansi)
{
    return isValidAnsiColor(QStringView{ansi});
}

TextBuffer normalizeAnsi(const AnsiSupportFlags supportFlags, const QStringView old)
{
    if (!containsAnsi(old)) {
        assert(false);
        TextBuffer output;
        output.append(old);
        return output;
    }

    TextBuffer output;
    output.reserve(2 * old.length()); /* no idea */

    const auto reset = AnsiString::get_reset_string();

    RawAnsi ansi;

    foreachLine(old,
                [&ansi, &reset, &output, supportFlags](const QStringView line,
                                                       const bool hasNewline) {
                    if (ansi != RawAnsi{}) {
                        const AnsiString &string = ansi_transition(supportFlags, RawAnsi{}, ansi);
                        const AnsiString &asReset = string.copy_as_reset();
                        output.append(asReset.c_str());
                    }

                    RawAnsi current = ansi;
                    RawAnsi next = ansi;

                    const auto transition = [&current, &next, &output, supportFlags]() {
                        if (current == next) {
                            return;
                        }
                        AnsiString delta = ansi_transition(supportFlags, current, next);
                        output.append(delta.c_str());
                        current = next;
                    };

                    const auto print = [&transition, &output](const QStringView ref) {
                        transition();
                        output.append(ref);
                    };

                    int pos = 0;
                    foreachAnsi(line,
                                [&next, &line, &pos, &print](const int begin,
                                                             const QStringView ansiStr) {
                                    assert(line.at(begin) == C_ESC);
                                    if (begin > pos) {
                                        print(line.mid(pos, begin - pos));
                                    }

                                    pos = begin + ansiStr.length();

                                    AnsiColorState colorState{next};
                                    ansiForeachColorCode(
                                        ansiStr,
                                        [&colorState](int code) { colorState.receive(code); },
                                        [&colorState](const AnsiItuColorCodes &codes) {
                                            colorState.receive_itu(codes);
                                        });
                                    if (colorState.hasCompleteState()) {
                                        next = colorState.getRawAnsi();
                                    }
                                });

                    if (pos < line.length()) {
                        print(line.mid(pos));
                    }

                    if (current != next || next != RawAnsi()) {
                        output.append(reset.c_str());
                    }

                    if (hasNewline) {
                        output.append(C_NEWLINE);
                    }
                    ansi = next;
                });

    return output;
}

TextBuffer normalizeAnsi(AnsiSupportFlags supportFlags, const QString &str)
{
    return normalizeAnsi(supportFlags, QStringView{str});
}

bool AnsiStringToken::isAnsiCsi() const
{
    return type == TokenTypeEnum::Ansi && length() >= 4 && at(0) == QC_ESC
           && at(1) == QC_OPEN_BRACKET && at(length() - 1) == 'm';
}

AnsiTokenizer::Iterator::Iterator(const QStringView str)
    : m_str{str}
{}

AnsiStringToken AnsiTokenizer::Iterator::next()
{
    assert(hasNext());
    MAYBE_UNUSED const auto len = m_str.size();
    const AnsiStringToken token = getCurrent();
    m_str = m_str.mid(token.length());
    return token;
}

AnsiStringToken AnsiTokenizer::Iterator::getCurrent()
{
    assert(!m_str.empty());
    const QChar c = m_str[0];
    if (c == QC_ESC) {
        return AnsiStringToken{TokenTypeEnum::Ansi, m_str, 0, skip_ansi()};
    } else if (c == QC_NEWLINE) {
        return AnsiStringToken{TokenTypeEnum::Newline, m_str, 0, 1};
    }
    // Special case to match "\r\n" as just "\n"
    else if (c == QC_CARRIAGE_RETURN && 1 < m_str.size() && m_str[1] == QC_NEWLINE) {
        return AnsiStringToken{TokenTypeEnum::Newline, m_str, 1, 1};
    } else if (c == QC_CARRIAGE_RETURN || isControl(c)) {
        return AnsiStringToken{TokenTypeEnum::Control, m_str, 0, skip_control()};
    } else if (c.isSpace() && c != QC_NBSP) {
        // TODO: Find out of this includes control codes like form-feed ('\f') and vertical-tab ('\v').
        return AnsiStringToken{TokenTypeEnum::Space, m_str, 0, skip_space()};
    } else {
        return AnsiStringToken{TokenTypeEnum::Word, m_str, 0, skip_word()};
    }
}

AnsiTokenizer::Iterator::size_type AnsiTokenizer::Iterator::skip_ansi() const
{
    // hack to avoid having to have two separate stop return values
    // (one to stop before current value, and one to include it)
    bool sawLetter = false;
    return skip([&sawLetter](const QChar c) -> ResultEnum {
        if (sawLetter || c == QC_ESC || c == QC_NBSP || c.isSpace()) {
            return ResultEnum::STOP;
        }

        if (c == QC_OPEN_BRACKET || c == QC_SEMICOLON || c == QC_COLON || c.isDigit()) {
            return ResultEnum::KEEPGOING;
        }

        if (c.isLetter()) {
            /* include the letter, but stop afterwards */
            sawLetter = true;
            return ResultEnum::KEEPGOING;
        }

        /* ill-formed ansi code */
        return ResultEnum::STOP;
    });
}

AnsiTokenizer::Iterator::size_type AnsiTokenizer::Iterator::skip_control() const
{
    return skip([](const QChar c) -> ResultEnum {
        return isControl(c) ? ResultEnum::KEEPGOING : ResultEnum::STOP;
    });
}

AnsiTokenizer::Iterator::size_type AnsiTokenizer::Iterator::skip_space() const
{
    return skip([](const QChar c) -> ResultEnum {
        switch (c.toLatin1()) {
        case C_ESC:
        case C_NBSP:
        case C_CARRIAGE_RETURN:
        case C_NEWLINE:
            return ResultEnum::STOP;
        default:
            return c.isSpace() ? ResultEnum::KEEPGOING : ResultEnum::STOP;
        }
    });
}

AnsiTokenizer::Iterator::size_type AnsiTokenizer::Iterator::skip_word() const
{
    return skip([](const QChar c) -> ResultEnum {
        switch (c.toLatin1()) {
        case C_ESC:
        case C_NBSP:
        case C_CARRIAGE_RETURN:
        case C_NEWLINE:
            return ResultEnum::STOP;
        default:
            return (c.isSpace() || isControl(c)) ? ResultEnum::STOP : ResultEnum::KEEPGOING;
        }
    });
}

} // namespace mmqt

bool containsAnsi(const std::string_view sv)
{
    return sv.find(C_ESC) != std::string_view::npos;
}

// NOTE: This function requires the input to be "complete".
void strip_ansi(std::ostream &os, const std::string_view input)
{
    if (!containsAnsi(input)) {
        os << input;
        return;
    }

    bool hasEsc = false;
    foreachUtf8CharSingle(
        input,
        C_ESC,
        [&hasEsc]() { hasEsc = true; },
        [&os, &hasEsc](std::string_view sv) {
            if (sv.empty()) {
                return;
            }
            if (!hasEsc) {
                os << sv;
                return;
            }
            if (sv.front() == C_OPEN_BRACKET) {
                sv.remove_prefix(1);
            }
            while (!sv.empty()) {
                const char c = sv.front();
                if (c != C_SEMICOLON && c != C_COLON && !isClamped(c, '0', '9')) {
                    break;
                }

                sv.remove_prefix(1);
            }

            if (!sv.empty()) {
                const char c = sv.front();
                if (isClamped(char(std::tolower(c)), 'a', 'z')) {
                    sv.remove_prefix(1);
                }
            }

            hasEsc = false;
            os << sv;
        });
}

std::string strip_ansi(std::string s)
{
    if (!containsAnsi(s)) {
        return s;
    }

    std::ostringstream oss;
    strip_ansi(oss, s);
    return std::move(oss).str();
}

std::string_view toStringViewLowercase(const AnsiUnderlineStyle style)
{
#define X_CASE(_n, _lower, _UPPER, _Snake) \
    case AnsiUnderlineStyle::_Snake: \
        return #_lower;

    switch (style) {
        XFOREACH_ANSI_UNDERLINE_TYPE(X_CASE)
    default:
        std::abort();
    }

#undef X_CASE
}

namespace { // anonymous
void test_itu()
{
    static const auto ul = RawAnsi{AnsiStyleFlags{AnsiStyleFlagEnum::Underline},
                                   AnsiColorVariant{},
                                   AnsiColorVariant{},
                                   AnsiColorVariant{AnsiColorRGB{123, 45, 67}}};
    const auto expectOldStyle = "\033[4;58;2;123;45;67m";
    const auto expectItuColons = "\033[4:1;58:2::123:45:67m"; // note the doubled colon
    {
        const auto str_withSemicolons = ansi_transition(ANSI_COLOR_SUPPORT_RGB, RawAnsi{}, ul);
        TEST_ASSERT(str_withSemicolons.getStdStringView() == expectOldStyle);
    }
    {
        const auto str_withColons = ansi_transition(ANSI_COLOR_SUPPORT_ALL, RawAnsi{}, ul);
        TEST_ASSERT(str_withColons.getStdStringView() == expectItuColons);
    }

    {
        const auto opt = mmqt::parseAnsiColor({}, expectOldStyle);
        TEST_ASSERT(opt == ul);
    }
    {
        const auto opt = mmqt::parseAnsiColor({}, expectItuColons);
        TEST_ASSERT(opt == ul);
    }
    {
        // special case: we allow 5-element ITU-style for 24-bit ansi
        const auto noDouble = QString(expectItuColons).replace("::", ":");
        TEST_ASSERT(noDouble != expectItuColons);
        const auto opt = mmqt::parseAnsiColor({}, noDouble);
        TEST_ASSERT(opt == ul);
    }

    {
        // round-trip testing with underline types
#define X_CASE(_n, _lower, _UPPER, _Snake) \
    do { \
        const bool isNone = (AnsiUnderlineStyle::_Snake) == AnsiUnderlineStyle::None; \
        const auto copy = []() { \
            auto tmp = ul; \
            tmp.setUnderlineStyle(AnsiUnderlineStyle::_Snake); \
            return tmp; \
        }(); \
        TEST_ASSERT(copy.getUnderlineStyle() == AnsiUnderlineStyle::_Snake); \
        const auto str = ansi_string(ANSI_COLOR_SUPPORT_ALL, copy); \
        const auto opt = mmqt::parseAnsiColor({}, QString::fromUtf8(str.c_str())); \
        TEST_ASSERT(opt.has_value()); \
        TEST_ASSERT(opt == copy); \
        TEST_ASSERT(!isNone == opt.value().hasUnderline()); \
        TEST_ASSERT(opt.value().hasUnderlineColor()); \
        TEST_ASSERT(opt.value().getUnderlineStyle() == AnsiUnderlineStyle::_Snake); \
    } while (false);
        XFOREACH_ANSI_UNDERLINE_TYPE(X_CASE)
#undef X_CASE
    }

    {
        const volatile bool debugging = false;

        // round-trip test with high ansi fg, Itu256 bg, and ItuRGB underline color,
        // and Itu underline style, all in the same code.
        const auto kitchenSink = []() {
            auto tmp = ul;
            tmp.setUnderlineStyle(AnsiUnderlineStyle::Curly);
            tmp.fg = AnsiColorVariant{AnsiColor16Enum::RED};
            tmp.bg = AnsiColorVariant{AnsiColor256{42}};
            return tmp;
        }();
        {
            const auto str = ansi_string(ANSI_COLOR_SUPPORT_ALL, kitchenSink);
            if (debugging) {
                print_string_quoted(std::cout, str.getStdStringView());
                std::cout << "\n";
            }
            TEST_ASSERT(str.getStdStringView() == "\033[4:3;91;48:5:42;58:2::123:45:67m");
            const auto opt = mmqt::parseAnsiColor({}, QString::fromUtf8(str.c_str()));
            TEST_ASSERT(opt.has_value());
            TEST_ASSERT(opt == kitchenSink);
        }
        {
            const auto expect = []() {
                RawAnsi tmp;
                tmp.setUnderlineStyle(AnsiUnderlineStyle::Normal);
                tmp.fg = AnsiColorVariant(AnsiColor16Enum::red);
                tmp.bg = AnsiColorVariant(AnsiColor16Enum::cyan);
                return tmp;
            }();
            const auto str = ansi_string(ANSI_COLOR_SUPPORT_LO, kitchenSink);
            if (debugging) {
                print_string_quoted(std::cout, str.getStdStringView());
                std::cout << "\n";
            }
            TEST_ASSERT(str.getStdStringView() == "\033[4;31;46m");
            const auto opt = mmqt::parseAnsiColor({}, QString::fromUtf8(str.c_str()));
            TEST_ASSERT(opt.has_value());
            TEST_ASSERT(opt != kitchenSink);
            TEST_ASSERT(opt == expect);
            TEST_ASSERT(opt.value().hasUnderline());
            TEST_ASSERT(opt.value().getUnderlineStyle() == AnsiUnderlineStyle::Normal);
            TEST_ASSERT(!opt.value().hasUnderlineColor()); // NOTE: underline color requires ansi256
        }
        {
            const auto expect = []() {
                RawAnsi tmp;
                tmp.setUnderlineStyle(AnsiUnderlineStyle::Normal);
                tmp.fg = AnsiColorVariant(AnsiColor16Enum::red);
                tmp.bg = AnsiColorVariant{AnsiColor256{42}};
                tmp.ul = AnsiColorVariant{AnsiColor256{89}};
                return tmp;
            }();
            const auto str = ansi_string(ANSI_COLOR_SUPPORT_256, kitchenSink);
            if (debugging) {
                print_string_quoted(std::cout, str.getStdStringView());
                std::cout << "\n";
            }
            TEST_ASSERT(str.getStdStringView() == "\033[4;31;48;5;42;58;5;89m");
            const auto opt = mmqt::parseAnsiColor({}, QString::fromUtf8(str.c_str()));
            TEST_ASSERT(opt.has_value());
            TEST_ASSERT(opt != kitchenSink);
            TEST_ASSERT(opt == expect);
            TEST_ASSERT(opt.value().hasUnderline());
            TEST_ASSERT(opt.value().getUnderlineStyle() == AnsiUnderlineStyle::Normal);
            TEST_ASSERT(opt.value().hasUnderlineColor());
        }

        {
            const auto a = mmqt::parseAnsiColor({}, "\033[4m");
            TEST_ASSERT(a.has_value());
            TEST_ASSERT(a.value().hasUnderline());
            TEST_ASSERT(a.value().getUnderlineStyle() == AnsiUnderlineStyle::Normal);
            {
                const auto b = mmqt::parseAnsiColor({}, "\033[4:1m");
                TEST_ASSERT(a == b);
            }
            {
                const auto b = mmqt::parseAnsiColor(a.value(), "\033[24m");
                const auto c = mmqt::parseAnsiColor(a.value(), "\033[4:0m");
                TEST_ASSERT(b.has_value());
                TEST_ASSERT(b == RawAnsi{});
                TEST_ASSERT(!b.value().hasUnderline());
                TEST_ASSERT(b.value().getUnderlineStyle() == AnsiUnderlineStyle::None);
                TEST_ASSERT(a != b);
                TEST_ASSERT(b == c);
            }
            {
                const auto b = mmqt::parseAnsiColor(a.value(), "\033[4:2m");
                TEST_ASSERT(b.has_value());
                TEST_ASSERT(b.value().hasUnderline());
                TEST_ASSERT(b.value().getUnderlineStyle() == AnsiUnderlineStyle::Double);
                TEST_ASSERT(a != b);

                const auto c = mmqt::parseAnsiColor(b.value(), "\033[4:3m");
                TEST_ASSERT(c.has_value());
                TEST_ASSERT(c.value().hasUnderline());
                TEST_ASSERT(c.value().getUnderlineStyle() == AnsiUnderlineStyle::Curly);
                TEST_ASSERT(a != c);
                TEST_ASSERT(b != c);
            }
        }
    }

} // namespace

void test_ansi_parse()
{
    {
        auto tmp = ansi_parse({}, "\033[31;1");
        TEST_ASSERT(!tmp.has_value());
    }
    {
        RawAnsi expect = getRawAnsi(AnsiColor16Enum::red).withBold();
        auto tmp = ansi_parse({}, "\033[31;1m");
        TEST_ASSERT(tmp.has_value());
        TEST_ASSERT(tmp.value() == expect);
    }
    {
        RawAnsi expect = getRawAnsi(AnsiColor16Enum::red).withBold();
        expect.setUnderlineStyle(AnsiUnderlineStyle::Curly);
        auto tmp = ansi_parse({}, "\033[31;4:3;1m");
        TEST_ASSERT(tmp.has_value());
        TEST_ASSERT(tmp.value() == expect);
    }
    {
        RawAnsi expect = getRawAnsi(AnsiColor16Enum::red).withBold();
        expect.setUnderline();
        auto tmp = ansi_parse({}, "\033[31;4;1m");
        TEST_ASSERT(tmp.has_value());
        TEST_ASSERT(tmp.value() == expect);
    }
    {
        RawAnsi expect = getRawAnsi(AnsiColor16Enum::red);
        expect.setUnderline();
        auto tmp = ansi_parse({}, "\033[31;4:1m");
        TEST_ASSERT(tmp.has_value());
        TEST_ASSERT(tmp.value() == expect);
    }
    {
        RawAnsi expect;
        auto tmp = ansi_parse(RawAnsi{}.withBold(), "\033[21m");
        TEST_ASSERT(tmp.has_value());
        TEST_ASSERT(tmp.value() == expect);
    }
}
} // namespace

namespace test {

// NOTE: This was originally a "visual test" shown at startup in debug mode,
// so it does not work well as a unit test.
void testAnsiTextUtils()
{
    auto buildState = [](std::initializer_list<int> values) -> AnsiColorState {
        AnsiColorState state;
        for (auto &x : values) {
            state.receive(x);
        }
        return state;
    };

    auto getRawAnsi = [&buildState](std::initializer_list<int> values) {
        auto state = buildState(values);
        TEST_ASSERT(state.getState() == AnsiColorState::StatusEnum::Pass);
        return state.getRawAnsi();
    };

    auto checkValid = [&getRawAnsi](std::initializer_list<int> values, const RawAnsi expect) {
        auto raw = getRawAnsi(values);
        TEST_ASSERT(raw == expect);
    };

    const auto noColor = AnsiColorVariant{};
    const AnsiStyleFlags noState;
    const auto black = AnsiColorVariant{AnsiColor256{0}};
    const auto BLACK = AnsiColorVariant{AnsiColor256{8}};

    const auto empty = RawAnsi{};
    const auto altEmpty = RawAnsi{noState, noColor, noColor, noColor};
    TEST_ASSERT(empty == altEmpty);

    const auto black_fg = RawAnsi{noState, black, noColor, noColor};
    const auto BLACK_fg = RawAnsi{noState, BLACK, noColor, noColor};

    checkValid({}, empty);

#define X_CHECK(_n, _lower, _UPPER, _Snake) checkValid({(_n), (_n) + ANSI_REVERT_OFFSET}, empty);
    XFOREACH_ANSI_STYLE(X_CHECK)
#undef X_CHECK

    checkValid({ANSI_FG_COLOR}, black_fg);
    checkValid({ANSI_FG_COLOR_HI}, BLACK_fg);
    checkValid({ANSI_FG_EXT, ANSI_EXT_256, 0}, black_fg);
    checkValid({ANSI_FG_EXT, ANSI_EXT_256, 8}, BLACK_fg);
    checkValid({ANSI_FG_EXT, ANSI_EXT_256, 0, ANSI_RESET}, empty);
    checkValid({ANSI_RESET, ANSI_FG_EXT, ANSI_EXT_256, 0}, black_fg);
    checkValid({ANSI_RESET, ANSI_FG_EXT, ANSI_EXT_256, 8}, BLACK_fg);

#define X_CHECK(_number, _lower, _UPPER) \
    checkValid({ANSI_FG_COLOR + (_number), ANSI_FG_DEFAULT}, empty); \
    checkValid({ANSI_FG_COLOR_HI + (_number), ANSI_FG_DEFAULT}, empty); \
    checkValid({ANSI_BG_COLOR + (_number), ANSI_BG_DEFAULT}, empty); \
    checkValid({ANSI_BG_COLOR_HI + (_number), ANSI_BG_DEFAULT}, empty);
    XFOREACH_ANSI_COLOR_0_7(X_CHECK)
#undef X_CHECK

#define X_CASE(_number, _lower, _UPPER, _Snake) \
    checkValid({(_number), ANSI_FG_EXT, ANSI_EXT_256, 0}, \
               RawAnsi{AnsiStyleFlags{AnsiStyleFlagEnum::_Snake}, black, noColor, noColor});
    XFOREACH_ANSI_STYLE(X_CASE)
#undef X_CASE

    auto check2 = [&getRawAnsi](std::initializer_list<int> init_before,
                                std::initializer_list<int> init_after,
                                const std::string_view expect) {
        const auto before = getRawAnsi(init_before);
        const auto after = getRawAnsi(init_after);

        const auto lo = ansi_transition(ANSI_COLOR_SUPPORT_LO, before, after); // 3-bit color
        const auto hi = ansi_transition(ANSI_COLOR_SUPPORT_HI, before, after); // 4-bit color
        const auto ext = ansi_transition(ANSI_COLOR_SUPPORT_256, before,
                                         after); // 8-bit color
        const auto full = ansi_transition(ANSI_COLOR_SUPPORT_RGB, before,
                                          after); // 24-bit color

        const auto sv = hi.getStdStringView();
        TEST_ASSERT(sv == expect);
    };

    check2({}, {ANSI_FG_COLOR}, "\033[30m");
    check2({ANSI_FG_COLOR}, {}, "\033[0m");
    check2({ANSI_FG_COLOR, ANSI_BOLD}, {ANSI_BOLD}, "\033[39m");
    check2({ANSI_FG_COLOR}, {31}, "\033[31m");
    check2({ANSI_FG_COLOR}, {ANSI_FG_DEFAULT}, "\033[0m");
    check2({ANSI_FG_COLOR, ANSI_BOLD}, {ANSI_FG_DEFAULT, 1}, "\033[39m");
    check2({ANSI_FG_COLOR}, {ANSI_BG_COLOR}, "\033[0;40m"); // shorter than "\033[39;40m"
    check2({ANSI_BOLD, ANSI_FG_COLOR},
           {ANSI_BOLD, 40},
           "\033[39;40m"); // shorter than "\033[0;1;40m"

    // bold cannot be removed with "\033[21m"
    check2({ANSI_FG_COLOR, ANSI_BOLD}, {ANSI_FG_COLOR}, "\033[0;30m");
    check2({ANSI_FG_COLOR}, {ANSI_FG_COLOR, ANSI_BOLD}, "\033[1m");
    check2({}, {ANSI_UL_EXT, ANSI_EXT_256, 0}, "");

    std::ostringstream os;

    // Tracing 6 of the 12 edges of the color cube, plus the main diagonal:
    for (int bits = 1; bits < 8; ++bits) {
        const int r = bits & 1;
        const int g = (bits & 2) / 2;
        const int b = (bits & 4) / 4;
        os << "On the black-" << AnsiColor16{static_cast<AnsiColor16Enum>(bits)} << " axis ...\n";
        AnsiColor16 tmp;
        for (int i = 0; i < 256; ++i) {
            const auto rgb = AnsiColorRGB{static_cast<uint8_t>(r * i),
                                          static_cast<uint8_t>(g * i),
                                          static_cast<uint8_t>(b * i)};
            const auto conv = AnsiColor16{toAnsiColor16Enum(rgb)};
            if (conv != tmp) {
                os << " ... " << rgb << " becomes " << conv << "\n";
                tmp = conv;
            }
        }
    }
    os << "\nand\n\n";

    // tracing the other 6 edges of the color cube
    for (int bits = 1; bits < 7; ++bits) {
        const int r = bits & 1;
        const int g = (bits & 2) / 2;
        const int b = (bits & 4) / 4;
        os << "On the " << AnsiColor16{static_cast<AnsiColor16Enum>(bits)} << "-white axis ...\n";

        AnsiColor16 tmp;
        for (int i = 0; i < 256; ++i) {
            const auto rgb = AnsiColorRGB{static_cast<uint8_t>(r * 255 + (1 - r) * i),
                                          static_cast<uint8_t>(g * 255 + (1 - g) * i),
                                          static_cast<uint8_t>(b * 255 + (1 - b) * i)};

            const auto conv = AnsiColor16{toAnsiColor16Enum(rgb)};
            if (conv != tmp) {
                os << " ... " << rgb << " becomes " << conv << "\n";
                tmp = conv;
            }
        }
    }

    {
#define X_CHECK(X, lower, UPPER) \
    TEST_ASSERT(getClosestMatchInColorSpace(toAnsiColorRGB(AnsiColor16Enum::lower)) \
                == AnsiColor16Enum::lower); \
    TEST_ASSERT(getClosestMatchInColorSpace(toAnsiColorRGB(AnsiColor16Enum::UPPER)) \
                == AnsiColor16Enum::UPPER);
        XFOREACH_ANSI_COLOR_0_7(X_CHECK)
#undef X_CHECK
    }

    bool failed = false;

    {
        for (int i = 0; i < ANSI256_RGB6_START; ++i) {
            const AnsiColor256 input{static_cast<uint8_t>(i)};
            const AnsiColor16Enum output = toAnsiColor16Enum(input);
            const AnsiColor256 roundTrip = toAnsiColor256(output);

            if (input == roundTrip) {
                continue;
            }

            failed = true;
            os << "WARNING: round-trip-failure: " << input << " converts to " << AnsiColor16{output}
               << ", which converts back to " << roundTrip << ".\n";
        }

        for (int i = 0; i < 256; ++i) {
            const AnsiColor256 input{static_cast<uint8_t>(i)};
            const AnsiColorRGB output = toAnsiColorRGB(input);
            const AnsiColor256 roundTrip = toAnsiColor256(output);

            if (input == roundTrip) {
                continue;
            }

            const bool shouldWarn = [input, roundTrip]() -> bool {
                switch (input.color) {
                case 59:
                    return roundTrip.color != 240;
                case 102:
                    return roundTrip.color != 245;
                case 145:
                    return roundTrip.color != 249;
                case 188:
                    return roundTrip.color != 253;
                default:
                    return input.color >= ANSI256_RGB6_START;
                }
            }();

            // values < 16 aren't expected to be able to round-trip.
            os << (shouldWarn ? "WARNING: round-trip-failure" : "INFO");
            os << ": " << input;
            os << " converts to " << output;
            os << ", which converts back to " << roundTrip;
            os << ".\n";

            if (shouldWarn) {
                failed = true;
            }
        }
    }

    MMLOG() << std::move(os).str();
    TEST_ASSERT(!failed);

    test_itu();
    test_ansi_parse();
}

} // namespace test
