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

#ifndef EXPERIMENTING_H
#define EXPERIMENTING_H

#include <list>
#include <QtGlobal>

#include "../expandoracommon/RoomRecipient.h"
#include "../expandoracommon/coordinate.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/mmapper2exit.h"

class AbstractRoomFactory;
class Path;
class PathMachine;
class PathParameters;
class Room;
class RoomAdmin;

class Experimenting : public RoomRecipient
{
protected:
    void augmentPath(Path *path, RoomAdmin *map, const Room *room);
    Coordinate direction{};
    ExitDirection dirCode = ExitDirection::UNKNOWN;
    std::list<Path *> *shortPaths = nullptr;
    std::list<Path *> *paths = nullptr;
    Path *best = nullptr;
    Path *second = nullptr;
    PathParameters &params;
    double numPaths = 0.0;
    AbstractRoomFactory *factory = nullptr;

public:
    explicit Experimenting(std::list<Path *> *paths,
                           ExitDirection dirCode,
                           PathParameters &params,
                           AbstractRoomFactory *factory);
    virtual ~Experimenting() override;

    std::list<Path *> *evaluate();
    virtual void receiveRoom(RoomAdmin *, const Room *) override = 0;

public:
    Experimenting() = delete;
    Experimenting(Experimenting &&) = delete;
    Experimenting(const Experimenting &) = delete;
    Experimenting &operator=(Experimenting &&) = delete;
    Experimenting &operator=(const Experimenting &) = delete;
};

#endif
