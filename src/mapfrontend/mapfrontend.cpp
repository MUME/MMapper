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

#include "mapfrontend.h"
#include "mapaction.h"
#include "roomlocker.h"
#include "frustum.h"
#include "roomrecipient.h"
#include "abstractroomfactory.h"

#include <assert.h>

using namespace Qt;
using namespace std;

MapFrontend::MapFrontend(AbstractRoomFactory * in_factory) :
    greatestUsedId(UINT_MAX),
    mapLock(QMutex::Recursive),
    factory(in_factory)
{}

MapFrontend::~MapFrontend()
{
  QMutexLocker locker(&mapLock);
  emit clearingMap();
  for (uint i = 0; i <= greatestUsedId; ++i)
  {
    if (roomIndex[i])
    {
      delete roomIndex[i];
    }
  }
}

void MapFrontend::block(){
  mapLock.lock();
  blockSignals(true);
}

void MapFrontend::unblock(){
  mapLock.unlock();
  blockSignals(false);
}

void MapFrontend::checkSize() {
  emit mapSizeChanged(ulf, lrb);
}

void MapFrontend::scheduleAction(MapAction * action)
{
  QMutexLocker locker(&mapLock);
  action->schedule(this);

  const set<uint> & affectedRooms = action->getAffectedRooms();
  bool executable = true;
  for (set<uint>::const_iterator i = affectedRooms.begin(); i != affectedRooms.end(); ++i)
  {
    uint roomId = *i;
    actionSchedule[roomId].insert(action);
    if (!locks[roomId].empty()) executable = false;
  }
  if (executable)
  {
    executeAction(action);
    removeAction(action);
  }
}

void MapFrontend::executeAction(MapAction * action)
{
  action->exec();
}

void MapFrontend::removeAction(MapAction * action)
{
  const set<uint> & affectedRooms = action->getAffectedRooms();
  for (set<uint>::const_iterator i = affectedRooms.begin(); i != affectedRooms.end(); ++i)
  {
    uint roomId = *i;
    actionSchedule[roomId].erase(action);
  }
  delete action;
}

bool MapFrontend::isExecutable(MapAction * action)
{
  const set<uint> & affectedRooms = action->getAffectedRooms();
  for (set<uint>::const_iterator i = affectedRooms.begin(); i != affectedRooms.end(); ++i)
  {
    if (!locks[*i].empty())
    {
      return false;
    }
  }
  return true;
}

void MapFrontend::executeActions(uint roomId)
{
  set<MapAction *> & actions = actionSchedule[roomId];
  set<MapAction *>::iterator actionIter = actions.begin();
  while(actionIter!= actions.end())
  {
    MapAction * action = *(actionIter++);
    if (isExecutable(action))
    {
      executeAction(action);
      removeAction(action);
    }
  }
}

void MapFrontend::lookingForRooms(RoomRecipient * recipient, Frustum * frustum)
{
  Room * r = 0;
  QMutexLocker locker(&mapLock);

  for(vector<Room *>::iterator i = roomIndex.begin(); i != roomIndex.end(); ++i)
  {
    r = *i;

    if(r)
    {
      Coordinate rc = r->getPosition();
      if(frustum->PointInFrustum(rc))
      {
        locks[r->getId()].insert(recipient);
        recipient->receiveRoom(this, r);
      }
    }
  }
}

void MapFrontend::lookingForRooms(RoomRecipient * recipient, const Coordinate & pos)
{
  QMutexLocker locker(&mapLock);
  Room * r = map.get(pos);
  if (r)
  {
    locks[r->getId()].insert(recipient);
    recipient->receiveRoom(this, r);
  }
}

void MapFrontend::clear()
{
  QMutexLocker locker(&mapLock);
  emit clearingMap();

  for (uint i = 0; i < roomIndex.size(); ++i)
  {
    if (roomIndex[i])
    {
      delete roomIndex[i];
      roomIndex[i] = 0;
      if (roomHomes[i]) roomHomes[i]->clear();
      roomHomes[i] = 0;
      locks[i].clear();
    }
  }

  map.clear();

  while (!unusedIds.empty()) unusedIds.pop();
  greatestUsedId = UINT_MAX;
  ulf.clear();
  lrb.clear();
  emit mapSizeChanged(ulf, lrb);
}

void MapFrontend::lookingForRooms(RoomRecipient * recipient, uint id)
{
  QMutexLocker locker(&mapLock);
  if (greatestUsedId >= id)
  {
    Room * r = roomIndex[id];
    if (r)
    {
      locks[id].insert(recipient);
      recipient->receiveRoom(this, r);
    }
  }
}

uint MapFrontend::assignId(Room * room, RoomCollection * roomHome)
{
  uint id;

  if (unusedIds.empty()) id = ++greatestUsedId;
  else
  {
    id = unusedIds.top();
    unusedIds.pop();
    if (id > greatestUsedId || greatestUsedId == UINT_MAX) greatestUsedId = id;
  }

  room->setId(id);

  if (roomIndex.size() <= id)
  {
    roomIndex.resize(id*2 + 1, 0);
    locks.resize(id*2 + 1);
    roomHomes.resize(id*2 + 1, 0);
  }
  roomIndex[id] = room;
  roomHomes[id] = roomHome;
  return id;
}

void MapFrontend::lookingForRooms(RoomRecipient * recipient, const Coordinate & ulf, const Coordinate & lrb)
{
  QMutexLocker locker(&mapLock);
  RoomLocker ret(recipient, this);
  map.getRooms(ret,ulf,lrb);
}


void MapFrontend::insertPredefinedRoom(Room * room)
{
  QMutexLocker locker(&mapLock);
  assert(signalsBlocked());
  uint id = room->getId();
  const Coordinate & c = room->getPosition();
  ParseEvent * event = factory->getEvent(room);

  assert (roomIndex.size() <= id || !roomIndex[id]);

  RoomCollection * roomHome = treeRoot.insertRoom(event);
  map.setNearest(c, room);
  checkSize(room->getPosition());
  unusedIds.push(id);
  assignId(room, roomHome);
  if (roomHome) roomHome->addRoom(room);
  delete event;
}

uint MapFrontend::createEmptyRoom(const Coordinate & c)
{
  QMutexLocker locker(&mapLock);
  Room * room = factory->createRoom();
  room->setPermanent();
  map.setNearest(c, room);
  checkSize(room->getPosition());
  uint id = assignId(room, 0);
  return id;
}

void MapFrontend::checkSize(const Coordinate & c)
{
  Coordinate lrbBackup(lrb);
  Coordinate ulfBackup(ulf);

  if (c.x < ulf.x) ulf.x = c.x;
  else if (c.x > lrb.x) lrb.x = c.x;
  if (c.y < ulf.y) ulf.y = c.y;
  else if (c.y > lrb.y) lrb.y = c.y;
  if (c.z < ulf.z) ulf.z = c.z;
  else if (c.z > lrb.z) lrb.z = c.z;

  if (ulf != ulfBackup || lrb != lrbBackup)
    emit mapSizeChanged(ulf, lrb);

}

void MapFrontend::createEmptyRooms(const Coordinate & ulf,const Coordinate & lrb) {
  map.fillArea(factory, ulf, lrb);
}

void MapFrontend::createRoom(ParseEvent * event, const Coordinate & expectedPosition)
{
  QMutexLocker locker(&mapLock);
  checkSize(expectedPosition); // still hackish but somewhat better
  RoomCollection * roomHome = treeRoot.insertRoom(event);

  if (roomHome)
  {
    Room * room = factory->createRoom(event);
    roomHome->addRoom(room);
    map.setNearest(expectedPosition, room);
    assignId(room, roomHome);
  }
  event->reset();
}


void MapFrontend::lookingForRooms(RoomRecipient * recipient, ParseEvent * event)
{
  QMutexLocker locker(&mapLock);
  if (greatestUsedId == UINT_MAX)
  {
    Coordinate c(0,0,0);
    createRoom(event, c);
    if (greatestUsedId != UINT_MAX)
    {
      roomIndex[0]->setPermanent();
    }
  }

  RoomLocker ret(recipient, this, factory, event);

  treeRoot.getRooms(ret, event);
  event->reset();
}

void MapFrontend::lockRoom(RoomRecipient * recipient, uint id)
{
  locks[id].insert(recipient);
}

// removes the lock on a room
// after the last lock is removed, the room is deleted
void MapFrontend::releaseRoom(RoomRecipient * sender, uint id)
{
  assert(sender);
  QMutexLocker lock(&mapLock);
  locks[id].erase(sender);
  if(locks[id].empty())
  {
    executeActions(id);
    Room * room = roomIndex[id];
    if (room && room->isTemporary())
    {
      MapAction * r = new SingleRoomAction(new Remove, id);
      scheduleAction(r);
    }
  }
}

// makes a lock on a room permanent and anonymous.
// Like that the room can't be deleted via releaseRoom anymore.
void MapFrontend::keepRoom(RoomRecipient * sender, uint id)
{
  assert(sender);
  QMutexLocker lock(&mapLock);
  locks[id].erase(sender);
  MapAction * mp = new SingleRoomAction(new MakePermanent, id);
  scheduleAction(mp);
  if (locks[id].empty()) executeActions(id);
}
