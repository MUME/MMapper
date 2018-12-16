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

#include "onebyone.h"

#include <list>

#include "../expandoracommon/AbstractRoomFactory.h"
#include "../expandoracommon/parseevent.h"
#include "../global/utils.h"
#include "../parser/CommandId.h"
#include "experimenting.h"
#include "pathparameters.h"
#include "roomsignalhandler.h"

class Path;

OneByOne::OneByOne(AbstractRoomFactory *const in_factory,
                   const SigParseEvent &sigParseEvent,
                   PathParameters &in_params,
                   RoomSignalHandler *const in_handler)
    : Experimenting{new std::list<Path *>,
                    getDirection(sigParseEvent.deref().getMoveType()),
                    in_params,
                    in_factory}
    , event{sigParseEvent.getShared()}
    , handler{in_handler}
{}

void OneByOne::receiveRoom(RoomAdmin *const admin, const Room *const room)
{
    if (factory->compare(room, deref(event), params.matchingTolerance) == ComparisonResult::EQUAL) {
        augmentPath(shortPaths->back(), admin, room);
    } else {
        // needed because the memory address is not unique and
        // calling admin->release might destroy a room still held by some path
        handler->hold(room, admin, this);
        handler->release(room);
    }
}

void OneByOne::addPath(Path *const path)
{
    shortPaths->push_back(path);
}
