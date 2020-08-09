#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: 'Elval' <ethorondil@gmail.com> (Elval)

#include <QSet>
#include <QVector>

#include "../expandoracommon/RoomAdmin.h"
#include "../parser/abstractparser.h"
#include "ExitDirection.h"
#include "mmapper2exit.h"

class Room;
class RoomAdmin;

class SPNode
{
public:
    const Room *r = nullptr;
    int parent = 0;
    double dist = 0.0;
    ExitDirEnum lastdir = ExitDirEnum::NONE;
    SPNode() = default;
    explicit SPNode(const Room *r, const int parent, double dist, ExitDirEnum lastdir)
        : r(r)
        , parent(parent)
        , dist(dist)
        , lastdir(lastdir)
    {}
};

class ShortestPathRecipient
{
public:
    virtual void receiveShortestPath(RoomAdmin *admin, QVector<SPNode> spnodes, int endpoint) = 0;
    virtual ~ShortestPathRecipient();
};
