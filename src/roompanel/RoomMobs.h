#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <unordered_map>
#include <utility>
#include <vector>
#include <QObject>
#include <QRecursiveMutex>
#include <QtCore>

#include "../global/RuleOf5.h"
#include "RoomMob.h"

class RoomMobs final : public QObject
{
    Q_OBJECT

public:
    explicit RoomMobs(QObject *parent);
    ~RoomMobs() final = default;
    DELETE_CTORS_AND_ASSIGN_OPS(RoomMobs);

    void updateModel(std::unordered_map<RoomMob::Id, SharedRoomMob> &mobsById,
                     std::vector<SharedRoomMob> &mobVector) const;

    NODISCARD bool isIdPresent(const RoomMob::Id id) const;
    NODISCARD SharedRoomMob getMobById(const RoomMob::Id id) const;
    void resetMobs();
    void addMob(RoomMobUpdate &&mob);
    bool removeMobById(const RoomMob::Id id);      // return true if removed
    NODISCARD bool updateMob(RoomMobUpdate &&mob); // return true if modified

signals:
    void sig_mobsChanged();

private:
    void mobsChanged();

private:
    struct SharedRoomMobAndIndex
    {
        SharedRoomMob mob;
        size_t index;
    };

    // mobs ordered by ID
    std::unordered_map<RoomMob::Id, SharedRoomMobAndIndex> m_mobs;
    // mobs ordered as they should be shown
    std::map<size_t, SharedRoomMob> m_mobsByIndex;
    mutable QRecursiveMutex mutex;
    size_t m_nextIndex;
};
