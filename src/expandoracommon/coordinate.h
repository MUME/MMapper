#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <cassert>
#include <glm/glm.hpp>
#include <optional>

#include "../global/utils.h"

struct Coordinate2i final
{
    int x = 0;
    int y = 0;
    Coordinate2i() = default;
    explicit Coordinate2i(const int x, const int y)
        : x{x}
        , y{y}
    {}
    explicit Coordinate2i(const glm::ivec2 &rhs)
        : x{rhs.x}
        , y{rhs.y}
    {}

    Coordinate2i operator+(const Coordinate2i &rhs) const
    {
        return Coordinate2i{x + rhs.x, y + rhs.y};
    }
    Coordinate2i operator-(const Coordinate2i &rhs) const
    {
        return Coordinate2i{x - rhs.x, y - rhs.y};
    }
    Coordinate2i &operator+=(const glm::ivec2 &rhs)
    {
        auto &self = *this;
        self = self + Coordinate2i{rhs};
        return *this;
    }
    Coordinate2i &operator-=(const glm::ivec2 &rhs)
    {
        auto &self = *this;
        self = self - Coordinate2i{rhs};
        return *this;
    }

    glm::ivec2 to_ivec2() const { return glm::ivec2{x, y}; }
};

struct Coordinate2f final
{
    float x = 0.f;
    float y = 0.f;
    Coordinate2f() = default;
    explicit Coordinate2f(const float x, const float y)
        : x{x}
        , y{y}
    {}

    Coordinate2i truncate() const;

    Coordinate2f operator-(const Coordinate2f &rhs) const
    {
        return Coordinate2f{this->x - rhs.x, this->y - rhs.y};
    }
    Coordinate2f operator*(const float f) const { return Coordinate2f{f * x, f * y}; }
    Coordinate2f operator/(float f) const;
    glm::vec2 to_vec2() const { return glm::vec2{x, y}; }
};

// Basis vectors: ENU (x=east, y=north, z=up).
// This is the standard RIGHT-HANDED coordinate system.
class Coordinate final
{
public:
    bool operator==(const Coordinate &other) const;
    bool operator!=(const Coordinate &other) const;
    void operator+=(const Coordinate &other);
    void operator-=(const Coordinate &other);
    Coordinate operator+(const Coordinate &other) const;
    Coordinate operator-(const Coordinate &other) const;
    Coordinate operator*(int scalar) const;
    Coordinate operator/(int scalar) const;

    int distance(const Coordinate &other) const;
    void clear();

public:
    Coordinate() = default;
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

public:
    bool isNull() const { return (x == 0 && y == 0 && z == 0); }

public:
    glm::ivec2 to_ivec2() const { return glm::ivec2{x, y}; }
    glm::ivec3 to_ivec3() const { return glm::ivec3{x, y, z}; }

public:
    glm::vec2 to_vec2() const { return glm::vec2{x, y}; }
    glm::vec3 to_vec3() const { return glm::vec3{x, y, z}; }

public:
    int x = 0;
    int y = 0;
    int z = 0;
};

struct Bounds final
{
    Coordinate min;
    Coordinate max;

    Bounds() = default;
    Bounds(const Coordinate &min, const Coordinate &max)
        : min{min}
        , max{max}
    {}

private:
    static inline bool isBounded(const int x, const int lo, const int hi)
    {
        return lo <= x && x <= hi;
    }

public:
    bool contains(const Coordinate &coord) const
    {
        return isBounded(coord.x, min.x, max.x)     //
               && isBounded(coord.y, min.y, max.y)  //
               && isBounded(coord.z, min.z, max.z); //
    }
};

struct OptBounds final
{
private:
    std::optional<Bounds> m_bounds;

public:
    OptBounds() = default;
    OptBounds(const Coordinate &min, const Coordinate &max)
        : m_bounds{Bounds{min, max}}
    {
        assert(min.x <= max.x);
        assert(min.y <= max.y);
        assert(min.z <= max.z);
    }

public:
    static OptBounds fromCenterRadius(const Coordinate &center, const Coordinate &radius)
    {
        assert(radius.x >= 0);
        assert(radius.y >= 0);
        assert(radius.z >= 0);
        return OptBounds{center - radius, center + radius};
    }

public:
    bool isRestricted() const { return m_bounds.has_value(); }
    const Bounds &getBounds() const { return m_bounds.value(); }
    void reset() { m_bounds.reset(); }

public:
    bool contains(const Coordinate &coord) const
    {
        return !isRestricted() || getBounds().contains(coord);
    }
};
