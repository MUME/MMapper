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

#include "mapdata.h"
#include "pathmachine.h"
#include "parser.h"

const char * q2c(QString & s) {
  return s.toLatin1().constData();
}

void init() {

/*  Coordinate north(0,1,0);
  Coordinate::stdMoves[CID_NORTH] = north;
  Coordinate south(0,-1,0);
  Coordinate::stdMoves[CID_SOUTH] = south;
  Coordinate west(-1,0,0);
  Coordinate::stdMoves[CID_WEST] = west;
  Coordinate east(1,0,0);
  Coordinate::stdMoves[CID_EAST] = east;
  Coordinate up(0,0,1);
  Coordinate::stdMoves[CID_UP] = up;
  Coordinate down(0,0,-1);
  Coordinate::stdMoves[CID_DOWN] = down;
  Coordinate none(0,0,0);
  Coordinate::stdMoves[CID_LOOK] = none;
  Coordinate::stdMoves[CID_EXAMINE] = none; 
  Coordinate::stdMoves[CID_FLEE] = none; 
  Coordinate::stdMoves[CID_UNKNOWN] = none; 
  Coordinate::stdMoves[CID_NULL] = none; */

  PathMachine * machine = new PathMachine(false);
  MapData * data = new MapData;
  QObject::connect(machine, SIGNAL(addExit(int, int, uint )), data, SLOT(addExit(int, int, uint )));
  QObject::connect(machine, SIGNAL(createRoom(ParseEvent*, Coordinate )), data, SLOT(createRoom(ParseEvent*, Coordinate )));
  QObject::connect(machine, SIGNAL(lookingForRooms(RoomRecipient*, Coordinate )), data, SLOT(lookingForRooms(RoomRecipient*, Coordinate )));
  QObject::connect(machine, SIGNAL(lookingForRooms(RoomRecipient*, ParseEvent* )), data, SLOT(lookingForRooms(RoomRecipient*, ParseEvent* )));
  QObject::connect(machine, SIGNAL(lookingForRooms(RoomRecipient*, uint )), data, SLOT(lookingForRooms(RoomRecipient*, uint )));
  QObject::connect(data, SIGNAL(clearingMap()), machine, SLOT(releaseAllPaths()));
  //QObject::connect(..., machine, SLOT(deleteMostLikelyRoom()));
  //QObject::connect(machine, SIGNAL(playerMoved(Coordinate, Coordinate )), ....);
  //QObject::connect(... , machine, SLOT(event(ParseEvent* )));
  //QObject::connect(... , data, SLOT(insertPredefinedRoom(ParseEvent*, Coordinate, int )));
  //QObject::connect(... , data, SLOT(lookingForRooms(RoomRecipient*, Coordinate, Coordinate )));
  //...
  machine->start();
  data->start();
}
