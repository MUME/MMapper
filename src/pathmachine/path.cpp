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
#include "roomsignalhandler.h"
#include "pathparameters.h"
#include "abstractroomfactory.h"
#include "coordinate.h"
#include "room.h"
#include "parseevent.h"

#include <iostream>
#include <assert.h>

using namespace std;

Path::Path(const Room * in_room, RoomAdmin * owner, RoomRecipient * locker, RoomSignalHandler * in_signaler, uint direction) :
    parent(0),
    probability(1.0),
    room(in_room),
    signaler(in_signaler),
    dir(direction)
{
  if (dir != UINT_MAX) signaler->hold(room, owner, locker);
}


/**
 * new Path is created, 
 * distance between rooms is calculated 
 * and probability is updated accordingly
 */
Path * Path::fork(const Room * in_room, Coordinate & expectedCoordinate, RoomAdmin * owner, PathParameters p, RoomRecipient * locker, uint direction, AbstractRoomFactory * factory)
{
  Path * ret = new Path(in_room, owner, locker, signaler, direction);
  assert(ret != parent);

  ret->setParent(this);
  children.insert(ret);

  double dist = expectedCoordinate.distance(in_room->getPosition());
  uint size = room->getExitsList().size();

  if (dist < 0.5)
  {
    if (direction < factory->numKnownDirs())
      dist = 1.0/p.correctPositionBonus;
    else
      dist = p.multipleConnectionsPenalty;
  }
  else
  {
    if (direction < size)
    {
      const Exit & e = room->exit(direction);
      uint oid = in_room->getId();
      if (e.containsOut(oid))
        dist = 1.0/p.correctPositionBonus;
      else if (e.outBegin() != e.outEnd() || oid == room->getId())
        dist *= p.multipleConnectionsPenalty;
      else
      {
        const Exit & oe = in_room->exit(factory->opposite(direction));
        if (oe.inBegin() != oe.inEnd())
          dist *= p.multipleConnectionsPenalty;
      }
    }
    else if (direction < factory->numKnownDirs())
    {
      for (uint d = 0; d < size; ++d)
      {
        const Exit & e = room->exit(d);
        if (e.containsOut(in_room->getId()))
        {
          dist = 1.0/p.correctPositionBonus;
          break;
        }
      }
    }
  }
  dist /= (signaler->getNumLockers(in_room));
  if (in_room->isTemporary()) dist *= p.newRoomPenalty;
  ret->setProb(probability / dist);

  return ret;
}

void Path::setParent(Path * p)
{
  parent = p;
}

void Path::approve()
{
  if (parent)
  {
    uint pId = UINT_MAX;
    const Room * proom = parent->getRoom();
    if (proom)
    {
      pId = proom->getId();
    }
    signaler->keep(room, dir, pId);
    parent->removeChild(this);
    parent->approve();
  }
  else
  {
    assert(dir == UINT_MAX);
  }

  set<Path *>::iterator i = children.begin();
  for(; i != children.end(); ++i)
  {
    (*i)->setParent(0);
  }

  delete this;
}



/** removes this path and all parents up to the next branch
 * and removes the respective rooms if experimental
 */
void Path::deny()
{
  if (!children.empty()) return;
  if (dir != UINT_MAX) signaler->release(room);
  if (parent)
  {
    parent->removeChild(this);
    parent->deny();
  }
  delete this;
}



void Path::insertChild(Path * p)
{
  children.insert(p);
}


void Path::removeChild(Path * p)
{
  children.erase(p);
}

