// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "experimenting.h"

#include "../expandoracommon/room.h"
#include "../global/utils.h"
#include "path.h"
#include "pathparameters.h"

#include <memory>

Experimenting::Experimenting(std::shared_ptr<PathList> pat,
                             const ExitDirEnum in_dirCode,
                             PathParameters &in_params)
    : direction(Room::exitDir(in_dirCode))
    , dirCode(in_dirCode)
    , paths(PathList::alloc())
    , params(in_params)
    , shortPaths(std::move(pat))
{}

Experimenting::~Experimenting() = default;

void Experimenting::augmentPath(const std::shared_ptr<Path> &path,
                                RoomAdmin *const map,
                                const Room *const room)
{
    const Coordinate c = path->getRoom()->getPosition() + direction;
    const auto working = path->fork(room, c, map, params, this, dirCode);
    if (best == nullptr) {
        best = working;
    } else if (working->getProb() > best->getProb()) {
        paths->push_back(best);
        second = best;
        best = working;
    } else {
        if (second == nullptr || working->getProb() > second->getProb()) {
            second = working;
        }
        paths->push_back(working);
    }
    numPaths++;
}

std::shared_ptr<PathList> Experimenting::evaluate()
{
    for (PathList &sp = deref(shortPaths); !sp.empty();) {
        std::shared_ptr<Path> ppath = utils::pop_front(sp);
        Path &path = deref(ppath);
        if (!path.hasChildren()) {
            path.deny();
        }
    }

    if (best != nullptr) {
        if (second == nullptr || best->getProb() > second->getProb() * params.acceptBestRelative
            || best->getProb() > second->getProb() + params.acceptBestAbsolute) {
            for (auto &path : *paths) {
                path->deny();
            }
            paths->clear();
            paths->push_front(best);
        } else {
            paths->push_back(best);

            for (std::shared_ptr<Path> working = paths->front(); working != best;) {
                paths->pop_front();
                // throw away if the probability is very low or not
                // distinguishable from best. Don't keep paths with equal
                // probability at the front, for we need to find a unique
                // best path eventually.
                if (best->getProb() > working->getProb() * params.maxPaths / numPaths
                    || (best->getProb() <= working->getProb()
                        && best->getRoom() == working->getRoom())) {
                    working->deny();
                } else {
                    paths->push_back(working);
                }
                working = paths->front();
            }
        }
    }
    second = nullptr;
    shortPaths = nullptr;
    best = nullptr;
    return paths;
}
