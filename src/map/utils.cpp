// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "utils.h"

#include "../global/utils.h"
#include "../mapfrontend/AbstractRoomVisitor.h"
#include "coordinate.h"
#include "room.h"

#include <map>
#include <memory>
#include <utility>

struct NODISCARD CoordinateMinMax final
{
    Coordinate min;
    Coordinate max;

    explicit CoordinateMinMax(const Coordinate &min_, const Coordinate &max_)
        : min{min_}
        , max{max_}
    {
        assert(isValid());
    }

    NODISCARD bool isValid() { return glm::all(glm::lessThanEqual(min.to_vec3(), max.to_vec3())); }

    NODISCARD CoordinateMinMax expandCopy(const Coordinate &radius) const
    {
        auto copy = *this;
        copy.min -= radius;
        copy.max += radius;
        assert(copy.isValid());
        return copy;
    }
};

class Map::MapOrderedTree final
{
private:
    // REVISIT: consider using something more efficient
    std::map<int, std::map<int, std::map<int, Room *>>> map{};

public:
    MapOrderedTree() = default;
    ~MapOrderedTree();

    void clear() { map.clear(); }

    void getRooms(AbstractRoomVisitor &stream) const
    {
        for (const auto &z : map) {
            for (const auto &y : z.second) {
                for (const auto &x : y.second) {
                    stream.visit(x.second);
                }
            }
        }
    }

    void getRooms(AbstractRoomVisitor &stream, const Coordinate &min, const Coordinate &max) const
    {
        const auto range = CoordinateMinMax(min, max).expandCopy(Coordinate{1, 1, 1});

        const auto zUpper = map.lower_bound(range.max.z);
        for (auto z = map.upper_bound(range.min.z); z != zUpper; ++z) {
            const auto &ymap = (*z).second;
            const auto yUpper = ymap.lower_bound(range.max.y);
            for (auto y = ymap.upper_bound(range.min.y); y != yUpper; ++y) {
                const auto &xmap = (*y).second;
                const auto xUpper = xmap.lower_bound(range.max.x);
                for (auto x = xmap.upper_bound(range.min.x); x != xUpper; ++x) {
                    stream.visit(x->second);
                }
            }
        }
    }

    /**
     * doesn't modify c
     */
    bool defined(const Coordinate &c) const
    {
        const auto &z = map.find(c.z);
        if (z != map.end()) {
            auto &ySeg = (*z).second;
            const auto &y = ySeg.find(c.y);
            if (y != ySeg.end()) {
                const auto &xSeg = (*y).second;
                if (xSeg.find(c.x) != xSeg.end()) {
                    return true;
                }
            }
        }
        return false;
    }

    Room *get(const Coordinate &c) const
    { // map<K,V>::operator[] is not const!
        //    if (!defined(c)) {
        //        return nullptr;
        //    }
        //    return m_map[c.z][c.y][c.x];

        const auto &zmap = map;
        const auto &z = zmap.find(c.z);
        if (z == zmap.end())
            return nullptr;

        const auto &ymap = z->second;
        const auto &y = ymap.find(c.y);
        if (y == ymap.end())
            return nullptr;

        const auto &xmap = y->second;
        const auto &x = xmap.find(c.x);
        if (x == xmap.end())
            return nullptr;

        return x->second;
    }

    void remove(const Coordinate &c) { map[c.z][c.y].erase(c.x); }

    /**
     * doesn't modify c
     */
    void set(const Coordinate &c, Room *room) { map[c.z][c.y][c.x] = room; }
};

Map::MapOrderedTree::~MapOrderedTree() = default;

Map::Map()
    : m_pimpl{std::make_unique<MapOrderedTree>()}
{}

Map::~Map() = default;

Room *Map::get(const Coordinate &c) const
{
    return m_pimpl->get(c);
}

void Map::remove(const Coordinate &c)
{
    return m_pimpl->remove(c);
}

void Map::clear()
{
    return m_pimpl->clear();
}

void Map::getRooms(AbstractRoomVisitor &stream) const
{
    return m_pimpl->getRooms(stream);
}

void Map::getRooms(AbstractRoomVisitor &stream, const Coordinate &min, const Coordinate &max) const
{
    return m_pimpl->getRooms(stream, min, max);
}

void Map::setNearest(const Coordinate &in_c, Room &room)
{
    const Coordinate c = getNearestFree(in_c);
    m_pimpl->set(c, &room);
    room.setPosition(c);
}

Coordinate Map::getNearestFree(const Coordinate &p)
{
    if (!m_pimpl->defined(p)) {
        return p;
    }

    Coordinate c;
    const int sum1 = (p.x + p.y + p.z) / 2;
    const int sum2 = (p.x + p.y + p.z + 1) / 2;
    const bool random = (sum1 == sum2);
    CoordinateIterator i;
    while (true) {
        if (random) {
            c = p + i.next();
        } else {
            c = p - i.next();
        }
        if (!m_pimpl->defined(c)) {
            return c;
        }
    }
    /*NOTREACHED*/
}

Coordinate &CoordinateIterator::next()
{
    switch (state) {
    case 0:
        c.y *= -1;
        c.x *= -1;
        c.z *= -1;
        break;
    case 1:
        c.z *= -1;
        break;
    case 2:
        c.z *= -1;
        c.y *= -1;
        break;
    case 3:
        c.y *= -1;
        c.x *= -1;
        break;
    case 4:
        c.y *= -1;
        break;
    case 5:
        c.y *= -1;
        c.z *= -1;
        break;
    case 6:
        c.y *= -1;
        c.x *= -1;
        break;
    case 7:
        c.x *= -1;
        break;
    case 8:
        if (c.z < threshold) {
            ++c.z;
        } else {
            c.z = 0;
            if (c.y < threshold) {
                ++c.y;
            } else {
                c.y = 0;
                if (c.x >= threshold) {
                    ++threshold;
                    c.x = 0;
                } else {
                    ++c.x;
                }
            }
        }
        state = -1;
        break;
    }
    ++state;
    return c;
}
