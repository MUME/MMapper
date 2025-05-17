#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Badge.h"
#include "../map/DoorFlags.h"
#include "../map/ExitDirection.h"
#include "../map/ExitFieldVariant.h"
#include "../map/ExitFlags.h"
#include "../map/RoomHandle.h"
#include "pathparameters.h"

#include <cassert>
#include <climits>
#include <list>
#include <memory>
#include <optional>
#include <vector>

#include <QtGlobal>

class Coordinate;
class RoomRecipient;
class RoomSignalHandler;
struct PathParameters;

class NODISCARD Path final : public std::enable_shared_from_this<Path>
{
private:
    std::shared_ptr<Path> m_parent;
    std::vector<std::weak_ptr<Path>> m_children;
    double m_probability = 1.0;
    // in fact a path only has one room, one parent and some children (forks).
    const RoomHandle m_room;
    RoomSignalHandler *const m_signaler;
    const std::optional<ExitDirEnum> m_dir;
    bool m_zombie = false;

public:
    static std::shared_ptr<Path> alloc(const RoomHandle &room,
                                       RoomRecipient *locker,
                                       RoomSignalHandler *signaler,
                                       std::optional<ExitDirEnum> direction);

public:
    explicit Path(Badge<Path>,
                  RoomHandle moved_room,
                  RoomRecipient *locker,
                  RoomSignalHandler *signaler,
                  std::optional<ExitDirEnum> direction);
    DELETE_CTORS_AND_ASSIGN_OPS(Path);

    void insertChild(const std::shared_ptr<Path> &p);
    void removeChild(const std::shared_ptr<Path> &p);
    void setParent(const std::shared_ptr<Path> &p);
    NODISCARD bool hasChildren() const
    {
        assert(!m_zombie);
        return !m_children.empty();
    }
    NODISCARD RoomHandle getRoom() const
    {
        assert(!m_zombie);
        return m_room;
    }

    // new Path is created, distance between rooms is calculated and probability is set accordingly
    NODISCARD std::shared_ptr<Path> fork(const RoomHandle &room,
                                         const Coordinate &expectedCoordinate,
                                         const PathParameters &params,
                                         RoomRecipient *locker,
                                         ExitDirEnum dir);
    NODISCARD double getProb() const
    {
        assert(!m_zombie);
        return m_probability;
    }
    void approve(ChangeList &changes);

    // deletes this path and all parents up to the next branch
    void deny();
    void setProb(double p)
    {
        assert(!m_zombie);
        m_probability = p;
    }

    NODISCARD const std::shared_ptr<Path> &getParent() const
    {
        assert(!m_zombie);
        return m_parent;
    }
};

struct NODISCARD PathList : public std::list<std::shared_ptr<Path>>,
                            public std::enable_shared_from_this<PathList>
{
public:
    NODISCARD static std::shared_ptr<PathList> alloc()
    {
        return std::make_shared<PathList>(Badge<PathList>{});
    }

public:
    explicit PathList(Badge<PathList>) {}
    DELETE_CTORS_AND_ASSIGN_OPS(PathList);
};
