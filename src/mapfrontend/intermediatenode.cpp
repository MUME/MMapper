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

#include "intermediatenode.h"
#include "property.h"
#include "parseevent.h"

using namespace std;

IntermediateNode::IntermediateNode(ParseEvent * event)
{
  Property * prop = event->next();

  if (prop == 0 || prop->isSkipped())
  {
    myChars = new char[1];
    myChars[0] = 0;
  }
  else
  {
    myChars = new char[strlen(prop->rest()) + 1];
    strcpy(myChars, prop->rest());
  }
  rooms = 0;
  event->prev();
}

RoomCollection * IntermediateNode::insertRoom(ParseEvent * event)
{

  if (event->next() == 0)
  {
    if (rooms == 0) rooms = new RoomCollection;
    return rooms;
  }
  else if (event->current()->isSkipped()) return 0;

  return SearchTreeNode::insertRoom(event);
}


void IntermediateNode::getRooms(RoomOutStream & stream, ParseEvent * event)
{
  if (event->next() == 0)
  {
    for (set<Room *>::iterator i = rooms->begin(); i != rooms->end(); ++i)
      stream << *i;
  }
  else if (event->current()->isSkipped()) 
    SearchTreeNode::skipDown(stream, event);
  else
  {
    SearchTreeNode::getRooms(stream, event);
  }
}

void IntermediateNode::skipDown(RoomOutStream & stream, ParseEvent * event)
{
  getRooms(stream, event);
}
