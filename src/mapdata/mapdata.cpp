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

#include "mapdata.h"
#include "roomfactory.h"
#include "drawstream.h"
#include "mmapper2room.h"
#include "customaction.h"
#include "roomselection.h"
#include "infomark.h"

#include <assert.h>

using namespace std;

MapData::MapData() :
    MapFrontend(new RoomFactory),
    m_dataChanged(false)
{}

QString MapData::getDoorName(const Coordinate & pos, uint dir)
{
  QMutexLocker locker(&mapLock);
  Room * room = map.get(pos);
  if (room && dir < 7)
  {
    return ::getDoorName(room->exit(dir));
  }
  else return "exit";
}

void MapData::setDoorName(const Coordinate & pos, const QString & name, uint dir)
{
  QMutexLocker locker(&mapLock);
  Room * room = map.get(pos);
  if (room && dir < 7)
  {
    /*
    // Is the Door there? If not, add it.
    if (!getExitFlag(pos, dir, EF_DOOR)) {
      MapAction * action_setdoor = new SingleRoomAction(new ModifyExitFlags(EF_DOOR, dir, E_FLAGS, FMM_SET), room->getId());
      scheduleAction(action_setdoor);
    }
    */
    setDataChanged();
    MapAction * action = new SingleRoomAction(new UpdateExitField(name, dir, E_DOORNAME), room->getId());
    scheduleAction(action);
  }
}

bool MapData::getExitFlag(const Coordinate & pos, uint flag, uint dir, uint field)
{
  QMutexLocker locker(&mapLock);
  Room * room = map.get(pos);
  if (room && dir < 7)
  {
    //ExitFlags ef = ::getFlags(room->exit(dir));
    ExitFlags ef = room->exit(dir)[field].toUInt();
    if (ISSET(ef, flag)) return true;
  }
  return false;
}

void MapData::toggleExitFlag(const Coordinate & pos, uint flag, uint dir, uint field)
{
  QMutexLocker locker(&mapLock);
  Room * room = map.get(pos);
  if (room && dir < 7)
  {
    setDataChanged();
    MapAction * action = new SingleRoomAction(new ModifyExitFlags(flag, dir, field, FMM_TOGGLE), room->getId());
    scheduleAction(action);
  }
}

void MapData::toggleRoomFlag(const Coordinate & pos, uint flag, uint field)
{
  QMutexLocker locker(&mapLock);
  Room * room = map.get(pos);
  if (room && field < ROOMFIELD_LAST )
  {
    setDataChanged();
    MapAction * action = new SingleRoomAction(new ModifyRoomFlags(flag, field, FMM_TOGGLE), room->getId());
    scheduleAction(action);
  }
}

bool MapData::getRoomFlag(const Coordinate & pos, uint flag, uint field)
{
  QMutexLocker locker(&mapLock);
  Room * room = map.get(pos);
  if (room && field < ROOMFIELD_LAST )
  {
    if (ISSET((*room)[field].toUInt(), flag)) return true;
  }
  return false;
}

void MapData::setRoomField(const Coordinate & pos, uint flag, uint field)
{
  QMutexLocker locker(&mapLock);
  Room * room = map.get(pos);
  if (room && field < ROOMFIELD_LAST )
  {
    setDataChanged();
    MapAction * action = new SingleRoomAction(new UpdateRoomField(flag, field), room->getId());
    scheduleAction(action);
  }
}

uint MapData::getRoomField(const Coordinate & pos, uint field)
{
  QMutexLocker locker(&mapLock);
  Room * room = map.get(pos);
  if (room && field < ROOMFIELD_LAST )
  {
    return (*room)[field].toUInt();
  }
  return 0;
}

QList<Coordinate> MapData::getPath(const QList<CommandIdType> dirs)
{
  QMutexLocker locker(&mapLock);
  QList<Coordinate> ret;
  Room * room = map.get(m_position);
  if (room)
  {
    QListIterator<CommandIdType> iter(dirs);
    while (iter.hasNext())
    {
      uint dir = iter.next();
      if (dir > 5) break;
      Exit & e = room->exit(dir);
      if (!(getFlags(e) & EF_EXIT)) continue;
      if (e.outBegin() == e.outEnd() || ++e.outBegin() != e.outEnd()) break;
      room = roomIndex[*e.outBegin()];
      if (!room) break;
      ret.append(room->getPosition());
    }
  }
  return ret;
}

const RoomSelection * MapData::select(const Coordinate & ulf, const Coordinate & lrb)
{
  QMutexLocker locker(&mapLock);
  RoomSelection * selection = new RoomSelection(this);
  selections[selection] = selection;
  lookingForRooms(selection, ulf, lrb);
  return selection;
}
// updates a selection created by the mapdata
const RoomSelection * MapData::select(const Coordinate & ulf, const Coordinate & lrb, const RoomSelection * in)
{
  QMutexLocker locker(&mapLock);
  RoomSelection * selection = selections[in];
  assert(selection);
  lookingForRooms(selection, ulf, lrb);
  return selection;
}
// creates and registers a selection with one room
const RoomSelection * MapData::select(const Coordinate & pos)
{
  QMutexLocker locker(&mapLock);
  RoomSelection * selection = new RoomSelection(this);
  selections[selection] = selection;
  lookingForRooms(selection, pos);
  return selection;
}
// creates and registers an empty selection
const RoomSelection * MapData::select()
{
  QMutexLocker locker(&mapLock);
  RoomSelection * selection = new RoomSelection(this);
  selections[selection] = selection;
  return selection;
}
// removes the selection from the internal structures and deletes it
void MapData::unselect(const RoomSelection * in)
{
  QMutexLocker locker(&mapLock);
  RoomSelection * selection = selections[in];
  assert(selection);
  selections.erase(in);
  QMap<uint, const Room *>::iterator i =selection->begin();
  while(i != selection->end())
  {
    keepRoom(selection, (i++).key());
  }
  delete selection;
}

// the room will be inserted in the given selection. the selection must have been created by mapdata
const Room * MapData::getRoom(const Coordinate & pos, const RoomSelection * in)
{
  QMutexLocker locker(&mapLock);
  RoomSelection * selection = selections[in];
  assert(selection);
  Room * room = map.get(pos);
  if (room)
  {
    uint id = room->getId();
    locks[id].insert(selection);
    selection->insert(id, room);
  }
  return room;
}

const Room * MapData::getRoom(uint id, const RoomSelection * in)
{
  QMutexLocker locker(&mapLock);
  RoomSelection * selection = selections[in];
  assert(selection);
  Room * room = roomIndex[id];
  if (room)
  {
    uint id = room->getId();
    locks[id].insert(selection);
    selection->insert(id, room);
  }
  return room;
}

void MapData::draw (const Coordinate & ulf, const Coordinate & lrb, MapCanvas & screen)
{
  QMutexLocker locker(&mapLock);
  DrawStream drawer(screen, roomIndex, locks);
  map.getRooms(drawer, ulf, lrb);
}

bool MapData::isOccupied(const Coordinate & position)
{
  QMutexLocker locker(&mapLock);
  return map.get(position);
}

bool MapData::isMovable(const Coordinate & offset, const RoomSelection * selection)
{
  QMutexLocker locker(&mapLock);
  QMap<uint, const Room *>::const_iterator i = selection->begin();
  const Room * other = 0;
  while(i != selection->end())
  {
    Coordinate target = (*(i++))->getPosition() + offset;
    other = map.get(target);
    if (other && !selection->contains(other->getId())) return false;
  }
  return true;
}

void MapData::unselect(uint id, const RoomSelection * in)
{
  QMutexLocker locker(&mapLock);
  RoomSelection * selection = selections[in];
  assert(selection);
  keepRoom(selection, id);
  selection->remove(id);
}

// selects the rooms given in "other" for "into"
const RoomSelection * MapData::select(const RoomSelection * other, const RoomSelection * in)
{
  QMutexLocker locker(&mapLock);
  RoomSelection * into = selections[in];
  assert(into);
  QMapIterator<uint, const Room *> i(*other);
  while(i.hasNext())
  {
    locks[i.key()].insert(into);
    into->insert(i.key(), i.value());
  }
  return into;
}

// removes the subset from the superset and unselects
void MapData::unselect(const RoomSelection * subset, const RoomSelection * in)
{
  QMutexLocker locker(&mapLock);
  RoomSelection * superset = selections[in];
  assert(superset);
  QMapIterator<uint, const Room *> i(*subset);
  while(i.hasNext())
  {
    keepRoom(superset, i.key());
    superset->remove(i.key());
  }
}

bool MapData::execute(MapAction * action)
{
  QMutexLocker locker(&mapLock);
  action->schedule(this);
  bool executable = isExecutable(action);
  if (executable) executeAction(action);
  return executable;
}

bool MapData::execute(MapAction * action, const RoomSelection * unlock)
{
  QMutexLocker locker(&mapLock);
  action->schedule(this);
  list<uint> selectedIds;

  RoomSelection * selection = selections[unlock];
  assert(selection);

  QMap<uint, const Room *>::iterator i = selection->begin();
  while(i != selection->end())
  {
    const Room * room = *(i++);
    uint id = room->getId();
    locks[id].erase(selection);
    selectedIds.push_back(id);
  }
  selection->clear();

  bool executable = isExecutable(action);
  if (executable) executeAction(action);

  for(list<uint>::const_iterator i =  selectedIds.begin(); i != selectedIds.end(); ++i)
  {
    uint id = (*i);
    Room * room = roomIndex[id];
    if (room)
    {
      locks[id].insert(selection);
      selection->insert(id, room);
    }
  }
  delete action;
  return executable;
}


void MapData::clear()
{
  MapFrontend::clear();
  while (!m_markers.isEmpty())
    delete m_markers.takeFirst();
  emit log("MapData", "cleared MapData");
}

void MapData::removeDoorNames()
{
  QMutexLocker locker(&mapLock);

  Room * r = 0;

  for(vector<Room *>::iterator i = roomIndex.begin(); i != roomIndex.end(); ++i)  
  {
    r = *i;
    if (r)
    {
      for (uint dir = 0; dir <= 6; dir++)
      {
        MapAction * action = new SingleRoomAction(new UpdateExitField("", dir, E_DOORNAME), r->getId());
        scheduleAction(action);
      }
    }
  setDataChanged();
  }
}

void MapData::searchDescriptions(RoomRecipient * recipient, QString s, Qt::CaseSensitivity cs)
{
  QMutexLocker locker(&mapLock);
  Room * r = 0;

  for(vector<Room *>::iterator i = roomIndex.begin(); i != roomIndex.end(); ++i)
  {
    r = *i;
    if (r) {
      if (QString((*r)[1].toString()).contains(s, cs))
      {
        locks[r->getId()].insert(recipient);
        recipient->receiveRoom(this, r);
      }
    }
  }
}

void MapData::searchNames(RoomRecipient * recipient, QString s, Qt::CaseSensitivity cs)
{
  QMutexLocker locker(&mapLock);
  Room * r = 0;

  for(vector<Room *>::iterator i = roomIndex.begin(); i != roomIndex.end(); ++i)
  {
    r = *i;
    if (r) {
      if (QString((*r)[0].toString()).contains(s, cs))
      {
        locks[r->getId()].insert(recipient);
        recipient->receiveRoom(this, r);
      }
    }
  }
}

void MapData::searchDoorNames(RoomRecipient * recipient, QString s, Qt::CaseSensitivity cs)
{
  QMutexLocker locker(&mapLock);
  Room * r = 0;

  for(vector<Room *>::iterator i = roomIndex.begin(); i != roomIndex.end(); ++i)
  {
    r = *i;
    if (r) {
      ExitsList exits = r->getExitsList();
      for(ExitsList::const_iterator exitIter = exits.begin(); exitIter != exits.end(); ++exitIter)
      {
        const Exit & e = *exitIter;
        if (QString((e)[0].toString()).contains(s, cs))
        {
          locks[r->getId()].insert(recipient);
          recipient->receiveRoom(this, r);
          break;
        }
      }
    }
  }
}

void MapData::searchNotes(RoomRecipient * recipient, QString s, Qt::CaseSensitivity cs)
{
  QMutexLocker locker(&mapLock);
  Room * r = 0;

  for(vector<Room *>::iterator i = roomIndex.begin(); i != roomIndex.end(); ++i)
  {
    r = *i;
    if (r) {
      if (QString((*r)[4].toString()).contains(s, cs))
      {
        locks[r->getId()].insert(recipient);
        recipient->receiveRoom(this, r);
      }
    }
  }
}

MapData::~MapData()
{
  while (!m_markers.isEmpty())
    delete m_markers.takeFirst();
}
bool MapData::execute(AbstractAction* action, const RoomSelection* unlock) {
    return execute(new GroupAction(action, unlock), unlock);
}
void MapData::removeMarker(InfoMark* im) {
    m_markers.removeAll(im);
    delete im;
}
void MapData::addMarker(InfoMark* im) {
    m_markers.append(im);
}

