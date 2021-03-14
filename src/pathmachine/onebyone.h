#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <memory>

#include "../expandoracommon/parseevent.h"
#include "experimenting.h"

class ParseEvent;
class Path;
class Room;
class RoomAdmin;
class RoomSignalHandler;
struct PathParameters;

class NODISCARD OneByOne final : public Experimenting
{
private:
    SharedParseEvent event;
    RoomSignalHandler *handler = nullptr;

public:
    explicit OneByOne(const SigParseEvent &sigParseEvent,
                      PathParameters &in_params,
                      RoomSignalHandler *handler);

private:
    void virt_receiveRoom(RoomAdmin *admin, const Room *room) final;

public:
    void addPath(std::shared_ptr<Path> path);
};
