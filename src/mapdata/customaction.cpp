/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper2 project. 
** Maintained by Marek Krejza <krejza@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file COPYING included in the packaging of
** this file.  
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
*************************************************************************/

#include "customaction.h"
#include "mmapper2exit.h"

using namespace std;

GroupAction::GroupAction(AbstractAction * action, const RoomSelection * selection) : executor(action)
{
  QMapIterator<uint, const Room*> roomIter(*selection);
  while(roomIter.hasNext())
  {
    uint rid = roomIter.next().key();
    affectedRooms.insert(rid);
    selectedRooms.push_back(rid);
  }
}

void AddTwoWayExit::exec() {
	if (room2Dir == UINT_MAX) room2Dir = opposite(dir);
	AddOneWayExit::exec();
	uint temp = to;
	to = from;
	from = temp;
	dir = room2Dir;
	AddOneWayExit::exec();
}



void RemoveTwoWayExit::exec() {
	if (room2Dir == UINT_MAX) room2Dir = opposite(dir);
	RemoveOneWayExit::exec();
	uint temp = to;
	to = from;
	from = temp;
	dir = room2Dir;
	RemoveOneWayExit::exec();
}


const std::set<uint> & GroupAction::getAffectedRooms()
{
  for(list<uint>::iterator i = selectedRooms.begin(); i != selectedRooms.end(); ++i)
  {
    executor->insertAffected(*i, affectedRooms);
  }
  return affectedRooms;
}

void GroupAction::exec()
{
  for(list<uint>::iterator i = selectedRooms.begin(); i != selectedRooms.end(); ++i)
  {
    executor->preExec(*i);
  }
  for(list<uint>::iterator i = selectedRooms.begin(); i != selectedRooms.end(); ++i)
  {
    executor->exec(*i);
  }
}

MoveRelative::MoveRelative(const Coordinate & in_move) :
    move(in_move)
{}

void MoveRelative::exec(uint id)
{
  Room * room = roomIndex()[id];
  if (room)
  {
    const Coordinate & c = room->getPosition();
    Coordinate newPos = c+move;
    map().setNearest(newPos, room);
  }
}

void MoveRelative::preExec(uint id)
{
  Room * room = roomIndex()[id];
  if (room)
  {
    map().remove(room->getPosition());
  }
}


MergeRelative::MergeRelative(const Coordinate & in_move) :
    move(in_move)
{}

void MergeRelative::insertAffected(uint id, set<uint> & affected)
{
  Room * room = roomIndex()[id];
  if (room)
  {
    ExitsAffecter::insertAffected(id, affected);
    const Coordinate & c = room->getPosition();
    Coordinate newPos = c+move;
    Room * other = map().get(newPos);
    if (other)
    {
      affected.insert(other->getId());
    }
  }
}

void MergeRelative::preExec(uint id)
{
  Room * room = roomIndex()[id];
  if (room)
  {
    map().remove(room->getPosition());
  }
}

void MergeRelative::exec(uint id)
{
  Room * source = roomIndex()[id];
  if (!source) return;
  Map & roomMap = map();
  vector<Room *> & rooms = roomIndex();
  const Coordinate & c = source->getPosition();
  Coordinate newPos = c+move;
  Room * target = roomMap.get(newPos);
  if (target)
  {
    factory()->update(target, source);
    ParseEvent * props = factory()->getEvent(target);
    uint oid = target->getId();
    RoomCollection * newHome = treeRoot().insertRoom(props);
    vector<RoomCollection *> & homes = roomHomes();
    RoomCollection * oldHome = homes[oid];
    if (oldHome) oldHome->erase(target);

    homes[oid] = newHome;
    if (newHome) newHome->insert(target);
    const ExitsList & exits = source->getExitsList();
    for(int dir = 0; dir < exits.size(); ++dir)
    {
      const Exit & e = exits[dir];
      for (set<uint>::const_iterator i = e.inBegin(); i != e.inEnd(); ++i)
      {
        uint oeid = *i;
        Room * oe = rooms[oeid];
        if (oe)
        {
          oe->exit(opposite(dir)).addOut(oid);
          target->exit(dir).addIn(oeid);
        }
      }
      for (set<uint>::const_iterator i = e.outBegin(); i != e.outEnd(); ++i)
      {
        uint oeid = *i;
        Room * oe = rooms[oeid];
        if (oe)
        {
          oe->exit(opposite(dir)).addIn(oid);
          target->exit(dir).addOut(oeid);
        }
      }
    }
    Remove::exec(source->getId());
  }
  else
  {
    Coordinate newPos = c+move;
    roomMap.setNearest(newPos, source);
  }
}


void ConnectToNeighbours::insertAffected(uint id, set<uint> & affected)
{
  Room * center = roomIndex()[id];
  if (center)
  {
    Coordinate other(0,-1,0);
    other += center->getPosition();
    Room * room = map().get(other);
    if (room) affected.insert(room->getId());
    other.y += 2;
    room = map().get(other);
    if (room) affected.insert(room->getId());
    other.y--;
    other.x--;
    room = map().get(other);
    if (room) affected.insert(room->getId());
    other.x += 2;
    room = map().get(other);
    if (room) affected.insert(room->getId());
  }
}

void ConnectToNeighbours::exec(uint cid)
{
  Room * center = roomIndex()[cid];
  if (center)
  {
    Coordinate other(0,-1,0);
    other += center->getPosition();
    connectRooms(center, other, ED_NORTH, cid);
    other.y += 2;
    connectRooms(center, other, ED_SOUTH, cid);
    other.y--;
    other.x--;
    connectRooms(center, other, ED_WEST, cid);
    other.x += 2;
    connectRooms(center, other, ED_EAST, cid);
  }
}

void ConnectToNeighbours::connectRooms(Room * center, Coordinate & otherPos, uint dir, uint cid)
{
  Room * room = map().get(otherPos);
  if (room)
  {
    uint oid = room->getId();
    Exit & oexit = room->exit(opposite(dir));
    oexit.addIn(cid);
    oexit.addOut(cid);
    Exit & cexit = center->exit(dir);
    cexit.addIn(oid);
    cexit.addOut(oid);
  }
}

void DisconnectFromNeighbours::exec(uint id)
{
  Room * room = roomIndex()[id];
  if (room)
  {
    ExitsList & exits = room->getExitsList();
    for (int dir = 0; dir < exits.size(); ++dir)
    {
      Exit & e = exits[dir];
      for (set<uint>::const_iterator in = e.inBegin(); in != e.inEnd(); ++in)
      {
        Room * other = roomIndex()[*in];
        if (other)
        {
          other->exit(opposite(dir)).removeOut(id);
        }
      }
      for (set<uint>::const_iterator out = e.outBegin(); out != e.outEnd(); ++out)
      {
        Room * other = roomIndex()[*out];
        if (other)
        {
          other->exit(opposite(dir)).removeIn(id);
        }
      }
      e.removeAll();
    }
  }
}

ModifyRoomFlags::ModifyRoomFlags(uint in_flags, uint in_fieldNum, FlagModifyMode in_mode) :
flags(in_flags), fieldNum(in_fieldNum), mode(in_mode) {}

void modifyFlags(uint & roomFlags, uint flags, FlagModifyMode mode)
{
  switch (mode)
  {
  case FMM_SET:
    roomFlags |= flags;
    break;
  case FMM_UNSET:
    roomFlags &= (~flags);
    break;
  case FMM_TOGGLE:
    roomFlags ^= flags;
    break;
  }
}

void ModifyRoomFlags::exec(uint id)
{
  Room * room = roomIndex()[id];
  if (room)
  {
    uint roomFlags = (*room)[fieldNum].toUInt();
    modifyFlags(roomFlags, flags, mode);
    (*room)[fieldNum] = roomFlags;
  }
}

UpdateExitField::UpdateExitField(const QVariant & in_update, uint in_dir, uint in_fieldNum) :
update(in_update), fieldNum(in_fieldNum), dir(in_dir) {}

void UpdateExitField::exec(uint id)
{
  Room * room = roomIndex()[id];
  if (room)
  {
    room->exit(dir)[fieldNum] = update;
  }
}

ModifyExitFlags::ModifyExitFlags(uint in_flags, uint in_dir, uint in_fieldNum, FlagModifyMode in_mode) :
flags(in_flags), fieldNum(in_fieldNum), mode(in_mode), dir(in_dir) {}

void ModifyExitFlags::exec(uint id)
{
  Room * room = roomIndex()[id];
  if (room)
  {
    uint exitFlags = room->exit(dir)[fieldNum].toUInt();
    modifyFlags(exitFlags, flags, mode);
    room->exit(dir)[fieldNum] = exitFlags;
  }
}
