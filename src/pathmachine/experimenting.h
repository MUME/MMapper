#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <list>
#include <QtGlobal>

#include "../expandoracommon/RoomRecipient.h"
#include "../expandoracommon/coordinate.h"
#include "../global/RuleOf5.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/mmapper2exit.h"
#include "path.h"

class AbstractRoomFactory;
class PathMachine;
class Room;
class RoomAdmin;
struct PathParameters;

class Experimenting : public RoomRecipient
{
protected:
    void augmentPath(Path *path, RoomAdmin *map, const Room *room);
    const Coordinate direction;
    const ExitDirEnum dirCode;
    PathList *const paths;
    PathParameters &params;
    PathList *shortPaths = nullptr;
    Path *best = nullptr;
    Path *second = nullptr;
    double numPaths = 0.0;
    AbstractRoomFactory *factory = nullptr;

public:
    explicit Experimenting(PathList *paths,
                           ExitDirEnum dirCode,
                           PathParameters &params,
                           AbstractRoomFactory *factory);
    virtual ~Experimenting() override;

    PathList *evaluate();
    virtual void receiveRoom(RoomAdmin *, const Room *) override = 0;

public:
    Experimenting() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(Experimenting);
};
