#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/hash.h"
#include "../global/utils.h"

#include <cassert>
#include <optional>

#include <glm/glm.hpp>

struct NODISCARD Coordinate2i final
{
public:
    int x = 0;
    int y = 0;

public:
    Coordinate2i() = default;
    explicit Coordinate2i(const int x_, const int y_)
        : x{x_}
        , y{y_}
    {}
    explicit Coordinate2i(const glm::ivec2 &rhs)
        : x{rhs.x}
        , y{rhs.y}
    {}

    NODISCARD Coordinate2i operator+(const Coordinate2i &rhs) const
    {
        return Coordinate2i{x + rhs.x, y + rhs.y};
    }
    NODISCARD Coordinate2i operator-(const Coordinate2i &rhs) const
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

    NODISCARD glm::ivec2 to_ivec2() const { return glm::ivec2{x, y}; }
};

struct NODISCARD Coordinate2f final
{
public:
    float x = 0.f;
    float y = 0.f;

public:
    Coordinate2f() = default;
    explicit Coordinate2f(const float x_, const float y_)
        : x{x_}
        , y{y_}
    {}

    NODISCARD Coordinate2i truncate() const;

    NODISCARD Coordinate2f operator-(const Coordinate2f &rhs) const
    {
        return Coordinate2f{this->x - rhs.x, this->y - rhs.y};
    }
    NODISCARD Coordinate2f operator*(const float f) const { return Coordinate2f{f * x, f * y}; }
    NODISCARD Coordinate2f operator/(float f) const;
    NODISCARD glm::vec2 to_vec2() const { return glm::vec2{x, y}; }
};

// Basis vectors: ENU (x=east, y=north, z=up).
// This is the standard RIGHT-HANDED coordinate system.
class NODISCARD Coordinate final
{
public:
    int x = 0;
    int y = 0;
    int z = 0;

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
    NODISCARD bool isNull() const { return (x == 0 && y == 0 && z == 0); }

public:
    NODISCARD glm::ivec2 to_ivec2() const { return glm::ivec2{x, y}; }
    NODISCARD glm::ivec3 to_ivec3() const { return glm::ivec3{x, y, z}; }

public:
    NODISCARD glm::vec2 to_vec2() const { return glm::vec2{x, y}; }
    NODISCARD glm::vec3 to_vec3() const { return glm::vec3{x, y, z}; }

public:
    NODISCARD bool operator==(const Coordinate &other) const;
    NODISCARD bool operator!=(const Coordinate &other) const;
    void operator+=(const Coordinate &other);
    void operator-=(const Coordinate &other);
    NODISCARD Coordinate operator+(const Coordinate &other) const;
    NODISCARD Coordinate operator-(const Coordinate &other) const;
    NODISCARD Coordinate operator*(int scalar) const;
    NODISCARD Coordinate operator/(int scalar) const;

    NODISCARD int distance(const Coordinate &other) const;
    void clear();

public:
    NODISCARD static Coordinate toCoordinate(const glm::ivec3 &c)
    {
        return Coordinate(c.x, c.y, c.z);
    }

    NODISCARD static Coordinate min(const Coordinate &a, const Coordinate &b)
    {
        return toCoordinate(glm::min(a.to_ivec3(), b.to_ivec3()));
    }

    NODISCARD static Coordinate max(const Coordinate &a, const Coordinate &b)
    {
        return toCoordinate(glm::max(a.to_ivec3(), b.to_ivec3()));
    }

public:
    bool operator<(const Coordinate &rhs) const;
    bool operator>(const Coordinate &rhs) const;
    bool operator<=(const Coordinate &rhs) const;
    bool operator>=(const Coordinate &rhs) const;
};

template<>
struct std::hash<Coordinate>
{
    std::size_t operator()(const Coordinate &id) const noexcept
    {
        const auto hx = numeric_hash(id.x);
        const auto hy = numeric_hash(id.y);
        const auto hz = numeric_hash(id.z);
        return hx ^ utils::rotate_bits64<21>(hy) ^ utils::rotate_bits64<42>(hz);
    }
};

struct NODISCARD Bounds final
{
    Coordinate min;
    Coordinate max;

    Bounds() = default;

    // NOTE: These are supposed to be min and max,
    // but we'll accept a strange mixture.
    Bounds(const Coordinate &a, const Coordinate &b);

private:
    NODISCARD static inline bool isBounded(const int x, const int lo, const int hi)
    {
        return lo <= x && x <= hi;
    }

public:
    NODISCARD bool contains(const Coordinate &coord) const
    {
        return isBounded(coord.x, min.x, max.x)     //
               && isBounded(coord.y, min.y, max.y)  //
               && isBounded(coord.z, min.z, max.z); //
    }

    void insert(const Coordinate &c);
    NODISCARD bool operator==(const Bounds &rhs) const { return min == rhs.min && max == rhs.max; }
    NODISCARD bool operator!=(const Bounds &rhs) const { return !(rhs == *this); }
};

template<>
struct std::hash<Bounds>
{
    size_t operator()(const Bounds &b) const
    {
        const std::hash<Coordinate> &hc = std::hash<Coordinate>();
        return hc(b.min) ^ utils::rotate_bits64<32>(hc(b.max));
    }
};

struct NODISCARD OptBounds final
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
    NODISCARD static OptBounds fromCenterRadius(const Coordinate &center, const Coordinate &radius)
    {
        assert(radius.x >= 0);
        assert(radius.y >= 0);
        assert(radius.z >= 0);
        return OptBounds{center - radius, center + radius};
    }

public:
    NODISCARD bool isRestricted() const { return m_bounds.has_value(); }
    NODISCARD const Bounds &getBounds() const { return m_bounds.value(); }
    void reset() { m_bounds.reset(); }

public:
    NODISCARD bool contains(const Coordinate &coord) const
    {
        return !isRestricted() || getBounds().contains(coord);
    }
};
