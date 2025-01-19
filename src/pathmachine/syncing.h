#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/RuleOf5.h"
#include "../map/RoomRecipient.h"
#include "path.h"

#include <list>
#include <memory>

#include <QtGlobal>

class Room;
class RoomAdmin;
class RoomSignalHandler;
struct PathParameters;

class NODISCARD Syncing final : public RoomRecipient
{
private:
    RoomSignalHandler *signaler = nullptr;
    PathParameters &params;
    const std::shared_ptr<PathList> paths;
    // This is not our parent; it's the parent we assign to new objects.
    std::shared_ptr<Path> parent;
    uint32_t numPaths = 0u;

public:
    explicit Syncing(PathParameters &p,
                     std::shared_ptr<PathList> paths,
                     RoomSignalHandler *signaler);

public:
    Syncing() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(Syncing);

private:
    void virt_receiveRoom(RoomAdmin *, const Room *) final;

public:
    NODISCARD std::shared_ptr<PathList> evaluate();
    ~Syncing() override;
};
