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
#include "parseevent.h"
#include "abstractroomfactory.h"
#include "pathparameters.h"
#include "roomsignalhandler.h"

using namespace std;

OneByOne::OneByOne(AbstractRoomFactory * in_factory, ParseEvent * in_event, 
                   PathParameters & in_params, RoomSignalHandler * in_handler) : 
    Experimenting(new list<Path*>, in_event->getMoveType(), in_params, in_factory), event(in_event), handler(in_handler)
{}

void OneByOne::receiveRoom(RoomAdmin * admin, const Room * room)
{
  if (factory->compare(room, event, params.matchingTolerance) == CR_EQUAL)
    augmentPath(shortPaths->back(), admin, room);
  else { 
    // needed because the memory address is not unique and 
    // calling admin->release might destroy a room still held by some path
    handler->hold(room, admin, this);
    handler->release(room);
  }
}

void OneByOne::addPath(Path * path)
{
  shortPaths->push_back(path);
}
