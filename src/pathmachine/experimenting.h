#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/RuleOf5.h"
#include "../map/DoorFlags.h"
#include "../map/ExitDirection.h"
#include "../map/ExitFieldVariant.h"
#include "../map/ExitFlags.h"
#include "../map/RoomRecipient.h"
#include "../map/coordinate.h"
#include "path.h"

#include <memory>

#include <QtGlobal>

class PathMachine;
struct PathParameters;

// Base class for Crossover and OneByOne
class NODISCARD Experimenting : public RoomRecipient
{
protected:
    void augmentPath(const std::shared_ptr<Path> &path, const RoomHandle &room);
    const Coordinate direction;
    const ExitDirEnum dirCode;
    const std::shared_ptr<PathList> paths;
    PathParameters &params;
    std::shared_ptr<PathList> shortPaths;
    std::shared_ptr<Path> best;
    std::shared_ptr<Path> second;
    double numPaths = 0.0;

public:
    Experimenting() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(Experimenting);

protected:
    explicit Experimenting(std::shared_ptr<PathList> paths,
                           ExitDirEnum dirCode,
                           PathParameters &params);

public:
    ~Experimenting() override;

public:
    std::shared_ptr<PathList> evaluate();
};
