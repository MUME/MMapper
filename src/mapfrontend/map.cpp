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

#include "map.h"
#include "abstractroomfactory.h"
#include "room.h"
#include "roomoutstream.h"
#include <QTime>
#include <iostream>

void Map::clear()
{
    m_map.clear();
}

void Map::getRooms(RoomOutStream &stream, const Coordinate &ulf, const Coordinate &lrb)
{
    //QTime start = QTime::currentTime();
    //int checks = 0;
    int xmin = (ulf.x < lrb.x ? ulf.x : lrb.x) - 1;
    int xmax = (ulf.x > lrb.x ? ulf.x : lrb.x) + 1;
    int ymin = (ulf.y < lrb.y ? ulf.y : lrb.y) - 1;
    int ymax = (ulf.y > lrb.y ? ulf.y : lrb.y) + 1;
    int zmin = (ulf.z < lrb.z ? ulf.z : lrb.z) - 1;
    int zmax = (ulf.z > lrb.z ? ulf.z : lrb.z) + 1;

    auto zUpper = m_map.lower_bound(zmax);
    for (auto z = m_map.upper_bound(zmin); z != zUpper; ++z) {
        auto &ymap = (*z).second;
        auto yUpper = ymap.lower_bound(ymax);
        for (auto y = ymap.upper_bound(ymin); y != yUpper; ++y) {
            auto &xmap = (*y).second;
            auto xUpper = xmap.lower_bound(xmax);
            for (auto x = xmap.upper_bound(xmin); x != xUpper; ++x) {
                stream << (*x).second;
                //++checks;
            }
        }
    }
    //cout << "rendering took " << start.msecsTo(QTime::currentTime()) << " msecs for " << checks << " checks" << endl;
}

void Map::fillArea(AbstractRoomFactory *factory, const Coordinate &ulf, const Coordinate &lrb)
{
    int xmin = ulf.x < lrb.x ? ulf.x : lrb.x;
    int xmax = ulf.x > lrb.x ? ulf.x : lrb.x;
    int ymin = ulf.y < lrb.y ? ulf.y : lrb.y;
    int ymax = ulf.y > lrb.y ? ulf.y : lrb.y;
    int zmin = ulf.z < lrb.z ? ulf.z : lrb.z;
    int zmax = ulf.z > lrb.z ? ulf.z : lrb.z;

    for (int z = zmin; z <= zmax; ++z) {
        for (int y = ymin; y <= ymax; ++y) {
            for (int x = xmin; x <= xmax; ++x) {
                Room *&room = m_map[z][y][x];
                if (room == nullptr) {
                    room = factory->createRoom();
                }
            }
        }
    }
}

/**
 * doesn't modify c
 */
bool Map::defined(const Coordinate &c)
{
    auto z = m_map.find(c.z);
    if (z != m_map.end()) {
        auto &ySeg = (*z).second;
        auto y = ySeg.find(c.y);
        if (y != ySeg.end()) {
            auto &xSeg = (*y).second;
            if (xSeg.find(c.x) != xSeg.end()) {
                return true;
            }
        }
    }
    return false;
}


Room *Map::get(const Coordinate &c)
{
    if (!defined(c)) {
        return nullptr;
    }
    return m_map[c.z][c.y][c.x];
}

void Map::remove(const Coordinate &c)
{
    m_map[c.z][c.y].erase(c.x);
}

/**
 * doesn't modify c
 */
void Map::set(const Coordinate &c, Room *room)
{
    m_map[c.z][c.y][c.x] = room;
}

/**
 * gets a new coordinate but doesn't return the old one ... should probably be changed ...
 */
Coordinate Map::setNearest(const Coordinate &in_c, Room &room)
{
    Coordinate c = getNearestFree(in_c);
    set(c, &room);
    room.setPosition(c);
    return c;
}

Coordinate Map::getNearestFree(const Coordinate &p)
{
    Coordinate c;
    int sum1 = (p.x + p.y + p.z) / 2;
    int sum2 = (p.x + p.y + p.z + 1) / 2;
    bool random = (sum1 == sum2);
    CoordinateIterator i;
    while (true) {
        if (random) {
            c = p + i.next();
        } else {
            c = p - i.next();
        }
        if (!defined(c)) {
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
