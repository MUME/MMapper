#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef COORDINATE
#define COORDINATE

struct Coordinate2i final
{
    int x = 0;
    int y = 0;
    explicit Coordinate2i() = default;
    explicit Coordinate2i(const int x, const int y)
        : x{x}
        , y{y}
    {}

    Coordinate2i operator-(const Coordinate2i &rhs) const
    {
        return Coordinate2i{x - rhs.x, y - rhs.y};
    }
};

struct Coordinate2f final
{
    float x = 0.0f;
    float y = 0.0f;
    explicit Coordinate2f() = default;
    explicit Coordinate2f(const float x, const float y)
        : x{x}
        , y{y}
    {}

    Coordinate2i round() const;

    Coordinate2f operator-(const Coordinate2f &rhs) const
    {
        return Coordinate2f{this->x - rhs.x, this->y - rhs.y};
    }
    Coordinate2f operator*(const float f) const { return Coordinate2f{f * x, f * y}; }
    Coordinate2f operator/(const float f) const;
};

class Coordinate final
{
public:
    bool operator==(const Coordinate &other) const;
    bool operator!=(const Coordinate &other) const;
    void operator+=(const Coordinate &other);
    void operator-=(const Coordinate &other);
    Coordinate operator+(const Coordinate &other) const;
    Coordinate operator-(const Coordinate &other) const;
    Coordinate operator*(const int scalar) const;

    int distance(const Coordinate &other) const;
    void clear();

public:
    explicit Coordinate() = default;
    explicit Coordinate(const Coordinate2i &in_xy, const int in_z)
        : x(in_xy.x)
        , y(in_xy.y)
        , z{in_z}
    {}
    explicit Coordinate(const int in_x, const int in_y, const int in_z)
        : x(in_x)
        , y(in_y)
        , z(in_z)
    {}
    bool isNull() const { return (x == 0 && y == 0 && z == 0); }

    int x = 0;
    int y = 0;
    int z = 0;
};

#endif
