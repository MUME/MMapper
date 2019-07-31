#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../mapdata/mmapper2exit.h"
#include "parseevent.h"
#include <QtGlobal>

class Room;
class Coordinate;

enum class ComparisonResultEnum { DIFFERENT = 0, EQUAL, TOLERANCE };

class AbstractRoomFactory
{
public:
    virtual ~AbstractRoomFactory();
    virtual Room *createRoom() const = 0;
    virtual Room *createRoom(const ParseEvent &event) const = 0;
    virtual ComparisonResultEnum compare(const Room *,
                                         const ParseEvent &props,
                                         int tolerance = 0) const = 0;
    virtual ComparisonResultEnum compareWeakProps(const Room *,
                                                  const ParseEvent &props,
                                                  int tolerance = 0) const = 0;
    virtual SharedParseEvent getEvent(const Room *) const = 0;
    virtual void update(Room &, const ParseEvent &event) const = 0;
    virtual void update(Room *target, const Room *source) const = 0;
};
