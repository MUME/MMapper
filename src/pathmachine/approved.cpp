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

#include "approved.h"
#include "mapaction.h"
#include "room.h"
#include "parseevent.h"
#include "roomadmin.h"
#include "abstractroomfactory.h"

void Approved::receiveRoom(RoomAdmin * sender, const Room * perhaps)
{
  if (matchedRoom == 0)
  {
    ComparisonResult indicator = factory->compare(perhaps, myEvent, matchingTolerance);
    if (indicator != CR_DIFFERENT)
    {
      matchedRoom = perhaps;
      owner = sender;
      if (indicator == CR_TOLERANCE && myEvent->getNumSkipped() == 0)
	update = true;
    }
    else
    {
      sender->releaseRoom(this, perhaps->getId());
    }
  }
  else
  {
    moreThanOne = true;
    sender->releaseRoom(this, perhaps->getId());
  }
}

Approved::~Approved()
{
  if(owner)
  {
    if (moreThanOne)
    {
      owner->releaseRoom(this, matchedRoom->getId());
    }
    else
    {
      owner->keepRoom(this, matchedRoom->getId());
      if (update)
	owner->scheduleAction(new SingleRoomAction(new Update( myEvent), matchedRoom->getId()));
    }
  }
}


Approved::Approved(AbstractRoomFactory * in_factory, ParseEvent * event, int tolerance) :
    matchedRoom(0),
    myEvent(event),
    matchingTolerance(tolerance),
    owner(0),
    moreThanOne(false),
    update(false),
    factory(in_factory)
{}


const Room * Approved::oneMatch()
{
  if (moreThanOne)
    return 0;
  else
    return matchedRoom;
}

void Approved::reset()
{
  if(matchedRoom)
  {
    owner->releaseRoom(this, matchedRoom->getId());
  }
  update = false;
  matchedRoom = 0;
  moreThanOne = false;
  owner = 0;
}



RoomAdmin * Approved::getOwner()
{
  if (moreThanOne)
    return 0;
  else
    return owner;
}
