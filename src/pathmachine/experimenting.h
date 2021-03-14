#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <memory>
#include <QtGlobal>

#include "../expandoracommon/RoomRecipient.h"
#include "../expandoracommon/coordinate.h"
#include "../global/RuleOf5.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/mmapper2exit.h"
#include "path.h"

class PathMachine;
class Room;
class RoomAdmin;
struct PathParameters;

// Base class for Crossover and OneByOne
class NODISCARD Experimenting : public RoomRecipient
{
protected:
    void augmentPath(const std::shared_ptr<Path> &path, RoomAdmin *map, const Room *room);
    const Coordinate direction;
    const ExitDirEnum dirCode;
    const std::shared_ptr<PathList> paths;
    PathParameters &params;
    std::shared_ptr<PathList> shortPaths;
    std::shared_ptr<Path> best;
    std::shared_ptr<Path> second;
    double numPaths = 0.0;

protected:
    Experimenting() = delete;
    explicit Experimenting(std::shared_ptr<PathList> paths,
                           ExitDirEnum dirCode,
                           PathParameters &params);

public:
    virtual ~Experimenting() override;
    DELETE_CTORS_AND_ASSIGN_OPS(Experimenting);

public:
    NODISCARD std::shared_ptr<PathList> evaluate();
};
