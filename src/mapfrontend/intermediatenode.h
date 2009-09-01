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

#ifndef INTERMEDIATENODE
#define INTERMEDIATENODE

#include "searchtreenode.h"
#include "roomcollection.h"
#include "roomoutstream.h"


/**
 * IntermediateNodes represent possible ends of a property
 * they hold a RoomSearchNode if this property can be the last one
 */
class IntermediateNode : public SearchTreeNode {
 public:
  virtual ~IntermediateNode() {delete rooms;}
  IntermediateNode() : SearchTreeNode((char*)"") {rooms = 0;}
  IntermediateNode(ParseEvent * event);
  RoomCollection * insertRoom(ParseEvent * event); 
  void getRooms(RoomOutStream & stream, ParseEvent * event);
  void skipDown(RoomOutStream & stream, ParseEvent * event);
 private:
  RoomCollection * rooms;
};

#endif

#ifdef DMALLOC
#include <mpatrol.h>
#endif
