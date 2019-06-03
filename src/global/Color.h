#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#ifndef MMAPPER_COLOR_H
#define MMAPPER_COLOR_H

#include <cmath>
#include <QColor>

enum class AnsiColorTable {
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

static inline QColor ansiColor(const AnsiColorTable i)
{
    static QColor black("#2e3436");
    static QColor BLACK("#555753");
    static QColor red("#cc0000");
    static QColor RED("#ef2929");
    static QColor green("#4e9a06");
    static QColor GREEN("#8ae234");
    static QColor yellow("#c4a000");
    static QColor YELLOW("#fce94f");
    static QColor blue("#3465a4");
    static QColor BLUE("#729fcf");
    static QColor magenta("#75507b");
    static QColor MAGENTA("#ad7fa8");
    static QColor cyan("#06989a");
    static QColor CYAN("#34e2e2");
    static QColor white("#d3d7cf");
    static QColor WHITE("#eeeeec");

    switch (i) {
    case AnsiColorTable::black:
        return black;
    case AnsiColorTable::red:
        return red;
    case AnsiColorTable::green:
        return green;
    case AnsiColorTable::yellow:
        return yellow;
    case AnsiColorTable::blue:
        return blue;
    case AnsiColorTable::magenta:
        return magenta;
    case AnsiColorTable::cyan:
        return cyan;
    case AnsiColorTable::white:
        return white;
    case AnsiColorTable::BLACK:
        return BLACK;
    case AnsiColorTable::RED:
        return RED;
    case AnsiColorTable::GREEN:
        return GREEN;
    case AnsiColorTable::YELLOW:
        return YELLOW;
    case AnsiColorTable::BLUE:
        return BLUE;
    case AnsiColorTable::MAGENTA:
        return MAGENTA;
    case AnsiColorTable::CYAN:
        return CYAN;
    case AnsiColorTable::WHITE:
        return WHITE;
    default:
        return white;
    }
}

static inline QColor textColor(const QColor color)
{
    // Dynamically select text color according to the background color
    // https://gist.github.com/jlong/f06f5843104ee10006fe
    constexpr const auto redMagic = 241;
    constexpr const auto greenMagic = 691;
    constexpr const auto blueMagic = 68;
    constexpr const auto divisor = redMagic + greenMagic + blueMagic;

    // Calculate a brightness value in 3d color space between 0 and 255
    const auto brightness = sqrt(((pow(color.red(), 2) * redMagic)
                                  + (pow(color.green(), 2) * greenMagic)
                                  + (pow(color.blue(), 2) * blueMagic))
                                 / divisor);
    const auto percentage = 100 * brightness / 255;
    return percentage < 65 ? QColor(Qt::white) : QColor(Qt::black);
}

static inline QColor ansi256toRgb(const int ansi)
{
    // 232-255: grayscale from black to white in 24 steps
    if (ansi >= 232) {
        const auto c = (ansi - 232) * 10 + 8;
        return QColor(c, c, c);
    }

    // 16-231: 6 x 6 x 6 cube (216 colors): 16 + 36 * r + 6 * g + b
    if (ansi >= 16) {
        const auto colors = ansi - 16;
        const auto remainder = colors % 36;
        const auto r = static_cast<int>(floor(colors / 36) / 5 * 255);
        const auto g = static_cast<int>(floor(remainder / 6) / 5 * 255);
        const auto b = (remainder % 6) / 5 * 255;
        return QColor(r, g, b);
    }

    // 8-15: highlighted colors
    if (ansi >= 8) {
        return ansiColor(static_cast<AnsiColorTable>(ansi - 8 + 60));
    }

    // 0-7: normal colors
    return ansiColor(static_cast<AnsiColorTable>(ansi));
}

static inline int rgbToAnsi256(const int r, const int g, const int b)
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

static inline QString rgbToAnsi256String(const QColor rgb, bool foreground = true)
{
    return QString("[%1;5;%2m")
        .arg(foreground ? "38" : QString("%1;48").arg(textColor(rgb) == Qt::white ? "37" : "30"))
        .arg(rgbToAnsi256(rgb.red(), rgb.green(), rgb.blue()));
}

#endif // MMAPPER_COLOR_H
