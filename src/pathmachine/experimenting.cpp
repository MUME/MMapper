// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "experimenting.h"

#include "../global/utils.h"
#include "path.h"
#include "pathparameters.h"

#include <memory>

Experimenting::Experimenting(std::shared_ptr<PathList> pat,
                             const ExitDirEnum in_dirCode,
                             PathParameters &in_params)
    : m_direction(::exitDir(in_dirCode))
    , m_dirCode(in_dirCode)
    , m_paths(PathList::alloc())
    , m_params(in_params)
    , m_shortPaths(std::move(pat))
{}

Experimenting::~Experimenting() = default;

void Experimenting::augmentPath(const std::shared_ptr<Path> &path, const RoomHandle &room)
{
    auto &p = deref(path);
    const Coordinate c = p.getRoom().getPosition() + m_direction;
    const auto working = p.fork(room, c, m_params, m_dirCode);
    if (m_best == nullptr) {
        m_best = working;
    } else if (working->getProb() > m_best->getProb()) {
        m_paths->push_back(m_best);
        m_second = m_best;
        m_best = working;
    } else {
        if (m_second == nullptr || working->getProb() > m_second->getProb()) {
            m_second = working;
        }
        m_paths->push_back(working);
    }
    ++m_numPaths;
}

std::shared_ptr<PathList> Experimenting::evaluate()
{
    for (PathList &sp = deref(m_shortPaths); !sp.empty();) {
        std::shared_ptr<Path> ppath = utils::pop_front(sp);
        Path &path = deref(ppath);
        if (!path.hasChildren()) {
            path.deny();
        }
    }

    if (m_best != nullptr) {
        if (m_second == nullptr
            || m_best->getProb() > m_second->getProb() * m_params.acceptBestRelative
            || m_best->getProb() > m_second->getProb() + m_params.acceptBestAbsolute) {
            for (auto &path : *m_paths) {
                path->deny();
            }
            m_paths->clear();
            m_paths->push_front(m_best);
        } else {
            m_paths->push_back(m_best);

            for (std::shared_ptr<Path> working = m_paths->front(); working != m_best;) {
                m_paths->pop_front();
                // throw away if the probability is very low or not
                // distinguishable from best. Don't keep paths with equal
                // probability at the front, for we need to find a unique
                // best path eventually.
                if (m_best->getProb() > working->getProb() * m_params.maxPaths / m_numPaths
                    || (m_best->getProb() <= working->getProb()
                        && m_best->getRoom() == working->getRoom())) {
                    working->deny();
                } else {
                    m_paths->push_back(working);
                }
                working = m_paths->front();
            }
        }
    }
    m_second = nullptr;
    m_shortPaths = nullptr;
    m_best = nullptr;
    return m_paths;
}
