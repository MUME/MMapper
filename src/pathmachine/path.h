#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../mapdata/ExitDirection.h"
#include <climits>
#include <list>
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

class Path final
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

using PathList = std::list<Path *>;
