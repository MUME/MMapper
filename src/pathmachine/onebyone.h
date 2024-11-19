#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../map/parseevent.h"
#include "experimenting.h"

#include <memory>

class ParseEvent;
class Path;
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
    void virt_receiveRoom(const RoomHandle &room) final;

public:
    void addPath(std::shared_ptr<Path> path);
};
