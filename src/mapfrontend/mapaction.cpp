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

#include "mapaction.h"
#include <assert.h>

using namespace std;

SingleRoomAction::SingleRoomAction(AbstractAction * ex, uint in_id) : id(in_id), executor(ex) {}

const std::set<uint> & SingleRoomAction::getAffectedRooms() {
  executor->insertAffected(id, affectedRooms);
  return affectedRooms;
}

AddExit::AddExit(uint in_from, uint in_to, uint in_dir) :
    from(in_from), to(in_to), dir(in_dir)
{
  affectedRooms.insert(from);
  affectedRooms.insert(to);
}

void AddExit::exec()
{
	tryExec();
}

Room * AddExit::tryExec() {
	vector<Room *> & rooms = roomIndex();
	Room * rfrom = rooms[from];
	if (!rfrom) return 0;
	Room * rto = rooms[to];
	if (!rto) return 0;

	rfrom->exit(dir).addOut(to);
	rto->exit(factory()->opposite(dir)).addIn(from);
	return rfrom;
}

RemoveExit::RemoveExit(uint in_from, uint in_to, uint in_dir) :
    from(in_from), to(in_to), dir(in_dir)
{
  affectedRooms.insert(from);
  affectedRooms.insert(to);
}

void RemoveExit::exec() {
	tryExec();
}

Room * RemoveExit::tryExec()
{
  vector<Room *> & rooms = roomIndex();
  Room * rfrom = rooms[from];
  if (rfrom) rfrom->exit(dir).removeOut(to);
  Room * rto = rooms[to];
  if (rto) rto->exit(factory()->opposite(dir)).removeIn(from);
	return rfrom;
}

void MakePermanent::exec(uint id) {
  Room * room = roomIndex()[id];
  if (room) room->setPermanent();
}

UpdateRoomField::UpdateRoomField(const QVariant & in_update, uint in_fieldNum) :
update(in_update), fieldNum(in_fieldNum) {}

void UpdateRoomField::exec(uint id)
{
  Room * room = roomIndex()[id];
  if (room)
  {
    (*room)[fieldNum] = update;
  }
}

Update::Update(ParseEvent * in_props) :
  props(*in_props) {
  //assert(props.getNumSkipped() == 0);
}

UpdatePartial::UpdatePartial(const QVariant & in_val, uint in_pos) : Update(0), UpdateRoomField(in_val, in_pos)  {}

void UpdatePartial::exec(uint id) {
	Room * room = roomIndex()[id];
	if (room) {
		UpdateRoomField::exec(id);
		props = *(factory()->getEvent(room));
		Update::exec(id);
	}
}

void Update::exec(uint id) {
  Room * room = roomIndex()[id];
  if (room) {
    factory()->update(room, &props);
    RoomCollection * newHome = treeRoot().insertRoom(&props);
    vector<RoomCollection *> & homes = roomHomes();
    RoomCollection * oldHome = homes[id];
    if (oldHome) oldHome->erase(room);
    homes[id] = newHome;
    newHome->insert(room);
  }
}


void ExitsAffecter::insertAffected(uint id, std::set<uint> & affected) {
  Room * room = roomIndex()[id];
  if (room) {
    affected.insert(id);
    const ExitsList & exits = room->getExitsList();
    ExitsList::const_iterator exitIter = exits.begin();
    while(exitIter != exits.end()) {
      const Exit & e = *exitIter;
      for (set<uint>::const_iterator i = e.inBegin(); i != e.inEnd(); ++i) 
	affected.insert(*i);
      for (set<uint>::const_iterator i = e.outBegin(); i != e.outEnd(); ++i) 
	affected.insert(*i);
      exitIter++;
    }
  }
}


void Remove::exec(uint id) {
  vector<Room *> & rooms = roomIndex();
  Room * room = rooms[id];
  if (!room) return;
  map().remove(room->getPosition());
  RoomCollection * home = roomHomes()[id];
  if (home) home->erase(room);
  rooms[id] = 0;
  // don't return previously used ids for now
  // unusedIds().push(id);
  const ExitsList & exits = room->getExitsList();
  for(int dir = 0; dir < exits.size(); ++dir) {
    const Exit & e = exits[dir];
    for (set<uint>::const_iterator i = e.inBegin(); i != e.inEnd(); ++i) {
      Room * other = rooms[*i];
      if (other) other->exit(factory()->opposite(dir)).removeOut(id);
    }
    for (set<uint>::const_iterator i = e.outBegin(); i != e.outEnd(); ++i) { 
      Room * other = rooms[*i];
      if (other) other->exit(factory()->opposite(dir)).removeIn(id);
    }
  }
  delete room;
}


