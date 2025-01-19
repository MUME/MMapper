#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: 'Elval' <ethorondil@gmail.com> (Elval)

#include "../expandoracommon/RoomAdmin.h"
#include "../map/DoorFlags.h"
#include "../map/ExitDirection.h"
#include "../map/ExitFieldVariant.h"
#include "../map/ExitFlags.h"
#include "../parser/abstractparser.h"

#include <QSet>
#include <QVector>

class Room;
class RoomAdmin;

class NODISCARD SPNode final
{
public:
    const Room *r = nullptr;
    int parent = 0;
    double dist = 0.0;
    ExitDirEnum lastdir = ExitDirEnum::NONE;

public:
    SPNode() = default;
    explicit SPNode(const Room *r, const int parent, double dist, ExitDirEnum lastdir)
        : r(r)
        , parent(parent)
        , dist(dist)
        , lastdir(lastdir)
    {}
};

class NODISCARD ShortestPathRecipient
{
public:
    virtual ~ShortestPathRecipient();

private:
    virtual void virt_receiveShortestPath(RoomAdmin *admin, QVector<SPNode> spnodes, int endpoint)
        = 0;

public:
    void receiveShortestPath(RoomAdmin *const admin, QVector<SPNode> spnodes, const int endpoint)
    {
        virt_receiveShortestPath(admin, std::move(spnodes), endpoint);
    }
};
