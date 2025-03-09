#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: 'Elval' <ethorondil@gmail.com> (Elval)

#include "../map/DoorFlags.h"
#include "../map/ExitDirection.h"
#include "../map/ExitFieldVariant.h"
#include "../map/ExitFlags.h"
#include "../parser/abstractparser.h"

#include <QSet>
#include <QVector>

struct NODISCARD SPNode final
{
    RoomHandle r;
    int parent = 0;
    double dist = 0.0;
    ExitDirEnum lastdir = ExitDirEnum::NONE;
};

class NODISCARD ShortestPathRecipient
{
public:
    virtual ~ShortestPathRecipient();

private:
    virtual void virt_receiveShortestPath(QVector<SPNode> spnodes, int endpoint) = 0;

public:
    void receiveShortestPath(QVector<SPNode> spnodes, const int endpoint)
    {
        virt_receiveShortestPath(spnodes, endpoint);
    }
};
