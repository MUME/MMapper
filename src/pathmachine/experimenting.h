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

#ifndef EXPERIMENTING_H
#define EXPERIMENTING_H

//#include "room.h"
//#include "path.h"
//#include "roomadmin.h"
//#include "pathparameters.h"

#include "coordinate.h"
#include "roomrecipient.h"
#include <QtGlobal>
#include <list>

class PathMachine;
class PathParameters;
class Path;
class AbstractRoomFactory;
class Room;
class RoomAdmin;

class Experimenting : public RoomRecipient {
 protected:
  void augmentPath(Path * path, RoomAdmin * map, const Room * room);
  Coordinate direction;
  uint dirCode;
  std::list<Path *> * shortPaths;
  std::list<Path *> * paths;
  Path * best;
  Path * second;
  PathParameters & params;
  double numPaths;
  AbstractRoomFactory * factory;

 public:
  Experimenting(std::list<Path *> * paths, uint dirCode, PathParameters & params, AbstractRoomFactory * factory);
  std::list<Path *> * evaluate();
  virtual void receiveRoom(RoomAdmin *, const Room *) = 0;
};

#endif

