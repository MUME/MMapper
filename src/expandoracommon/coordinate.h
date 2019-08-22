#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

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
