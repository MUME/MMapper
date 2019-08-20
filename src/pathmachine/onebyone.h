#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../expandoracommon/parseevent.h"
#include "experimenting.h"

class ParseEvent;
class Path;
class Room;
class RoomAdmin;
class RoomSignalHandler;
struct PathParameters;

class OneByOne final : public Experimenting
{
public:
    explicit OneByOne(const SigParseEvent &sigParseEvent,
                      PathParameters &in_params,
                      RoomSignalHandler *handler);
    void receiveRoom(RoomAdmin *admin, const Room *room) override;
    void addPath(Path *path);

private:
    SharedParseEvent event;
    RoomSignalHandler *handler = nullptr;
};
