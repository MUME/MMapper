#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../map/ChangeList.h"
#include "../map/DoorFlags.h"
#include "../map/ExitDirection.h"
#include "../map/ExitFieldVariant.h"
#include "../map/ExitFlags.h"
#include "../map/roomid.h"

#include <map>
#include <set>

#include <QString>

class PathProcessor;
struct RoomId;
class MapFrontend;

/*!
 * @brief Manages room lifecycle signals and "holds" during pathfinding.
 *
 * RoomSignalHandler is responsible for tracking which PathProcessor strategies
 * or Path objects have an active interest in a particular RoomId. This is done
 * primarily through a "hold count" per room, managed in `m_holdCount`.
 *
 * Key functionalities:
 * - `hold(RoomId)`: Called to indicate a Path or strategy is currently using or
 *   evaluating a room. Increments the room's hold count.
 * - `release(RoomId room)`: Decrements a room's hold count. If the count reaches zero
 *   for a temporary room, it may be queued for removal.
 * - `keep(RoomId room, ExitDirEnum dir, RoomId fromId, ChangeList &changes)`:
 *   Converts a "hold" to a "kept" state (e.g., making a temporary room permanent
 *   and adding an exit). It then calls `release()` to decrement the hold count that
 *   was covering the initial exploration of the room.
 * - `getNumHolders(RoomId room)`: Returns the current hold count for a room,
 *   indicating how many active interests are registered for it.
 *
 * Owned by PathMachine, it queues changes to a ChangeList rather than applying
 * them directly.
 */
class NODISCARD_QOBJECT RoomSignalHandler final : public QObject
{
    Q_OBJECT

private:
    MapFrontend &m_map;
    RoomIdSet m_owners;
    std::map<RoomId, size_t> m_holdCount;

public:
    RoomSignalHandler() = delete;
    explicit RoomSignalHandler(MapFrontend &map, QObject *parent)
        : QObject(parent)
        , m_map{map}
    {}
    /* receiving from our clients: */
    // hold the room, indicating it's in use or being evaluated.
    // Overrides release if the room was previously un-cached.
    void hold(RoomId room);
    // room isn't needed anymore and can be deleted if its hold count reaches zero and it's temporary.
    void release(RoomId room);
    // keep the room but un-cache it - overrides both hold and release
    // toId is negative if no exit should be added, else it's the id of
    // the room where the exit should lead
    void keep(RoomId room, ExitDirEnum dir, RoomId fromId, ChangeList &changes);

    /* Sending to the rooms' owners:
       keepRoom: keep the room, but we don't need it anymore for now
       releaseRoom: delete the room, if you like */

    // Returns the number of active holds on the given room.
    NODISCARD size_t getNumHolders(RoomId room) const;
};
