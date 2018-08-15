/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "path.h"

#include <cassert>
#include <cstdint>

#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../global/utils.h"
#include "../mapdata/ExitDirection.h"
#include "pathparameters.h"
#include "roomsignalhandler.h"

Path::Path(const Room *in_room,
           RoomAdmin *owner,
           RoomRecipient *locker,
           RoomSignalHandler *in_signaler,
           ExitDirection direction)
    : room(in_room)
    , signaler(in_signaler)
    , dir(direction)
{
    if (dir != INVALID_DIRECTION) {
        signaler->hold(room, owner, locker);
    }
}

/**
 * new Path is created,
 * distance between rooms is calculated
 * and probability is updated accordingly
 */
Path *Path::fork(const Room *const in_room,
                 const Coordinate &expectedCoordinate,
                 RoomAdmin *const owner,
                 const PathParameters &p,
                 RoomRecipient *const locker,
                 const ExitDirection direction)
{
    auto *const ret = new Path(in_room, owner, locker, signaler, direction);
    assert(ret != parent);
    assert(isClamped(static_cast<uint32_t>(direction), 0u, NUM_EXITS));

    ret->setParent(this);
    children.insert(ret);

    double dist = expectedCoordinate.distance(in_room->getPosition());
    const auto size = static_cast<uint>(room->getExitsList().size());
    // NOTE: we can probably assert that size is nonzero (room is not a dummy).
    assert(size == 0u /* dummy */ || size == NUM_EXITS /* valid */);

    if (dist < 0.5) {
        if (static_cast<uint>(direction) < NUM_EXITS_INCLUDING_NONE) {
            /* NOTE: This is currently always true unless the data is corrupt. */
            dist = 1.0 / p.correctPositionBonus;
        } else {
            dist = p.multipleConnectionsPenalty;
        }
    } else {
        if (static_cast<uint>(direction) < size) {
            const Exit &e = room->exit(direction);
            auto oid = in_room->getId();
            if (e.containsOut(oid)) {
                dist = 1.0 / p.correctPositionBonus;
            } else if (!e.outIsEmpty() || oid == room->getId()) {
                dist *= p.multipleConnectionsPenalty;
            } else {
                const Exit &oe = in_room->exit(opposite(direction));
                if (!oe.inIsEmpty()) {
                    dist *= p.multipleConnectionsPenalty;
                }
            }

        } else if (static_cast<uint>(direction) < NUM_EXITS_INCLUDING_NONE) {
            /* NOTE: This is currently always true unless the data is corrupt. */
            for (uint d = 0; d < size; ++d) {
                const Exit &e = room->exit(static_cast<ExitDirection>(d));
                if (e.containsOut(in_room->getId())) {
                    dist = 1.0 / p.correctPositionBonus;
                    break;
                }
            }
        }
    }
    dist /= static_cast<double>(signaler->getNumLockers(in_room));
    if (in_room->isTemporary()) {
        dist *= p.newRoomPenalty;
    }
    ret->setProb(probability / dist);

    return ret;
}

void Path::setParent(Path *p)
{
    parent = p;
}

void Path::approve()
{
    if (parent == nullptr) {
        assert(dir == INVALID_DIRECTION);
    } else {
        const Room *const proom = parent->getRoom();
        const auto pId = (proom == nullptr) ? INVALID_ROOMID : proom->getId();
        signaler->keep(room, dir, pId);
        parent->removeChild(this);
        parent->approve();
    }

    for (Path *const child : children) {
        if (child != nullptr)
            child->setParent(nullptr);
    }

    // UGG!
    delete this;
}

/** removes this path and all parents up to the next branch
 * and removes the respective rooms if experimental
 */
void Path::deny()
{
    if (!children.empty()) {
        return;
    }
    if (dir != INVALID_DIRECTION) {
        signaler->release(room);
    }
    if (parent != nullptr) {
        parent->removeChild(this);
        parent->deny();
    }
    delete this;
}

void Path::insertChild(Path *p)
{
    children.insert(p);
}

void Path::removeChild(Path *p)
{
    children.erase(p);
}
