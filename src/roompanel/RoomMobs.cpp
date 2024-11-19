// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "RoomMobs.h"

#include "../configuration/configuration.h"

#include <unordered_map>
#include <vector>

RoomMobs::RoomMobs(QObject *const parent)
    : QObject(parent)
    , m_mobs()
    , m_mobsByIndex()
    , m_nextIndex(0)
{}

void RoomMobs::updateModel(std::unordered_map<RoomMob::Id, SharedRoomMob> &mobsById,
                           std::vector<SharedRoomMob> &mobVector) const
{
    QMutexLocker locker(&mutex);

    mobsById.clear();
    mobVector.clear();

    mobsById.reserve(m_mobs.size());
    for (const auto &pair : m_mobs) {
        mobsById.emplace(pair.first, pair.second.mob);
    }
    mobVector.reserve(m_mobsByIndex.size());
    for (const auto &pair : m_mobsByIndex) {
        mobVector.push_back(pair.second);
    }
}

bool RoomMobs::isIdPresent(const RoomMob::Id id) const
{
    QMutexLocker locker(&mutex);
    return m_mobs.find(id) != m_mobs.end();
}

SharedRoomMob RoomMobs::getMobById(const RoomMob::Id id) const
{
    QMutexLocker locker(&mutex);
    auto it = m_mobs.find(id);
    if (it != m_mobs.end()) {
        return it->second.mob;
    }
    return {};
}

void RoomMobs::resetMobs()
{
    QMutexLocker locker(&mutex);
    m_mobs.clear();
    m_mobsByIndex.clear();
    m_nextIndex = 0;
}

void RoomMobs::addMob(RoomMobUpdate &&mob)
{
    const RoomMob::Id id = mob.getId();
    removeMobById(id); // in case it's already present

    SharedRoomMob newMob = RoomMob::alloc();
    newMob->setId(id);
    newMob->updateFrom(std::move(mob));

    QMutexLocker locker(&mutex);
    const size_t index = m_nextIndex++;
    m_mobs.try_emplace(id, SharedRoomMobAndIndex{newMob, index});
    m_mobsByIndex.try_emplace(index, newMob);
}

bool RoomMobs::removeMobById(const RoomMob::Id id)
{
    QMutexLocker locker(&mutex);
    auto it = m_mobs.find(id);
    if (it == m_mobs.end()) {
        return false;
    }
    const size_t index = it->second.index;
    m_mobs.erase(it);
    m_mobsByIndex.erase(index);
    if (m_mobsByIndex.empty()) {
        m_nextIndex = 0;
    } else {
        m_nextIndex = 1 + (--m_mobsByIndex.end())->first;
    }
    return true;
}

bool RoomMobs::updateMob(RoomMobUpdate &&mob)
{
    const SharedRoomMob currMob = getMobById(mob.getId());
    if (!currMob) {
        addMob(std::move(mob));
        return true;
    } else {
        return currMob->updateFrom(std::move(mob));
    }
}
