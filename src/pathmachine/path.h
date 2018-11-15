#pragma once
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

#include "../mapdata/ExitDirection.h"
#ifndef PATH
#define PATH

#include <climits>
#include <set>
#include <QtGlobal>

#include "../mapdata/mmapper2exit.h"
#include "pathparameters.h"

class AbstractRoomFactory;
class Coordinate;
class PathParameters;
class Room;
class RoomAdmin;
class RoomRecipient;
class RoomSignalHandler;

class Path
{
public:
    static constexpr const auto INVALID_DIRECTION = static_cast<ExitDirection>(UINT_MAX);
    explicit Path(const Room *room,
                  RoomAdmin *owner,
                  RoomRecipient *locker,
                  RoomSignalHandler *signaler,
                  ExitDirection direction = INVALID_DIRECTION);
    void insertChild(Path *p);
    void removeChild(Path *p);
    void setParent(Path *p);
    bool hasChildren() const { return (!children.empty()); }
    const Room *getRoom() const { return room; }

    // new Path is created, distance between rooms is calculated and probability is set accordingly
    Path *fork(const Room *room,
               const Coordinate &expectedCoordinate,
               RoomAdmin *owner,
               const PathParameters &params,
               RoomRecipient *locker,
               ExitDirection dir);
    double getProb() const { return probability; }
    void approve();

    // deletes this path and all parents up to the next branch
    void deny();
    void setProb(double p) { probability = p; }

    Path *getParent() const { return parent; }

private:
    Path *parent = nullptr;
    std::set<Path *> children{};
    double probability = 1.0;
    const Room *room
        = nullptr; // in fact a path only has one room, one parent and some children (forks).
    RoomSignalHandler *signaler = nullptr;
    ExitDirection dir = INVALID_DIRECTION;
    ~Path() {}
};

#endif
