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

#ifndef MAP
#define MAP

#include "coordinate.h"
#include <map>

class Room;
class RoomOutStream;
class AbstractRoomFactory;

/**
 * The Map stores the geographic relations of rooms to each other
 * it doesn't store the search tree. The Map class is only used by the
 * RoomAdmin, which also stores the search tree
 */
class Map
{
  public:
    bool defined(const Coordinate &c);
    Coordinate setNearest(const Coordinate &c, Room *room);
    Room * get(const Coordinate &c);
    void remove(const Coordinate & c);
    void clear();
    void getRooms(RoomOutStream & stream, const Coordinate & ulf, const Coordinate & lrb);
    void fillArea(AbstractRoomFactory * factory, const Coordinate & ulf, const Coordinate & lrb);

  private:
    void set(const Coordinate &c, Room *room);
    Coordinate getNearestFree(const Coordinate & c);
    std::map<int, std::map<int, std::map<int, Room *> > > m_map;
};

class CoordinateIterator
{
  public:
    CoordinateIterator() : threshold(1), state(7) {}
    Coordinate & next();
  private:
    Coordinate c;
    int threshold;
    int state;
};

#endif
