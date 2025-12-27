#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "RuleOf5.h"
#include "macros.h"

#include <ostream>
#include <string_view>

#include <glm/glm.hpp>

class AnsiOstream;
class QColor;
class QString;

struct NODISCARD Color final
{
private:
    static constexpr const uint32_t TRANSPARENT_BLACK = 0u;
    static constexpr const uint32_t OPAQUE_WHITE = ~TRANSPARENT_BLACK;
    static constexpr const uint32_t MASK_RGB = 0x00ffffffu;
    static constexpr const uint32_t MASK_ALPHA = ~MASK_RGB;
    static constexpr const int SHIFT_ALPHA = 24;
    static_assert(MASK_RGB == ((1u << SHIFT_ALPHA) - 1u));
    static_assert(MASK_ALPHA == (255u << SHIFT_ALPHA));
    static_assert(OPAQUE_WHITE == (MASK_RGB | MASK_ALPHA));
    static_assert(TRANSPARENT_BLACK == (MASK_RGB & MASK_ALPHA));

private:
    uint32_t m_color = OPAQUE_WHITE;

public:
    Color() = default;
    ~Color() = default;

public:
    DEFAULT_CTORS_AND_ASSIGN_OPS(Color);

public:
    explicit Color(const glm::vec4 &color)
        : m_color{glm::packUnorm4x8(color)}
    {}

public:
    explicit Color(uint32_t, float) = delete;
    explicit Color(Color rgb, float alpha);

public:
    explicit Color(int r, int g, int b);
    explicit Color(int r, int g, int b, int a);
    explicit Color(float r, float g, float b);
    explicit Color(float r, float g, float b, float a);
    explicit Color(double r, double g, double b) = delete;

public:
    explicit Color(const QColor &qcolor);
    explicit Color(const QColor &rgb, float alpha);

    NODISCARD static Color fromRGB(uint32_t rgb);

public:
    NODISCARD QColor getQColor() const;
    NODISCARD uint32_t getRGB() const { return m_color & MASK_RGB; }
    NODISCARD uint32_t getAlpha() const { return (m_color & MASK_ALPHA) >> SHIFT_ALPHA; }
    NODISCARD uint32_t getRGBA() const { return m_color; }
    NODISCARD uint32_t getUint32() const { return m_color; }
    NODISCARD glm::vec4 getVec4() const { return glm::unpackUnorm4x8(m_color); }

public:
    NODISCARD uint32_t getRed() const;   // 0..255
    NODISCARD uint32_t getGreen() const; // 0..255
    NODISCARD uint32_t getBlue() const;  // 0..255

public:
    NODISCARD bool operator==(const Color &rhs) const { return m_color == rhs.m_color; }
    NODISCARD bool operator!=(const Color &rhs) const { return !operator==(rhs); }
    NODISCARD explicit operator glm::vec4() const { return getVec4(); }
    NODISCARD explicit operator QColor() const;

public:
    NODISCARD Color withAlpha(float alpha) const;
    NODISCARD bool isTransparent() const { return (m_color & MASK_ALPHA) == 0; }

public:
    NODISCARD static Color fromHex(std::string_view sv);
    NODISCARD std::string toHex() const;
    std::ostream &toHex(std::ostream &os) const;
    friend std::ostream &operator<<(std::ostream &os, const Color c) { return c.toHex(os); }
    friend AnsiOstream &operator<<(AnsiOstream &os, Color c);

public:
    // note: this is not done in linear color space.
    NODISCARD static Color multiplyAsVec4(const Color a, const Color b);
};

static_assert(sizeof(Color) == sizeof(uint32_t));
static_assert(alignof(Color) == alignof(uint32_t));

#define XFOREACH_COLOR(X) \
    X(black, 000000) \
    X(blue, 0000FF) \
    X(cyan, 00FFFF) \
    /* X(darkOrange, FF8C00) */ \
    X(darkOrange1, FF7F00) \
    X(gray70, B3B3B3) \
    X(gray75, C0C0C0) \
    X(green, 00FF00) \
    X(magenta, FF00FF) \
    /* X(orange, FFA500) */ \
    /* X(orangeRed, FF4500) */ \
    X(red, FF0000) \
    X(red20, 330000) \
    X(webGray, 808080) \
    X(white, FFFFFF) \
    X(yellow, FFFF00)

namespace Colors {
#define X_DECL(name, hex) extern const Color name;
XFOREACH_COLOR(X_DECL)
#undef X_DECL
} // namespace Colors

NODISCARD Color textColor(Color color);

namespace mmqt {
NODISCARD QColor textColor(QColor color);
NODISCARD Color toColor(const QString &);
} // namespace mmqt

namespace test {
extern void testColor();
} // namespace test
