#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "AbstractRoomVisitor.h"

class Room;
class MapFrontend;
class ParseEvent;
class AbstractRoomFactory;
class RoomRecipient;

class RoomLocker final : public AbstractRoomVisitor
{
public:
    explicit RoomLocker(RoomRecipient &forward,
                        MapFrontend &frontend,
                        AbstractRoomFactory *factory = nullptr,
                        ParseEvent *compare = nullptr);
    virtual void visit(const Room *room) override;
    virtual ~RoomLocker() override;

private:
    RoomRecipient &recipient;
    MapFrontend &data;
    AbstractRoomFactory *factory;
    ParseEvent *comparator;
};
