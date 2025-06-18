#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../map/parseevent.h"
#include "experimenting.h"
#include "roomsignalhandler.h"

#include <memory>

class ParseEvent;
class Path;
struct PathParameters;

/*!
 * @brief PathProcessor strategy for exploring from existing paths to known rooms.
 *
 * Used in "Experimenting" state, typically when not creating new rooms.
 * PathMachine feeds it rooms found via current paths' exits/coordinates.
 * If a received room matches the event, `augmentPath()` (from Experimenting)
 * is called to extend the current path being processed.
 */
class NODISCARD OneByOne final : public Experimenting
{
private:
    SharedParseEvent m_event;
    RoomSignalHandler &m_handler;

public:
    explicit OneByOne(const SigParseEvent &sigParseEvent,
                      PathParameters &in_params,
                      RoomSignalHandler &handler);

private:
    void virt_receiveRoom(const RoomHandle &room) final;

public:
    void addPath(std::shared_ptr<Path> path);
};
