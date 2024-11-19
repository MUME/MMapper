#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "macros.h"

#include <cmath>

#include <QColor>

enum class NODISCARD AnsiColorTableEnum {
    black = 0,
    red,
    green,
    yellow,
    blue,
    magenta,
    cyan,
    white,
    BLACK = 60,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE
};

namespace mmqt {

NODISCARD static inline QColor ansiColor(const AnsiColorTableEnum i)
{
    static const QColor black("#2e3436");
    static const QColor BLACK("#555753");
    static const QColor red("#cc0000");
    static const QColor RED("#ef2929");
    static const QColor green("#4e9a06");
    static const QColor GREEN("#8ae234");
    static const QColor yellow("#c4a000");
    static const QColor YELLOW("#fce94f");
    static const QColor blue("#3465a4");
    static const QColor BLUE("#729fcf");
    static const QColor magenta("#75507b");
    static const QColor MAGENTA("#ad7fa8");
    static const QColor cyan("#06989a");
    static const QColor CYAN("#34e2e2");
    static const QColor white("#d3d7cf");
    static const QColor WHITE("#eeeeec");

    switch (i) {
    case AnsiColorTableEnum::black:
        return black;
    case AnsiColorTableEnum::red:
        return red;
    case AnsiColorTableEnum::green:
        return green;
    case AnsiColorTableEnum::yellow:
        return yellow;
    case AnsiColorTableEnum::blue:
        return blue;
    case AnsiColorTableEnum::magenta:
        return magenta;
    case AnsiColorTableEnum::cyan:
        return cyan;
    case AnsiColorTableEnum::white:
        return white;
    case AnsiColorTableEnum::BLACK:
        return BLACK;
    case AnsiColorTableEnum::RED:
        return RED;
    case AnsiColorTableEnum::GREEN:
        return GREEN;
    case AnsiColorTableEnum::YELLOW:
        return YELLOW;
    case AnsiColorTableEnum::BLUE:
        return BLUE;
    case AnsiColorTableEnum::MAGENTA:
        return MAGENTA;
    case AnsiColorTableEnum::CYAN:
        return CYAN;
    case AnsiColorTableEnum::WHITE:
        return WHITE;
    default:
        return white;
    }
}

NODISCARD static inline QColor textColor(const QColor color)
{
    // Dynamically select text color according to the background color
    // http://www.nbdtech.com/Blog/archive/2008/04/27/Calculating-the-Perceived-Brightness-of-a-Color.aspx
    constexpr const auto redMagic = 241;
    constexpr const auto greenMagic = 691;
    constexpr const auto blueMagic = 68;
    constexpr const auto divisor = redMagic + greenMagic + blueMagic;

    // Calculate a brightness value in 3d color space between 0 and 255
    const auto brightness = std::sqrt(((std::pow(color.red(), 2) * redMagic)
                                       + (std::pow(color.green(), 2) * greenMagic)
                                       + (std::pow(color.blue(), 2) * blueMagic))
                                      / divisor);
    const auto percentage = 100 * brightness / 255;
    return percentage < 50 ? QColor(Qt::white) : QColor(Qt::black);
}

NODISCARD static inline QColor ansi256toRgb(const int ansi)
{
    // 232-255: grayscale from black to white in 24 steps
    if (ansi >= 232) {
        const auto c = (ansi - 232) * 10 + 8;
        return QColor(c, c, c);
    }

    // 16-231: 6 x 6 x 6 cube (216 colors): 16 + 36 * r + 6 * g + b
    if (ansi >= 16) {
        const int colors = ansi - 16;
        const int remainder = colors % 36;
        const auto r = (colors / 36) * 255 / 5;
        const auto g = (remainder / 6) * 255 / 5;
        const auto b = (remainder % 6) * 255 / 5;
        return QColor(r, g, b);
    }

    // 8-15: highlighted colors
    if (ansi >= 8) {
        return ansiColor(static_cast<AnsiColorTableEnum>(ansi - 8 + 60));
    }

    // 0-7: normal colors
    return ansiColor(static_cast<AnsiColorTableEnum>(ansi));
}

} // namespace mmqt

NODISCARD static inline int rgbToAnsi256(const int r, const int g, const int b)
{
    // https://stackoverflow.com/questions/15682537/ansi-color-specific-rgb-sequence-bash
    // we use the extended greyscale palette here, with the exception of
    // black and white. normal palette only has 4 greyscale shades.
    if (r == g && g == b) {
        if (r < 8) {
            return 16;
        }

        if (r > 248) {
            return 231;
        }

        return static_cast<int>(round((static_cast<double>(r - 8) / 247) * 24)) + 232;
    }
    const int red = 36 * static_cast<int>(round(static_cast<double>(r) / 255 * 5));
    const int green = 6 * static_cast<int>(round(static_cast<double>(g) / 255 * 5));
    const int blue = static_cast<int>(round(static_cast<double>(b) / 255 * 5));
    return 16 + red + green + blue;
}

namespace mmqt {

NODISCARD static inline QString rgbToAnsi256String(const QColor rgb, bool foreground = true)
{
    return QString("[%1;5;%2m")
        .arg(foreground ? "38" : QString("%1;48").arg(textColor(rgb) == Qt::white ? "37" : "30"))
        .arg(rgbToAnsi256(rgb.red(), rgb.green(), rgb.blue()));
}

} // namespace mmqt
