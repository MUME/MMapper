#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <list>
#include <QtGlobal>

#include "../expandoracommon/RoomRecipient.h"
#include "../global/RuleOf5.h"

class Path;
class PathParameters;
class Room;
class RoomAdmin;
class RoomSignalHandler;

class Syncing : public RoomRecipient
{
private:
    RoomSignalHandler *signaler = nullptr;
    uint numPaths = 0u;
    PathParameters &params;
    std::list<Path *> *paths = nullptr;
    Path *parent = nullptr;

public:
    explicit Syncing(PathParameters &p, std::list<Path *> *paths, RoomSignalHandler *signaler);
    void receiveRoom(RoomAdmin *, const Room *) override;
    std::list<Path *> *evaluate();
    ~Syncing() override;

public:
    Syncing() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(Syncing);
};
