#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"
#include "../global/macros.h"

class RoomHandle;

/*!
 * @brief Defines an interface for various room processing and pathfinding strategies.
 *
 * PathProcessor is an abstract base class used by PathMachine to delegate
 * the logic of how incoming room data (from parse events or map lookups)
 * should be processed in different pathfinding states (e.g., Approved,
 * Experimenting, Syncing). Concrete subclasses implement specific strategies
 * for matching rooms, evaluating potential paths, or creating new path segments.
 *
 * The primary interface method is virt_receiveRoom(), which is called to pass
 * a RoomHandle to the strategy.
 */
class NODISCARD PathProcessor
{
public:
    PathProcessor();
    virtual ~PathProcessor();

private:
    virtual void virt_receiveRoom(const RoomHandle &) = 0;

public:
    void receiveRoom(const RoomHandle &room) { virt_receiveRoom(room); }

public:
    DELETE_CTORS_AND_ASSIGN_OPS(PathProcessor);
};
