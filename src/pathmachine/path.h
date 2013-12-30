/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#ifndef PATH
#define PATH

#include <QtGlobal>
#include <set>
#include <limits.h>

//#include "roomsignalhandler.h"
//#include "room.h"
//#include "roomrecipient.h"
//#include "roomadmin.h"
//#include "pathparameters.h"
//#include "abstractroomfactory.h"

class Room;
class RoomSignalHandler;
class RoomRecipient;
class RoomAdmin;
class Coordinate;
class PathParameters;
class AbstractRoomFactory;

class Path
{
public:
  Path(const Room * room, RoomAdmin * owner, RoomRecipient * locker, RoomSignalHandler * signaler, uint direction = UINT_MAX);
  void insertChild(Path * p);
  void removeChild(Path * p);
  void setParent(Path * p);
  bool hasChildren() const {return (!children.empty());}
  const Room * getRoom() const {return room;}

  //new Path is created, distance between rooms is calculated and probability is set accordingly
  Path * fork(const Room * room, Coordinate & expectedCoordinate, 
              RoomAdmin * owner, PathParameters params, RoomRecipient * locker, 
              uint dir, AbstractRoomFactory * factory);
  double getProb() const {return probability;}
  void approve();

  // deletes this path and all parents up to the next branch
  void deny();
  void setProb(double p) {probability = p;}

  Path * getParent() const {return parent;}

private:
  Path * parent;
  std::set<Path *> children;
  double probability;
  const Room * room; // in fact a path only has one room, one parent and some children (forks).
  RoomSignalHandler * signaler;
  uint dir;
  ~Path() {}
};

#endif

#ifdef DMALLOC
#include <mpatrol.h>
#endif
