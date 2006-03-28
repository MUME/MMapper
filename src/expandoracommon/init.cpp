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

#include "mapdata.h"
#include "pathmachine.h"
#include "parser.h"

const char * q2c(QString & s) {
  return s.toLatin1().constData();
}

void init() {

/*  Coordinate north(0,1,0);
  Coordinate::stdMoves[CID_NORTH] = north;
  Coordinate south(0,-1,0);
  Coordinate::stdMoves[CID_SOUTH] = south;
  Coordinate west(-1,0,0);
  Coordinate::stdMoves[CID_WEST] = west;
  Coordinate east(1,0,0);
  Coordinate::stdMoves[CID_EAST] = east;
  Coordinate up(0,0,1);
  Coordinate::stdMoves[CID_UP] = up;
  Coordinate down(0,0,-1);
  Coordinate::stdMoves[CID_DOWN] = down;
  Coordinate none(0,0,0);
  Coordinate::stdMoves[CID_LOOK] = none;
  Coordinate::stdMoves[CID_EXAMINE] = none; 
  Coordinate::stdMoves[CID_FLEE] = none; 
  Coordinate::stdMoves[CID_UNKNOWN] = none; 
  Coordinate::stdMoves[CID_NULL] = none; */

  PathMachine * machine = new PathMachine(false);
  MapData * data = new MapData;
  QObject::connect(machine, SIGNAL(addExit(int, int, uint )), data, SLOT(addExit(int, int, uint )));
  QObject::connect(machine, SIGNAL(createRoom(ParseEvent*, Coordinate )), data, SLOT(createRoom(ParseEvent*, Coordinate )));
  QObject::connect(machine, SIGNAL(lookingForRooms(RoomRecipient*, Coordinate )), data, SLOT(lookingForRooms(RoomRecipient*, Coordinate )));
  QObject::connect(machine, SIGNAL(lookingForRooms(RoomRecipient*, ParseEvent* )), data, SLOT(lookingForRooms(RoomRecipient*, ParseEvent* )));
  QObject::connect(machine, SIGNAL(lookingForRooms(RoomRecipient*, uint )), data, SLOT(lookingForRooms(RoomRecipient*, uint )));
  QObject::connect(data, SIGNAL(clearingMap()), machine, SLOT(releaseAllPaths()));
  //QObject::connect(..., machine, SLOT(deleteMostLikelyRoom()));
  //QObject::connect(machine, SIGNAL(playerMoved(Coordinate, Coordinate )), ....);
  //QObject::connect(... , machine, SLOT(event(ParseEvent* )));
  //QObject::connect(... , data, SLOT(insertPredefinedRoom(ParseEvent*, Coordinate, int )));
  //QObject::connect(... , data, SLOT(lookingForRooms(RoomRecipient*, Coordinate, Coordinate )));
  //...
  machine->start();
  data->start();
}
