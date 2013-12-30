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

#include "roomfactory.h"
#include "mmapper2exit.h"
#include "mmapper2room.h"
#include "mmapper2event.h"
#include "coordinate.h"
#include "room.h"

#include <QStringList>
#include <QRegExp>

Room * RoomFactory::createRoom(const ParseEvent * ev) const
{
  Room * room = new Room(11, 7, 3);
  if (ev) update(room, ev);
  return room;
}


ParseEvent * RoomFactory::getEvent(const Room * room) const
{
  ExitsFlagsType exitFlags = 0;
  for(uint dir = 0; dir < 6; ++dir)
  {
    const Exit & e = room->exit(dir);

    ExitFlags eFlags = getFlags(e);
    exitFlags |= (eFlags << (dir*3));
  }

  exitFlags |= EXITS_FLAGS_VALID;
  PromptFlagsType promptFlags = getTerrainType(room);
  promptFlags |= PROMPT_FLAGS_VALID;

  CommandIdType c = CID_UNKNOWN;

  ParseEvent * event = createEvent(c, getName(room), getDynamicDescription(room) , getDescription(room), exitFlags, promptFlags);
  return event;
}

ComparisonResult RoomFactory::compareStrings(const QString & room, const QString & event, uint prevTolerance, bool updated) const
{
  prevTolerance *= room.size();
  prevTolerance /= 100;
  int tolerance = prevTolerance;

  
  QStringList descWords = room.split(QRegExp("\\s+"), QString::SkipEmptyParts);
  QStringList eventWords = event.split(QRegExp("\\s+"), QString::SkipEmptyParts);

  if (!eventWords.isEmpty()) // if event is empty we don't compare
  {
    while (tolerance >= 0)
    {
      if (descWords.isEmpty())
      {
        if (updated) // if notUpdated the desc is allowed to be shorter than the event
          tolerance -= eventWords.join(" ").size();
        break;
      }
      else if (eventWords.isEmpty()) // if we get here the event isn't empty
      {
        tolerance -= descWords.join(" ").size();
        break;
      }

      QString eventWord = eventWords.takeFirst();
      QString descWord = descWords.takeFirst();
      int descSize = descWord.size();
      int eventSize = eventWord.size();
      if (descSize > eventSize)
      {
        tolerance -= (descSize - eventSize);
      }
      else
      {
        tolerance -= (eventSize - descSize);
        eventSize = descSize;
      }
      for (; eventSize >= 0; --eventSize)
        if (eventWord[eventSize] != descWord[eventSize])
          --tolerance;
    }
  }

  if (tolerance < 0)
    return CR_DIFFERENT;
  else if ((int)prevTolerance != tolerance) 
    return CR_TOLERANCE;
  else if (event.size() != room.size()) // differences in amount of whitespace
    return CR_TOLERANCE;
  else 
    return CR_EQUAL;

}

ComparisonResult RoomFactory::compare(const Room * room, const ParseEvent * event, uint tolerance) const
{
  QString m_name = getName(room);
  QString m_desc = getDescription(room);
  RoomTerrainType m_terrainType = getTerrainType(room);
  bool updated = room->isUpToDate();

  if (event == NULL) 
  {
  	//return CR_DIFFERENT;
    return CR_EQUAL;
  }

  if (tolerance >= 100 || (m_name.isEmpty() && m_desc.isEmpty() && (!updated)))
  {
    // user-created oder explicit update
    return CR_TOLERANCE;
  }

  PromptFlagsType pf = getPromptFlags(event);
  if (pf & PROMPT_FLAGS_VALID)
  {
    if ((pf & (bit1 + bit2 + bit3 + bit4)) != m_terrainType)
    {
      if (room->isUpToDate())
        return CR_DIFFERENT;
    }
  }

  switch (compareStrings(m_name, getRoomName(event), tolerance))
  {
  case CR_TOLERANCE: updated = false; break;
  case CR_DIFFERENT: return CR_DIFFERENT;
  case CR_EQUAL: break;
  }

  switch (compareStrings(m_desc, getParsedRoomDesc(event), tolerance, updated))
  {
  case CR_TOLERANCE: updated = false; break;
  case CR_DIFFERENT: return CR_DIFFERENT;
  case CR_EQUAL: break;
  }

  switch (compareWeakProps(room, event))
  {
  case CR_DIFFERENT: return CR_DIFFERENT;
  case CR_TOLERANCE: updated = false; break;
  case CR_EQUAL: break;
  }

  if (updated)
    return CR_EQUAL;
  else
    return CR_TOLERANCE;
}

ComparisonResult RoomFactory::compareWeakProps(const Room * room, const ParseEvent * event, uint) const
{
  bool exitsValid = room->isUpToDate();
  bool different = false;
  bool tolerance = false;
  ExitsFlagsType eFlags = getExitFlags(event);
  if (eFlags & EXITS_FLAGS_VALID)
  {
    for (uint dir = 0; dir < 6; ++dir)
    {
      const Exit & e = room->exit(dir);
      ExitFlags mFlags = getFlags(e);
      if (mFlags)
      { // exits are considered valid as soon as one exit is found (or if the room is updated
        exitsValid = true;
        if (different) return CR_DIFFERENT;
      }
      if (mFlags & EF_NO_MATCH) continue;
      else
      {
        ExitsFlagsType eThisExit = eFlags >> (dir * 4);
        ExitsFlagsType diff = (eThisExit ^ mFlags);
        if (diff & (EF_EXIT | EF_DOOR))
        {
          if (exitsValid)
          {
            if (!tolerance)
            {
              if (!(mFlags & EF_EXIT) && (eThisExit & EF_DOOR)) // We have no exit on record and there is a secret door
		tolerance = true;
	      else if (mFlags & EF_DOOR) // We have a secret door on record
		tolerance = true;
              else return CR_DIFFERENT;
            }
            else return CR_DIFFERENT;
          }
          else different = true;
        }
	else if (diff & EF_ROAD)
	  tolerance = true;
	else if (diff & EF_CLIMB)
	  tolerance = true;
      }
    }
  }
  if (tolerance || !exitsValid) return CR_TOLERANCE;
  else return CR_EQUAL;
}

void RoomFactory::update(Room * room, const ParseEvent * event) const
{
  (*room)[R_DYNAMICDESC] = getRoomDesc(event);

  ExitsFlagsType eFlags = getExitFlags(event);
  PromptFlagsType rt = getPromptFlags(event);
  if (eFlags & EXITS_FLAGS_VALID)
  {
    eFlags ^= EXITS_FLAGS_VALID;
    if (!room->isUpToDate())
    {
      for (int dir = 0; dir < 6; ++dir)
      {

        ExitFlags mFlags = (eFlags >> (dir * 4) & (EF_EXIT | EF_DOOR | EF_ROAD | EF_CLIMB));

        Exit & e = room->exit(dir);
        if (getFlags(e) & EF_DOOR && !(mFlags & EF_DOOR)) // We have a secret door on record
        {
          mFlags |= EF_DOOR;
          mFlags |= EF_EXIT;
        }
        if ((mFlags & EF_DOOR) && (mFlags & EF_ROAD)) // TODO: Door and road is confusing??
        {
          mFlags |= EF_NO_MATCH;
        }
        e[E_FLAGS] = mFlags;
      }
    }
    else
    {
      for (int dir = 0; dir < 6; ++dir)
      {
        Exit & e = room->exit(dir);
        ExitFlags mFlags = (eFlags >> (dir * 4) & (EF_EXIT | EF_DOOR | EF_ROAD | EF_CLIMB));
        updateExit(e, mFlags);
      }
    }
    room->setUpToDate();
  }
  else
    room->setOutDated();

  if (rt & PROMPT_FLAGS_VALID)
  {
    rt &= (bit1 + bit2 + bit3 + bit4);
    (*room)[R_TERRAINTYPE] = (RoomTerrainType)rt;
    if (rt & SUN_ROOM) {
      (*room)[R_LIGHTTYPE] = RLT_LIT;
    }
  }
  else
  {
    room->setOutDated();
  }

  QString desc = getParsedRoomDesc(event);
  if (!desc.isEmpty()) room->replace(R_DESC, desc);
  else room->setOutDated();

  QString name = getRoomName(event);
  if (!name.isEmpty()) room->replace(R_NAME, name);
  else room->setOutDated();
}


void RoomFactory::update(Room * target, const Room * source) const
{
  QString name = getName(source);
  if (!name.isEmpty()) target->replace(R_NAME, name);
  QString desc = getDescription(source);
  if (!desc.isEmpty()) target->replace(R_DESC, desc);
  QString dynamic = getDynamicDescription(source);
  if (!dynamic.isEmpty()) target->replace(R_DYNAMICDESC, dynamic);

  if (getAlignType(target) == RAT_UNDEFINED) target->replace(R_ALIGNTYPE, getAlignType(source));
  if (getLightType(target) == RLT_UNDEFINED) target->replace(R_LIGHTTYPE, getLightType(source));
  if (getPortableType(target) == RPT_UNDEFINED) target->replace(R_PORTABLETYPE, getPortableType(source));
  if (getRidableType(target) == RRT_UNDEFINED) target->replace(R_RIDABLETYPE, getRidableType(source));
  if (getTerrainType(source) != RTT_UNDEFINED) target->replace(R_TERRAINTYPE, getTerrainType(source));

  target->replace(R_NOTE, getNote(target).append(getNote(source)));

  target->replace(R_MOBFLAGS, getMobFlags(target) | getMobFlags(source));
  target->replace(R_LOADFLAGS, getLoadFlags(target) | getLoadFlags(source));

  if (!target->isUpToDate())
  {
    for (int dir = 0; dir < 6; ++dir)
    {
      const Exit & o = source->exit(dir);
      Exit & e = target->exit(dir);
      ExitFlags mFlags = getFlags(o);
      if (getFlags(e) & EF_DOOR && !(mFlags & EF_DOOR))
      {
        mFlags |= EF_NO_MATCH;
        mFlags |= EF_DOOR;
        mFlags |= EF_EXIT;
      }
      else
      {
        e[E_DOORNAME] = o[E_DOORNAME];
        e[E_DOORFLAGS] = o[E_DOORFLAGS];
      }
      if ((mFlags & EF_DOOR) && (mFlags & EF_ROAD))
      {
        mFlags |= EF_NO_MATCH;
      }
      e[E_FLAGS] = mFlags;
    }
  }
  else
  {
    for (int dir = 0; dir < 6; ++dir)
    {
      Exit & e = target->exit(dir);
      const Exit & o = source->exit(dir);
      ExitFlags mFlags = getFlags(o);
      ExitFlags eFlags = getFlags(e);
      if (eFlags != mFlags)
      {
        e[E_FLAGS] = (eFlags | mFlags) | EF_NO_MATCH;
      }
      const QString & door = getDoorName(o);
      if (!door.isEmpty())
        e[E_DOORNAME] = door;
      DoorFlags doorFlags = getDoorFlags(o);
      doorFlags |= getDoorFlags(e);
      e[E_DOORFLAGS] = doorFlags;
    }
  }
  if (source->isUpToDate()) target->setUpToDate();
}


uint RoomFactory::opposite(uint in) const
{
  switch (in)
  {
  case ED_NORTH: return ED_SOUTH;
  case ED_SOUTH: return ED_NORTH;
  case ED_WEST: return ED_EAST;
  case ED_EAST: return ED_WEST;
  case ED_UP: return ED_DOWN;
  case ED_DOWN: return ED_UP;
  default: return ED_UNKNOWN;
  }
}


const Coordinate & RoomFactory::exitDir(uint dir) const
{
  return exitDirs[dir];
}

RoomFactory::RoomFactory() : exitDirs(64)
{
  Coordinate north(0,-1,0);
  exitDirs[CID_NORTH] = north;
  Coordinate south(0,1,0);
  exitDirs[CID_SOUTH] = south;
  Coordinate west(-1,0,0);
  exitDirs[CID_WEST] = west;
  Coordinate east(1,0,0);
  exitDirs[CID_EAST] = east;
  Coordinate up(0,0,1);
  exitDirs[CID_UP] = up;
  Coordinate down(0,0,-1);
  exitDirs[CID_DOWN] = down;
  // rest is default constructed which means (0,0,0)
}


