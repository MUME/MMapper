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

#include <cmath>
#include <QColor>

#ifndef MMAPPER_COLOR_H
#define MMAPPER_COLOR_H

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

#endif // MMAPPER_COLOR_H
