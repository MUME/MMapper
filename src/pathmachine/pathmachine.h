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

#ifndef PARSER
#define PARSER

//#include "parseevent.h"
//#include "property.h"
//#include "path.h"
#include "room.h"
//#include "coordinate.h"
//#include "crossover.h"
//#include "onebyone.h"
//#include "approved.h"
//#include "syncing.h"
#include "component.h"
#include "roomsignalhandler.h"
//#include "abstractroomfactory.h"
#include "pathparameters.h"

//#include <queue>
//#include <list>
//#include <queue>
//#include <qobject.h>

#define APPROVED 0
#define EXPERIMENTING 1
#define SYNCING 2

class Approved;
class Path;
class ParseEvent;
class AbstractRoomFactory;

/**
 * the parser determines the relations between incoming move- and room-events
 * and decides if rooms have to be added (and where) and where the player is
 * the results are published via signals
 */
class PathMachine : public Component
{
  Q_OBJECT
public slots:
  virtual void event(ParseEvent *);
  virtual void deleteMostLikelyRoom();
  virtual void releaseAllPaths();
  virtual void retry();
  virtual void setCurrentRoom(uint id);
  virtual void setCurrentRoom(const Coordinate & pos);

signals:
  void lookingForRooms(RoomRecipient *,ParseEvent *);
  void lookingForRooms(RoomRecipient *,uint);
  void lookingForRooms(RoomRecipient *,const Coordinate &);
  void playerMoved(const Coordinate &);
  void createRoom(ParseEvent *, const Coordinate &);
  void scheduleAction(MapAction *);
  void setCharPosition(uint id);

public:
  PathMachine(AbstractRoomFactory * factory, bool threaded = true);
  virtual void init();
  virtual Qt::ConnectionType requiredConnectionType(const QString &);

protected:
  PathParameters params;
  void experimenting(ParseEvent * event);
  void syncing(ParseEvent * event);
  void approved(ParseEvent * event);
  void evaluatePaths();
  void setCurrentRoom(Approved * app);
  void tryExits(const Room *, RoomRecipient *, ParseEvent *, bool out);
  void tryExit(const Exit & possible, RoomRecipient * recipient, bool out);
  void tryCoordinate(const Room *, RoomRecipient *, ParseEvent *);
  AbstractRoomFactory * factory;

  RoomSignalHandler signaler;
  Room pathRoot;
  Room mostLikelyRoom;
  ParseEvent * lastEvent;
  char state;
  std::list<Path *> * paths;

}
;


#endif

