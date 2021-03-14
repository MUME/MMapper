#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <list>
#include <memory>
#include <QtGlobal>

#include "../expandoracommon/RoomRecipient.h"
#include "../global/RuleOf5.h"
#include "path.h"

class Room;
class RoomAdmin;
class RoomSignalHandler;
struct PathParameters;

class NODISCARD Syncing : public RoomRecipient
{
private:
    RoomSignalHandler *signaler = nullptr;
    uint numPaths = 0u;
    PathParameters &params;
    const std::shared_ptr<PathList> paths;
    // This is not our parent; it's the parent we assign to new objects.
    std::shared_ptr<Path> parent;

public:
    explicit Syncing(PathParameters &p,
                     std::shared_ptr<PathList> paths,
                     RoomSignalHandler *signaler);
    void receiveRoom(RoomAdmin *, const Room *) override;
    std::shared_ptr<PathList> evaluate();
    ~Syncing() override;

public:
    Syncing() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(Syncing);
};
