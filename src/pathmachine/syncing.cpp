// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "syncing.h"

#include "../mapdata/ExitDirection.h"
#include "path.h"
#include "pathparameters.h"

class Room;

Syncing::Syncing(PathParameters &in_p, PathList *in_paths, RoomSignalHandler *in_signaler)
    : signaler(in_signaler)
    , params(in_p)
    , paths(in_paths)
    , parent(new Path(nullptr, nullptr, this, signaler))
{}

void Syncing::receiveRoom(RoomAdmin *sender, const Room *in_room)
{
    if (++numPaths > params.maxPaths) {
        if (!paths->empty()) {
            for (auto &path : *paths) {
                path->deny();
            }
            paths->clear();
            parent = nullptr;
        }
    } else {
        auto *p = new Path(in_room, sender, this, signaler, ExitDirection::NONE);
        p->setParent(parent);
        parent->insertChild(p);
        paths->push_back(p);
    }
}

PathList *Syncing::evaluate()
{
    return paths;
}

Syncing::~Syncing()
{
    if (parent != nullptr) {
        parent->deny();
    }
}
