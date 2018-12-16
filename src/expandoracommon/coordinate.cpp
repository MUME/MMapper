/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include "coordinate.h"

#include <cmath>
#include <cstdlib>
#include <stdexcept>

#include "../global/utils.h"

vec2i vec2f::round() const
{
    return vec2i{utils::round_ftoi(x), utils::round_ftoi(y)};
}

vec2f vec2f::operator/(const float f) const
{
    if (f == 0.0f)
        throw std::runtime_error("division by zero");

    if (std::isnan(f))
        throw std::runtime_error("division by NaN");

    return operator*(1.0f / f);
}

bool Coordinate::operator==(const Coordinate &other) const
{
    return (other.x == x && other.y == y && other.z == z);
}

bool Coordinate::operator!=(const Coordinate &other) const
{
    return (other.x != x || other.y != y || other.z != z);
}

int Coordinate::distance(const Coordinate &other) const
{
    return std::abs(x - other.x) + std::abs(y - other.y) + std::abs(z - other.z);
}

void Coordinate::clear()
{
    x = 0;
    y = 0;
    z = 0;
}

void Coordinate::operator+=(const Coordinate &other)
{
    x += other.x;
    y += other.y;
    z += other.z;
}

void Coordinate::operator-=(const Coordinate &other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
}

Coordinate Coordinate::operator+(const Coordinate &other) const
{
    Coordinate ret = *this;
    ret += other;
    return ret;
}

Coordinate Coordinate::operator-(const Coordinate &other) const
{
    Coordinate ret = *this;
    ret -= other;
    return ret;
}

Coordinate Coordinate ::operator*(const int scalar) const
{
    Coordinate ret = *this;
    ret.x *= scalar;
    ret.y *= scalar;
    ret.z *= scalar;
    return ret;
}
