// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Color.h"

#include "utils.h"

#include <iomanip>
#include <sstream>

#include <QColor>

static constexpr const int SHIFT_r = 0;
static constexpr const int SHIFT_g = 8;
static constexpr const int SHIFT_b = 16;
static constexpr const int SHIFT_a = 24;

Color::Color(const Color &rgb, const float alpha)
    : Color{rgb.withAlpha(alpha)}
{}

Color::Color(const int r, const int g, const int b)
    : Color{r, g, b, 255}
{
    static_assert(SHIFT_a == Color::SHIFT_ALPHA);
}

NODISCARD static constexpr uint32_t clamp_0_255(int n)
{
    return static_cast<uint32_t>(std::clamp(n, 0, 255));
}

#define CLAMP_SHIFT(x) (clamp_0_255(x) << (SHIFT_##x))
Color::Color(const int r, const int g, const int b, const int a)
    : m_color{CLAMP_SHIFT(r) | CLAMP_SHIFT(g) | CLAMP_SHIFT(b) | CLAMP_SHIFT(a)}
{
    assert(isClamped(r, 0, 255));
    assert(isClamped(g, 0, 255));
    assert(isClamped(b, 0, 255));
    assert(isClamped(a, 0, 255));
}
#undef CLAMP_SHIFT

Color::Color(const float r, const float g, const float b)
    : Color{r, g, b, 1.f}
{}

Color::Color(const float r, const float g, const float b, const float a)
    : Color{glm::vec4(r, g, b, a)}
{
    assert(isClamped(r, 0.f, 1.f));
    assert(isClamped(g, 0.f, 1.f));
    assert(isClamped(b, 0.f, 1.f));
    assert(isClamped(a, 0.f, 1.f));
}

Color::Color(const QColor &qcolor)
    : Color{qcolor.isValid() ? Color{qcolor.red(), qcolor.green(), qcolor.blue(), qcolor.alpha()}
                             : Color{}}
{}

Color::Color(const QColor &rgb, const float alpha)
    : Color{Color{rgb}.withAlpha(alpha)}

{}

uint32_t Color::getRed() const
{
    return (m_color >> SHIFT_r) & 0xFFu;
}

uint32_t Color::getGreen() const
{
    return (m_color >> SHIFT_g) & 0xFFu;
}

uint32_t Color::getBlue() const
{
    return (m_color >> SHIFT_b) & 0xFFu;
}

QColor Color::getQColor() const
{
    return QColor(static_cast<int>(getRed()),    //
                  static_cast<int>(getGreen()),  //
                  static_cast<int>(getBlue()),   //
                  static_cast<int>(getAlpha())); //
}

Color::operator QColor() const
{
    return getQColor();
}

Color Color::withAlpha(const float alpha) const
{
    Color copy = *this;
    copy.m_color &= MASK_RGB;
    copy.m_color |= (Color{0.f, 0.f, 0.f, alpha}.m_color & MASK_ALPHA);
    return copy;
}

NODISCARD static int hexval(const char c)
{
    if (isClamped(c, '0', '9'))
        return c - '0';
    else if (isClamped(c, 'a', 'f'))
        return 10 + (c - 'a');
    else if (isClamped(c, 'A', 'F'))
        return 10 + (c - 'A');

    assert(false);
    return 0;
}

NODISCARD static int decode(const std::string_view sv)
{
    assert(sv.size() == 2);
    return (hexval(sv[0]) << 4) | hexval(sv[1]);
}

Color Color::fromHex(const std::string_view sv)
{
    assert(sv.size() == 6);
    const int r = decode(sv.substr(0, 2));
    const int g = decode(sv.substr(2, 2));
    const int b = decode(sv.substr(4, 2));
    return Color(r, g, b);
}

std::string Color::toHex() const
{
    std::ostringstream oss;
    toHex(oss);
    return oss.str();
}

std::ostream &Color::toHex(std::ostream &os) const
{
    os << '#';

    auto hex2 = [&os](uint32_t val) {
        os << std::hex << std::setw(2) << std::setfill('0') << (val & 0xffu) << std::dec;
    };

    uint32_t rgb = getRGB();
    hex2(rgb >> SHIFT_r);
    hex2(rgb >> SHIFT_g);
    hex2(rgb >> SHIFT_b);

    auto alpha = getAlpha();
    if (alpha != 255) {
        os << " (with alpha " << alpha << "/255)";
    }

    return os;
}

Color Color::fromRGB(const uint32_t rgb)
{
    Color c;
    c.m_color = (rgb & MASK_RGB) | MASK_ALPHA;
    return c;
}

namespace Colors {

#define DECL(name, hex) const Color name = Color::fromHex(#hex);
XFOREACH_COLOR(DECL)
#undef DECL

} // namespace Colors

static const int color_self_test = []() -> int {
    const Color redf{1.f, 0.f, 0.f};
    const Color greenf{0.f, 1.f, 0.f};
    const Color bluef{0.f, 0.f, 1.f};

    const Color redi{255, 0, 0};
    const Color greeni{0, 255, 0};
    const Color bluei{0, 0, 255};

    const Color white(Qt::white);
    const Color red(Qt::red);
    const Color green(Qt::green);
    const Color blue(Qt::blue);

    assert(white == Colors::white);
    assert(red == Colors::red);
    assert(green == Colors::green);
    assert(blue == Colors::blue);

    assert(white == Color{});
    assert(red == redf);
    assert(green == greenf);
    assert(blue == bluef);

    assert(red == redi);
    assert(green == greeni);
    assert(blue == bluei);

    const glm::vec4 &redVec = red.getVec4();
    const glm::vec4 &greenVec = green.getVec4();
    const glm::vec4 &blueVec = blue.getVec4();

    assert(utils::equals(redVec.r, 1.f));
    assert(utils::equals(greenVec.g, 1.f));
    assert(utils::equals(blueVec.b, 1.f));

    assert(red.getRGB() == 255);
    assert(green.getRGB() == (255 << 8));
    assert(blue.getRGB() == (255 << 16));

    assert(red.getRGBA() == (255u | (255u << SHIFT_a)));
    return 42;
}();
