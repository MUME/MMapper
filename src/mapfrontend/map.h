#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../expandoracommon/coordinate.h"
#include "../global/RuleOf5.h"

#include <memory>

class AbstractRoomVisitor;
class Room;

/**
 * The Map stores the geographic relations of rooms to each other
 * it doesn't store the search tree. The Map class is only used by the
 * RoomAdmin, which also stores the search tree
 */
class Map final
{
public:
    class MapOrderedTree;

private:
    std::unique_ptr<MapOrderedTree> m_pimpl;

public:
    Map();
    ~Map();
    DELETE_CTORS_AND_ASSIGN_OPS(Map);

public:
    bool defined(const Coordinate &c) const;
    Coordinate setNearest(const Coordinate &c, Room &room);
    Room *get(const Coordinate &c) const;
    void remove(const Coordinate &c);
    void clear();
    void getRooms(AbstractRoomVisitor &stream) const;
    void getRooms(AbstractRoomVisitor &stream, const Coordinate &min, const Coordinate &max) const;

private:
    Coordinate getNearestFree(const Coordinate &c);
};

class CoordinateIterator final
{
public:
    CoordinateIterator() = default;
    Coordinate &next();

private:
    Coordinate c;
    int threshold = 1;
    int state = 7;
};
