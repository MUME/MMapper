#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "Charset.h"
#include "Color.h"
#include "Consts.h"
#include "Flags.h"
#include "TextBuffer.h"
#include "macros.h"
#include "utils.h"

#include <array>
#include <optional>
#include <ostream>
#include <tuple>
#include <variant>

#include <QColor>
#include <QRegularExpression>

// Not supported by MUME:
// 6: fast blink
// 8: conceal (also, this "feature" is ANTI-user, so please don't enable it by default)

// X(_n, _lower, _UPPER, _Snake)
#define XFOREACH_ANSI_STYLE_EXCEPT_UNDERLINE(X) \
    X(1, bold, BOLD, Bold) \
    X(2, faint, FAINT, Faint) \
    X(3, italic, ITALIC, Italic) \
    X(5, blink, BLINK, Blink) \
    X(7, reverse, REVERSE, Reverse) \
    X(8, conceal, CONCEAL, Conceal) \
    X(9, strikeout, STRIKEOUT, Strikeout)

// X(n, lower, UPPER)
#define XFOREACH_ANSI_STYLE(X) \
    XFOREACH_ANSI_STYLE_EXCEPT_UNDERLINE(X) X(4, underline, UNDERLINE, Underline)

// X(n, lower, UPPER)
#define XFOREACH_ANSI_COLOR_0_7(X) \
    X(0, black, BLACK) \
    X(1, red, RED) \
    X(2, green, GREEN) \
    X(3, yellow, YELLOW) \
    X(4, blue, BLUE) \
    X(5, magenta, MAGENTA) \
    X(6, cyan, CYAN) \
    X(7, white, WHITE)

enum class NODISCARD AnsiStyleFlagEnum : uint8_t {
#define X_DECL(_number, _lower, _UPPER, _Snake) _Snake,
    XFOREACH_ANSI_STYLE(X_DECL)
#undef X_DECL
};

NODISCARD std::string_view to_string_view(AnsiStyleFlagEnum flag, bool uppercase);
NODISCARD inline std::string_view to_string_view(const AnsiStyleFlagEnum flag)
{
    return to_string_view(flag, false);
}

#define X_COUNT(_number, _lower, _UPPER, _snake) +1
static constexpr const size_t NUM_ANSI_FLAGS = (XFOREACH_ANSI_STYLE(X_COUNT));
#undef X_COUNT
static_assert(NUM_ANSI_FLAGS == 8);

DEFINE_ENUM_COUNT(AnsiStyleFlagEnum, NUM_ANSI_FLAGS);

struct NODISCARD AnsiStyleFlags final : enums::Flags<AnsiStyleFlags, AnsiStyleFlagEnum, uint8_t>
{
    using Flags::Flags;
};

#define X_DECL_LO(n, lower, UPPER) lower,
#define X_DECL_HI(n, lower, UPPER) UPPER,
enum class NODISCARD AnsiColor16Enum {
    XFOREACH_ANSI_COLOR_0_7(X_DECL_LO) //
    XFOREACH_ANSI_COLOR_0_7(X_DECL_HI) //
};
#undef X_DECL_LO
#undef X_DECL_HI

// NOTE: 16-color ANSI only supports foreground and background colors
// (even though 256-color and RGB support underline color).
enum class NODISCARD AnsiColor16LocationEnum { Foreground, Background };

// 0..15
NODISCARD int getIndex16(AnsiColor16Enum c);
NODISCARD bool isLow(AnsiColor16Enum c);
NODISCARD bool isHigh(AnsiColor16Enum c);
NODISCARD AnsiColor16Enum toLow(AnsiColor16Enum c);
NODISCARD AnsiColor16Enum toHigh(AnsiColor16Enum c);

struct NODISCARD AnsiColor16 final
{
    std::optional<AnsiColor16Enum> color;

public:
    NODISCARD bool hasColor() const { return color.has_value(); }
    NODISCARD bool isDefault() const { return !hasColor(); }
    NODISCARD AnsiColor16Enum getColor() const { return color.value(); }

public:
    NODISCARD constexpr bool operator==(const AnsiColor16 &rhs) const { return color == rhs.color; }
    NODISCARD constexpr bool operator!=(const AnsiColor16 &rhs) const { return !(rhs == *this); }

public:
    NODISCARD bool isLow() const { return hasColor() && ::isLow(color.value()); }
    NODISCARD bool isHigh() const { return hasColor() && ::isHigh(color.value()); }

public:
    NODISCARD std::string_view to_string_view() const
    {
        if (!hasColor()) {
            return "default";
        }

#define X_CASE(n, lower, UPPER) \
    case AnsiColor16Enum::lower: \
        return #lower; \
    case AnsiColor16Enum::UPPER: \
        return #UPPER;
        switch (color.value()) {
            XFOREACH_ANSI_COLOR_0_7(X_CASE) //
        default:
            break;
        }
        assert(false);
        return "*error*";
#undef X_CASE
    }

public:
    NODISCARD friend std::string_view to_string_view(const AnsiColor16 c)
    {
        return c.to_string_view();
    }
    friend std::ostream &to_stream(std::ostream &os, const AnsiColor16 c)
    {
        return os << c.to_string_view();
    }
    friend std::ostream &operator<<(std::ostream &os, const AnsiColor16 c)
    {
        return to_stream(os, c);
    }
};

struct NODISCARD AnsiColor256 final
{
    uint8_t color = 0;

public:
    NODISCARD bool operator==(const AnsiColor256 &rhs) const { return color == rhs.color; }
    NODISCARD bool operator!=(const AnsiColor256 &rhs) const { return !(rhs == *this); }

public:
    friend std::ostream &to_stream(std::ostream &os, AnsiColor256 c);
    friend std::ostream &operator<<(std::ostream &os, const AnsiColor256 c)
    {
        return to_stream(os, c);
    }
};

struct NODISCARD AnsiColorRGB final
{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

public:
    NODISCARD bool operator==(const AnsiColorRGB &rhs) const
    {
        return r == rhs.r && g == rhs.g && b == rhs.b;
    }
    NODISCARD bool operator!=(const AnsiColorRGB &rhs) const { return !(rhs == *this); }

public:
    friend std::ostream &to_stream(std::ostream &os, const AnsiColorRGB c)
    {
        os << "AnsiColorRGB{" << static_cast<uint32_t>(c.r) << ", " << static_cast<uint32_t>(c.g)
           << ", " << static_cast<uint32_t>(c.b) << "}";
        return os;
    }
    friend std::ostream &operator<<(std::ostream &os, const AnsiColorRGB c)
    {
        return to_stream(os, c);
    }
};

NODISCARD static inline constexpr AnsiColor256 toAnsiColor256(const AnsiColor16Enum color)
{
#define X_CASE(X, lower, UPPER) \
    case AnsiColor16Enum::lower: \
        return AnsiColor256{static_cast<uint8_t>(X)}; \
    case AnsiColor16Enum::UPPER: \
        return AnsiColor256{static_cast<uint8_t>(static_cast<int>(X) + 8)};

    switch (color) {
        XFOREACH_ANSI_COLOR_0_7(X_CASE)
    }

    assert(false);
    return {};
#undef X_CASE
}

// WARNING: Lossy!
NODISCARD AnsiColor256 toAnsiColor256(AnsiColorRGB rgb);

NODISCARD AnsiColorRGB toAnsiColorRGB(AnsiColor16Enum ansi);
NODISCARD AnsiColorRGB toAnsiColorRGB(AnsiColor256 ansiColor);
NODISCARD AnsiColorRGB toAnsiColorRGB(const Color &c);

// lossy, except when rgb is one of the table entries
NODISCARD AnsiColor16Enum toAnsiColor16Enum(AnsiColorRGB rgb);
// perfect conversion for 0..15, but lossy for 16-255.
NODISCARD AnsiColor16Enum toAnsiColor16Enum(AnsiColor256 ansi256);

struct NODISCARD AnsiColorVariant final
{
private:
    std::variant<std::monostate, AnsiColor256, AnsiColorRGB> m_var;

public:
    constexpr AnsiColorVariant()
        : m_var(std::monostate())
    {}

    explicit constexpr AnsiColorVariant(const AnsiColor16Enum x)
        : m_var{toAnsiColor256(x)}
    {}
    explicit AnsiColorVariant(const AnsiColor16 x) { *this = x; }

    explicit AnsiColorVariant(const AnsiColor256 x)
        : m_var{x}
    {}
    explicit AnsiColorVariant(const AnsiColorRGB x)
        : m_var{x}
    {}

public:
    AnsiColorVariant &operator=(const AnsiColor16 x)
    {
        if (x.hasColor()) {
            m_var = toAnsiColor256(x.getColor());
        }
        return *this;
    }
    AnsiColorVariant &operator=(const AnsiColor256 x)
    {
        m_var = x;
        return *this;
    }
    AnsiColorVariant &operator=(const AnsiColorRGB x)
    {
        m_var = x;
        return *this;
    }

public:
    NODISCARD constexpr bool hasDefaultColor() const
    {
        return std::holds_alternative<std::monostate>(m_var);
    }
    NODISCARD constexpr explicit operator bool() const { return !hasDefaultColor(); }
    NODISCARD constexpr bool has256() const { return std::holds_alternative<AnsiColor256>(m_var); }
    NODISCARD constexpr bool hasRGB() const { return std::holds_alternative<AnsiColorRGB>(m_var); }

public:
    NODISCARD constexpr AnsiColor256 get256() const { return std::get<AnsiColor256>(m_var); }
    NODISCARD constexpr AnsiColorRGB getRGB() const { return std::get<AnsiColorRGB>(m_var); }

public:
    NODISCARD constexpr bool operator==(const AnsiColorVariant &rhs) const
    {
        return m_var == rhs.m_var;
    }
    NODISCARD constexpr bool operator!=(const AnsiColorVariant &rhs) const
    {
        return !(rhs == *this);
    }

public:
    friend std::ostream &to_stream(std::ostream &os, const AnsiColorVariant c)
    {
        if (c.hasDefaultColor()) {
            os << AnsiColor16{};
        } else if (c.has256()) {
            os << c.get256();
        } else if (c.hasRGB()) {
            os << c.getRGB();
        } else {
            assert(false);
            os << "*error*";
        }

        return os;
    }
    friend std::ostream &operator<<(std::ostream &os, const AnsiColorVariant c)
    {
        return to_stream(os, c);
    }
};

NODISCARD AnsiColor16 toAnsiColor16(AnsiColorVariant var);

// X(_n, _lower, _UPPER, _Snake)
#define XFOREACH_ANSI_UNDERLINE_TYPE(X) \
    X(0, none, NONE, None) \
    X(1, normal, NORMAL, Normal) \
    X(2, double, DOUBLE, Double) \
    X(3, curly, CURLY, Curly) \
    X(4, dotted, DOTTED, Dotted) \
    X(5, dashed, DASHED, Dashed)

#define X_DECL_UL_TYPE(_n, _lower, _UPPER, _Snake) _Snake = (_n),
enum class NODISCARD AnsiUnderlineStyleEnum : uint8_t {
    XFOREACH_ANSI_UNDERLINE_TYPE(X_DECL_UL_TYPE)
};
#undef X_DECL_UL_TYPE

NODISCARD inline std::string_view toStringViewLowercase(AnsiUnderlineStyleEnum style);

struct NODISCARD RawAnsi final
{
    AnsiColorVariant fg;
    AnsiColorVariant bg;
    AnsiColorVariant ul;

private:
    AnsiStyleFlags m_flags; // only bits 0..9 are used; the other 6 are reserved
    AnsiUnderlineStyleEnum m_underlineStyle = AnsiUnderlineStyleEnum::None;

public:
    constexpr RawAnsi() = default;
    constexpr RawAnsi(const AnsiStyleFlags flags,
                      const AnsiColorVariant fg_,
                      const AnsiColorVariant bg_,
                      const AnsiColorVariant ul_)
        : fg{fg_}
        , bg{bg_}
        , ul{ul_}
        , m_flags{flags}
    {
        // REVISIT: consider looping over input flags and setting each individually;
        // currently this is fine if underline is the only special case.
        if (m_flags.contains(AnsiStyleFlagEnum::Underline)) {
            setUnderline(); // normal
        }
    }

public:
    NODISCARD constexpr bool hasForegroundColor() const { return !fg.hasDefaultColor(); }
    NODISCARD constexpr bool hasBackgroundColor() const { return !bg.hasDefaultColor(); }
    NODISCARD constexpr bool hasUnderlineColor() const { return !ul.hasDefaultColor(); }

public:
#define X_DECL_WITH(_number, _lower, _UPPER, _Snake) \
    NODISCARD constexpr RawAnsi with##_Snake() const \
    { \
        auto copy = *this; \
        copy.set##_Snake(); \
        return copy; \
    } \
    NODISCARD constexpr RawAnsi without##_Snake() const \
    { \
        auto copy = *this; \
        copy.clear##_Snake(); \
        return copy; \
    } \
    NODISCARD constexpr RawAnsi withToggled##_Snake() const \
    { \
        auto copy = *this; \
        copy.toggle##_Snake(); \
        return copy; \
    }
    XFOREACH_ANSI_STYLE(X_DECL_WITH)
#undef X_DECL_WITH

public:
    NODISCARD constexpr RawAnsi withForeground(const AnsiColorVariant var) const
    {
        auto copy = *this;
        copy.fg = var;
        return copy;
    }
    NODISCARD constexpr RawAnsi withBackground(const AnsiColorVariant var) const
    {
        auto copy = *this;
        copy.bg = var;
        return copy;
    }
    NODISCARD constexpr RawAnsi withUnderlineColor(const AnsiColorVariant var) const
    {
        auto copy = *this;
        copy.ul = var;
        return copy;
    }
    NODISCARD constexpr RawAnsi withUnderlineStyle(const AnsiUnderlineStyleEnum style) const
    {
        auto copy = *this;
        copy.setUnderlineStyle(style);
        return copy;
    }

public:
    NODISCARD constexpr RawAnsi withForeground(const AnsiColor16Enum newColor) const
    {
        return withForeground(AnsiColorVariant{newColor});
    }
    NODISCARD constexpr RawAnsi withBackground(const AnsiColor16Enum newColor) const
    {
        return withBackground(AnsiColorVariant{newColor});
    }
    NODISCARD constexpr RawAnsi withUnderlineColor(const AnsiColor16Enum newColor) const
    {
        return withUnderlineColor(AnsiColorVariant{newColor});
    }

public:
#define X_DECL_ACCESSORS(_number, _lower, _UPPER, _Snake) \
    NODISCARD constexpr bool has##_Snake() const \
    { \
        return m_flags.contains(AnsiStyleFlagEnum::_Snake); \
    } \
    constexpr void set##_Snake() \
    { \
        m_flags.insert(AnsiStyleFlagEnum::_Snake); \
    } \
    constexpr void clear##_Snake() \
    { \
        m_flags.remove(AnsiStyleFlagEnum::_Snake); \
    }
    XFOREACH_ANSI_STYLE_EXCEPT_UNDERLINE(X_DECL_ACCESSORS)
#undef X_DECL_ACCESSORS

#define X_DECL_ACCESSORS(_number, _lower, _UPPER, _Snake) \
    constexpr void toggle##_Snake() \
    { \
        if (has##_Snake()) { \
            clear##_Snake(); \
        } else { \
            set##_Snake(); \
        } \
    }
    XFOREACH_ANSI_STYLE(X_DECL_ACCESSORS)
#undef X_DECL_ACCESSORS

public:
    NODISCARD constexpr bool hasUnderline() const
    {
        return m_flags.contains(AnsiStyleFlagEnum::Underline);
    }
    constexpr void setUnderline()
    {
        setUnderlineStyle(AnsiUnderlineStyleEnum::Normal);
    }
    constexpr void clearUnderline()
    {
        m_flags.remove(AnsiStyleFlagEnum::Underline);
        m_underlineStyle = AnsiUnderlineStyleEnum::None;
    }

public:
    constexpr void setUnderlineStyle(const AnsiUnderlineStyleEnum style)
    {
        if (style == AnsiUnderlineStyleEnum::None) {
            clearUnderline();
        } else {
            m_flags.insert(AnsiStyleFlagEnum::Underline);
            m_underlineStyle = style;
        }
    }

public:
    NODISCARD constexpr AnsiStyleFlags getFlags() const
    {
        return m_flags;
    }
    NODISCARD constexpr AnsiUnderlineStyleEnum getUnderlineStyle() const
    {
        return m_underlineStyle;
    }

public:
    constexpr void setFlag(const AnsiStyleFlagEnum flag)
    {
#define X_CASE(_number, _lower, _UPPER, _Snake) \
    case (AnsiStyleFlagEnum::_Snake): \
        set##_Snake(); \
        return;

        switch (flag) {
            XFOREACH_ANSI_STYLE(X_CASE)
        }

        std::abort();
#undef X_CASE
    }
    constexpr void removeFlag(const AnsiStyleFlagEnum flag)
    {
#define X_CASE(_number, _lower, _UPPER, _Snake) \
    case (AnsiStyleFlagEnum::_Snake): \
        clear##_Snake(); \
        return;

        switch (flag) {
            XFOREACH_ANSI_STYLE(X_CASE)
        }

        std::abort();
#undef X_CASE
    }

public:
    NODISCARD constexpr bool operator==(const RawAnsi &rhs) const
    {
        return fg == rhs.fg && bg == rhs.bg && ul == rhs.ul && m_flags == rhs.m_flags
               && m_underlineStyle == rhs.m_underlineStyle;
    }
    NODISCARD constexpr bool operator!=(const RawAnsi &rhs) const
    {
        return !(rhs == *this);
    }

public:
    friend std::ostream &to_stream(std::ostream &os, const RawAnsi &raw);
    friend std::ostream &operator<<(std::ostream &os, const RawAnsi &raw)
    {
        return to_stream(os, raw);
    }
};

NODISCARD static inline constexpr RawAnsi getRawAnsi(const AnsiColor16Enum fgColor)
{
    RawAnsi ansi;
    ansi.fg = AnsiColorVariant{fgColor};
    return ansi;
}

NODISCARD static inline constexpr RawAnsi getRawAnsi(const AnsiColor16Enum fgColor,
                                                     const AnsiColor16Enum bgColor)
{
    RawAnsi ansi;
    ansi.fg = AnsiColorVariant{fgColor};
    ansi.bg = AnsiColorVariant{bgColor};
    return ansi;
}

struct NODISCARD AnsiItuColorCodes final
{
private:
    // https://en.wikipedia.org/wiki/ANSI_escape_code#24-bit says:
    // > ESC [ 38 : 2 : ⟨Color-Space-ID⟩ : ⟨r⟩ : ⟨g⟩ : ⟨b⟩ : ⟨unused⟩ : ⟨CS tolerance⟩
    // > : ⟨Color-Space associated with tolerance: 0 for "CIELUV"; 1 for "CIELAB"⟩ m
    //
    // so that's:
    // 0: 38
    // 1: 2
    // 2: Color-Space-ID
    // 3: r
    // 4: g
    // 5: b
    // 6: unused
    // 7: CS tolerance
    // 8: Color-Space associated with tolerance
    //
    // note: MMapper will ignore [2], [6], [7], and [8], and will assume RGB values are in 0-255
    // even though some terminals use color-space ID of 100 to indicate that values go from 0 to 100.

    static inline constexpr size_t MAX_ELEMENTS = 9;
    using SmallInt = uint8_t;
    std::array<SmallInt, MAX_ELEMENTS> m_arr{};
    uint8_t m_size = 0;
    bool m_overflowed = false;

public:
    NODISCARD size_t size() const { return m_size; }
    NODISCARD bool empty() const { return m_size == 0; }
    NODISCARD bool overflowed() const { return m_overflowed; }
    NODISCARD int at(const size_t pos) const
    {
        if (pos >= m_size) {
            throw std::runtime_error("read out of bounds");
        }
        return m_arr[pos];
    }
    NODISCARD int front() const { return at(0); }
    NODISCARD int operator[](const size_t pos) const { return at(pos); }

public:
    class ALLOW_DISCARD ConstIterator final
    {
    private:
        const AnsiItuColorCodes *m_self = nullptr;
        uint8_t m_pos = 0;

    public:
        NODISCARD explicit ConstIterator(const AnsiItuColorCodes *const self, const uint8_t pos)
            : m_self{self}
            , m_pos{pos}
        {
            std::ignore = deref(self);
        }

    private:
        NODISCARD const AnsiItuColorCodes &getSelf() const { return deref(m_self); }

    public:
        void operator++(int) = delete;
        ALLOW_DISCARD ConstIterator &operator++()
        {
            if (m_pos >= getSelf().size()) {
                throw std::runtime_error("invalid increment");
            }
            ++m_pos;
            return *this;
        }
        NODISCARD int operator*() const { return getSelf().at(m_pos); }
        NODISCARD bool operator==(const ConstIterator other) const
        {
            return m_self == other.m_self && m_pos == other.m_pos;
        }
        NODISCARD bool operator!=(const ConstIterator other) const { return !operator==(other); }
    };
    NODISCARD ConstIterator begin() const { return ConstIterator{this, 0}; }
    NODISCARD ConstIterator end() const { return ConstIterator{this, m_size}; }

public:
    void push_back(const int n)
    {
        if (size() >= MAX_ELEMENTS || n < 0 || n > std::numeric_limits<SmallInt>::max()) {
            m_overflowed = true;
            return;
        }
        m_arr[m_size] = static_cast<uint8_t>(n);
        ++m_size;
    }
    void clear()
    {
        m_size = 0;
        m_overflowed = false;
    }
};

struct NODISCARD AnsiColorState final
{
public:
    enum class NODISCARD StatusEnum { Fail, Pass, Incomplete };

private:
    enum class NODISCARD InternalStateEnum : uint8_t { Normal, Ext, Ext256, ExtRGB, Fail };
    struct NODISCARD Stack final
    {
        std::array<uint8_t, 5> m_buffer{};
        uint8_t m_size = 0;
        NODISCARD size_t size() const { return static_cast<size_t>(m_size); }
        void clear() { *this = {}; }
        void push(int n)
        {
            assert(isClamped(n, 0, 255));
            m_buffer.at(m_size++) = static_cast<uint8_t>(std::clamp(n, 0, 255));
        }
        template<size_t N>
        NODISCARD std::array<uint8_t, N> getAndClear()
        {
            static_assert(N == 3 || N == 5);
            assert(size() == N);
            std::array<uint8_t, N> result;
            for (size_t i = 0; i < N; ++i) {
                result[i] = m_buffer[i];
            }
            clear();
            return result;
        }
    };

private:
    InternalStateEnum m_state = InternalStateEnum::Normal;
    Stack m_stack;
    RawAnsi m_raw;

public:
    AnsiColorState() = default;
    explicit AnsiColorState(const RawAnsi raw)
        : m_raw{raw}
    {}

public:
    NODISCARD StatusEnum getState() const
    {
        switch (m_state) {
        case InternalStateEnum::Normal:
            return StatusEnum::Pass;

        case InternalStateEnum::Ext:
            return StatusEnum::Incomplete;

        case InternalStateEnum::Ext256:
        case InternalStateEnum::ExtRGB:
            // REVISIT: handling of implicit zeros?
            //
            // e.g. "[38;5m" may be a valid encoding of "[38;5;0m",
            // and "[38;2m" maybe a valid encoding of "[38;2;0;0;0m"
            //
            // If we want to support that functionality, we'll need a flush() function
            // that actually processes the incomplete data in these two states.
            return StatusEnum::Incomplete;

        case InternalStateEnum::Fail:
            return StatusEnum::Fail;
        }

        std::abort();
    }

    NODISCARD bool hasInvalidState() const { return m_state == InternalStateEnum::Fail; }
    // Complete implies Pass
    NODISCARD bool hasCompleteState() const { return m_state == InternalStateEnum::Normal; }
    // Means it's currently processing an extended ANSI code.
    NODISCARD bool hasIncompleteState() const { return !hasInvalidState() && !hasCompleteState(); }

public:
    NODISCARD RawAnsi getRawAnsi() const { return m_raw; }

public:
    void reset() { *this = {}; }

public:
    void receive(int n);
    void receive_itu(const AnsiItuColorCodes &codes);

private:
    void state_normal(int n);
    void state_ext(int n);
    void state_ext256(int n);
    void state_extRGB(int n);
};

// Caution: This does not perform *ANY* checking on the codes.
//
// If you're considering using this to manually insert codes,
// then you should probably consider using AnsiColorState to generate a RawAnsi,
// and then call ansi_string() or ansi_transition() to return an AnsiString.
class NODISCARD AnsiString final
{
private:
    std::string m_buffer;

public:
    AnsiString();

private:
    void add_code_common(bool useItuColon);

public:
    void add_empty_code(bool useItuColon = false);
    void add_code(int code, bool useItuColon = false);
    NODISCARD AnsiString copy_as_reset() const;

public:
    NODISCARD inline bool isEmpty() const { return m_buffer.empty(); }
    NODISCARD inline int size() const { return static_cast<int>(m_buffer.size()); }
    NODISCARD const char *c_str() const { return m_buffer.c_str(); }
    NODISCARD const std::string &getStdString() const { return m_buffer; }
    NODISCARD std::string_view getStdStringView() const { return m_buffer; }

public:
    NODISCARD static AnsiString get_reset_string();
};

enum class NODISCARD AnsiSupportFlagEnum : uint8_t {
    AnsiHI,
    Ansi256,
    AnsiRGB,
    Itu256,
    ItuRGB,
    ItuUnderline,
    // make sure to update NUM_ANSI_SUPPORT_FLAGS if you add any flags
};
static inline constexpr const size_t NUM_ANSI_SUPPORT_FLAGS = 6;
struct NODISCARD AnsiSupportFlags final
    : enums::Flags<AnsiSupportFlags, AnsiSupportFlagEnum, uint8_t, NUM_ANSI_SUPPORT_FLAGS>
{
    using Flags::Flags;
};

// 3-bit color
static inline constexpr AnsiSupportFlags ANSI_COLOR_SUPPORT_LO{};
// 4-bit color
static inline constexpr AnsiSupportFlags ANSI_COLOR_SUPPORT_HI{AnsiSupportFlagEnum::AnsiHI};
// 8-bit color
static inline constexpr AnsiSupportFlags ANSI_COLOR_SUPPORT_256{AnsiSupportFlagEnum::Ansi256};
// 24-bit color
static inline constexpr AnsiSupportFlags ANSI_COLOR_SUPPORT_RGB{AnsiSupportFlagEnum::AnsiRGB};

// 8-bit color using colons
static inline constexpr AnsiSupportFlags ANSI_COLOR_SUPPORT_ITU_256{AnsiSupportFlagEnum::Itu256};
// 24-bit color using colons
static inline constexpr AnsiSupportFlags ANSI_COLOR_SUPPORT_ITU_RGB{AnsiSupportFlagEnum::ItuRGB};
// underline styles using colons
static inline constexpr AnsiSupportFlags ANSI_COLOR_SUPPORT_ITU_UNDERLINE{
    AnsiSupportFlagEnum::ItuUnderline};

static inline constexpr AnsiSupportFlags ANSI_COLOR_SUPPORT_ALL = ANSI_COLOR_SUPPORT_HI
                                                                  | ANSI_COLOR_SUPPORT_256
                                                                  | ANSI_COLOR_SUPPORT_RGB
                                                                  | ANSI_COLOR_SUPPORT_ITU_256
                                                                  | ANSI_COLOR_SUPPORT_ITU_RGB
                                                                  | ANSI_COLOR_SUPPORT_ITU_UNDERLINE;

void ansi_transition(std::ostream &, AnsiSupportFlags, const RawAnsi &before, const RawAnsi &after);
void ansi_string(std::ostream &, AnsiSupportFlags flags, const RawAnsi &ansi);

NODISCARD AnsiString ansi_transition(AnsiSupportFlags, const RawAnsi &before, const RawAnsi &after);
NODISCARD AnsiString ansi_string(AnsiSupportFlags flags, const RawAnsi &ansi);

NODISCARD std::string_view to_string_view(AnsiColor16Enum);

// "#000000" to "#FFFFFF" ('#' followed by 6-digit hex)
NODISCARD std::string_view to_hex_color_string_view(AnsiColor16Enum ansi);
NODISCARD Color toColor(AnsiColor16Enum ansi);
NODISCARD int rgbToAnsi256(int r, int g, int b);

NODISCARD size_t ansiCodeLen(std::string_view input);
NODISCARD bool isAnsiColor(std::string_view input);
NODISCARD std::optional<RawAnsi> ansi_parse(RawAnsi ansi, std::string_view input);

template<typename ValidAnsiColorCallback, typename InvalidEscapeCallback, typename NonAnsiCallback>
void foreachAnsi(const std::string_view input,
                 ValidAnsiColorCallback &&validAnsiColor,
                 InvalidEscapeCallback &&invalidEscape,
                 NonAnsiCallback &&nonAnsi)
{
    std::string_view sv = input;
    while (!sv.empty()) {
        const auto esc_pos = sv.find(char_consts::C_ESC);
        if (esc_pos == std::string_view::npos) {
            nonAnsi(sv);
            break;
        }
        if (esc_pos > 0) {
            nonAnsi(sv.substr(0, esc_pos));
            sv.remove_prefix(esc_pos);
        }

        const auto len = ansiCodeLen(sv);
        assert(len > 0);
        const auto ansi = sv.substr(0, len);
        sv.remove_prefix(len);

        if (isAnsiColor(ansi)) {
            validAnsiColor(ansi);
        } else {
            invalidEscape(ansi);
        }
    }
}

namespace mmqt {
enum class NODISCARD TokenTypeEnum { Ansi, Control, Newline, Space, Word };
class NODISCARD AnsiStringToken final
{
public:
    using size_type = decltype(std::declval<QString>().size());
    TokenTypeEnum type = TokenTypeEnum::Ansi; // There is no good default value.

private:
    const QStringView m_text;

public:
    explicit AnsiStringToken(const TokenTypeEnum input_type,
                             const QStringView text,
                             const size_type offset,
                             const size_type length)
        : type{input_type}
        , m_text{text.mid(offset, length)}
    {}
    NODISCARD size_type length() const { return m_text.length(); }

public:
    NODISCARD QChar at(const size_type pos) const
    {
        assert(!m_text.empty());
        assert(isClamped(pos, size_type(0), m_text.length() - 1));
        return m_text.at(pos);
    }
    NODISCARD QChar operator[](const size_type pos) const { return at(pos); }

public:
    NODISCARD auto begin() const { return m_text.begin(); }
    NODISCARD auto end() const { return m_text.end(); }

public:
    NODISCARD QStringView getQStringView() const { return m_text; }
    NODISCARD bool isAnsiCsi() const;
};

struct NODISCARD AnsiTokenizer final
{
    class NODISCARD Iterator final
    {
    public:
        using size_type = AnsiStringToken::size_type;

    private:
        QStringView m_str;

    public:
        explicit Iterator(QStringView str);

        NODISCARD bool operator!=(std::nullptr_t) const { return hasNext(); }
        NODISCARD bool operator!=(const Iterator &other) const = delete;

        /*
         * NOTE: The `operator*()` and `operator++()` paradigm is subtly
         * different from the `while (hasNext()) foo(next());` paradigm.
         *
         * With a range-based for loop,
         *   for (YourType yourVarName : yourRange) {
         *     // your code here
         *   }
         * becomes:
         *   auto _it = std::begin(yourRange); // defaults to yourRange.begin()
         *   auto _end = std::end(yourRange);  // defaults to yourRange.end()
         *   while (_it != _end) {             // calls _it.operator!=(_end)
         *     YourType yourVarName = *_it;    // calls _it.operator*()
         *     // your code here
         *     ++_it;                          // calls _it.operator++()
         *   }
         *
         * The current implementation is poorly optimized for range-based
         * for-loops because it will end up calling `getCurrent()` in both
         * `operator*()` and `operator++()`. This could be fixed by storing
         * an integer offset when it's computed in `operator*()`,
         * and then using that value in `operator++`.
         *
         * It would make iterator copies marginally slower, but it would
         * end up being faster in most cases. If you do that, consider
         * using a boolean value to make it equivalent to std::optional<T>,
         * (use negative values for that purpose).
         */
        NODISCARD AnsiStringToken operator*() { return getCurrent(); }
        ALLOW_DISCARD Iterator &operator++()
        {
            std::ignore = next();
            return *this;
        }

        NODISCARD bool hasNext() const { return !m_str.empty(); }
        NODISCARD AnsiStringToken next();
        NODISCARD AnsiStringToken getCurrent();

    private:
        enum class NODISCARD ResultEnum : uint8_t { KEEPGOING, STOP };

        template<typename Callback>
        NODISCARD size_type skip(Callback &&check) const
        {
            const auto len = m_str.size();
            assert(len > 0);
            const auto start = 0;
            assert(isClamped<qsizetype>(start, 0, len));
            auto it = start + 1;
            for (; it < len; ++it) {
                if (check(m_str[it]) == ResultEnum::STOP) {
                    break;
                }
            }
            assert(isClamped<qsizetype>(it, start, len));
            return it - start;
        }
        NODISCARD size_type skip_ansi() const;
        NODISCARD static bool isControl(const QChar c)
        {
            return ascii::isCntrl(c.toLatin1()) && c != QC_NBSP;
        }
        NODISCARD size_type skip_control() const;
        NODISCARD size_type skip_space() const;
        NODISCARD size_type skip_word() const;
    };

private:
    const QStringView m_sv;

public:
    explicit AnsiTokenizer(const QStringView sv)
        : m_sv{sv}
    {}

    NODISCARD Iterator begin() { return Iterator{m_sv}; }
    NODISCARD auto end() { return nullptr; }
};

NODISCARD bool containsAnsi(QStringView str);
NODISCARD bool containsAnsi(const QString &str);

extern const QRegularExpression weakAnsiRegex;

// Reports any potential ANSI sequence, including invalid sequences.
// Use isAnsiColor(ref) to verify if the value reported is a color.
//
// Callback:
// void(int start, QStringView sv)
//
// NOTE: This version only reports callback(start, length),
// because the intended caller needs the start position,
// but QStringView can't expose the start position.
template<typename Callback>
void foreachAnsi(const QStringView line, Callback &&callback)
{
    const auto len = line.size();
    qsizetype pos = 0;
    while (pos < len) {
        QRegularExpressionMatch m = weakAnsiRegex.match(line, pos);
        if (!m.hasMatch()) {
            break;
        }
        callback(m.capturedStart(), m.captured());
        pos = m.capturedEnd();
    }
}

template<typename Callback>
void foreachAnsi(const QString &line, Callback &&callback)
{
    foreachAnsi(QStringView{line}, std::forward<Callback>(callback));
}

NODISCARD extern bool isAnsiColor(QStringView ansi);
NODISCARD extern bool isAnsiColor(const QString &ansi);

class NODISCARD AnsiColorParser final
{
private:
    struct NODISCARD Erased1 final
    {
        void *const data;
        void (*const callback)(void *data, int);
    } m_callback1;
    struct NODISCARD Erased2 final
    {
        void *const data;
        void (*const callback)(void *data, const AnsiItuColorCodes &codes);
    } m_callback2;
    explicit AnsiColorParser(Erased1 callback1, Erased2 callback2)
        : m_callback1{callback1}
        , m_callback2{callback2}
    {}

    void for_each(QStringView end) const;
    void report(int n) const { m_callback1.callback(m_callback1.data, n); }
    void report_itu(const AnsiItuColorCodes &codes) const
    {
        m_callback2.callback(m_callback2.data, codes);
    }

public:
    // "ESC[m"     -> callback(0)
    // "ESC[0m"    -> callback(0)
    // "ESC[1;33m" -> callback(1); callback(33)
    //
    // NOTE: Empty values other than "ESC[m" are errors.
    // e.g. ESC[;;m -> callback(-1); callback(-1); callback(-1)
    // (Question: Is that still true? Probably not!)
    //
    // NOTE: Values too large to fit in a signed integer
    // (e.g. "ESC[2147483648m" -> callback(-1).
    template<typename Callback, typename Callback2>
    static void for_each_code(const QStringView ansi, Callback &&callback, Callback2 &&callback2)
    {
        const auto erased1 = std::invoke([&callback]() -> Erased1 {
            auto ptr = &callback;
            using ptr_type = decltype(ptr);

            /* use type-erasure to avoid putting implementation in the header */
            const auto type_erased = [](void *data, int n) -> void {
                Callback &cb = *reinterpret_cast<ptr_type>(data);
                cb(n);
            };
            return Erased1{reinterpret_cast<void *>(ptr), type_erased};
        });

        const auto erased2 = std::invoke([&callback2]() -> Erased2 {
            auto ptr2 = &callback2;
            using ptr2_type = decltype(ptr2);

            /* use type-erasure to avoid putting implementation in the header */
            const auto type_erased2 = [](void *data, const AnsiItuColorCodes &codes) -> void {
                Callback2 &cb = *reinterpret_cast<ptr2_type>(data);
                cb(codes);
            };
            return Erased2{reinterpret_cast<void *>(ptr2), type_erased2};
        });

        AnsiColorParser{erased1, erased2}.for_each(ansi);
    }
};

// Input must be a valid ansi color code: ESC[...m,
// where the ... is any number of digits or semicolons.
// Callback:
// void(int csi)
template<typename Callback, typename Callback2>
void ansiForeachColorCode(const QStringView ansi, Callback &&callback, Callback2 &&callback2)
{
    AnsiColorParser::for_each_code(ansi,
                                   std::forward<Callback>(callback),
                                   std::forward<Callback2>(callback2));
}

template<typename Callback, typename Callback2>
void ansiForeachColorCode(const QString &ansi, Callback &&callback, Callback2 &&callback2)
{
    ansiForeachColorCode(QStringView{ansi},
                         std::forward<Callback>(callback),
                         std::forward<Callback2>(callback2));
}

NODISCARD extern std::optional<RawAnsi> parseAnsiColor(RawAnsi, QStringView ansi);
NODISCARD extern std::optional<RawAnsi> parseAnsiColor(RawAnsi, const QString &ansi);

NODISCARD extern bool isValidAnsiColor(QStringView ansi);
NODISCARD extern bool isValidAnsiColor(const QString &ansi);

NODISCARD extern bool isAnsiEraseLine(QStringView ansi);
NODISCARD extern bool isAnsiEraseLine(const QString &ansi);

/**
 * NOTE:
 * 1. Code will assert() if the text does not contain any ansi strings.
 * 2. Input is assumed to start with reset ansi (ESC[0m).
 * 3. All ansi codes will be normalized (e.g. ESC[1mESC[32m -> ESC[1;32m).
 * 4. Ansi will be explicitly reset before every newline, and it will
 *    be restored at the start of the next line.
 * 5. The function assumes it's the entire document, and it always resets
 *    the ansi color at the end, even if there's no newline. Therefore,
 *    the function doesn't return a "current" ansi color.
 * 6. Assumes UNIX-style newlines (LF only, not CRLF).
 */
NODISCARD TextBuffer normalizeAnsi(AnsiSupportFlags supportFlags, QStringView old);
NODISCARD TextBuffer normalizeAnsi(AnsiSupportFlags supportFlags, const QString &str);

NODISCARD QColor toQColor(AnsiColor16Enum ansi);
NODISCARD QColor ansi256toRgb(int ansi);
NODISCARD QString rgbToAnsi256String(const QColor &rgb, AnsiColor16LocationEnum type);
} // namespace mmqt

NODISCARD bool containsAnsi(std::string_view sv);
void strip_ansi(std::ostream &os, std::string_view sv);
NODISCARD std::string strip_ansi(std::string);

namespace test {
extern void testAnsiTextUtils();
} // namespace test
