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

#ifndef MAPFRONTEND_H
#define MAPFRONTEND_H

#include <qmutex.h>
#include <vector>
#include <stack>
#include <set>
#include <list>
#include <map>

#include "intermediatenode.h"
#include "component.h"
#include "frustum.h"
#include "room.h"
#include "map.h"
#include "roomrecipient.h"
#include "roomadmin.h"
#include "abstractroomfactory.h"

/**
 * The MapFrontend organizes rooms and their relations to each other.
 */
 
class MapAction;


class MapFrontend : public Component, public RoomAdmin
{
Q_OBJECT
friend class FrontendAccessor;

protected:
  
  IntermediateNode treeRoot;
  Map map;
  std::vector<Room *> roomIndex;
  std::stack<uint>  unusedIds;
  std::map<uint, std::set<MapAction *> > actionSchedule;
  std::vector<RoomCollection *> roomHomes;
  std::vector<std::set<RoomRecipient *> > locks;
  
  uint greatestUsedId;
  QMutex mapLock;
  Coordinate ulf;
  Coordinate lrb;
  AbstractRoomFactory * factory;

  void executeActions(uint roomId);
  void executeAction(MapAction * action);
  bool isExecutable(MapAction * action);
  void removeAction(MapAction * action);

  virtual uint assignId(Room * room, RoomCollection * roomHome);
  virtual void checkSize(const Coordinate &);
  
  
public:
  MapFrontend(AbstractRoomFactory * factory);
  virtual ~MapFrontend();
  virtual void clear();
  void block();
  void unblock();
  virtual void checkSize();
  virtual Qt::ConnectionType requiredConnectionType(const QString &) {return Qt::DirectConnection;}
  // removes the lock on a room
  // after the last lock is removed, the room is deleted
  virtual void releaseRoom(RoomRecipient *, uint);

  // makes a lock on a room permanent and anonymous.
  // Like that the room can't be deleted via releaseRoom anymore.
  virtual void keepRoom(RoomRecipient *, uint);

  virtual void lockRoom(RoomRecipient *, uint);

  virtual uint createEmptyRoom(const Coordinate &);

  virtual void insertPredefinedRoom(Room *);

public slots:
  // looking for rooms leads to a bunch of foundRoom() signals
  virtual void lookingForRooms(RoomRecipient *,ParseEvent *);
  virtual void lookingForRooms(RoomRecipient *,uint); // by id
  virtual void lookingForRooms(RoomRecipient *,const Coordinate &);
  virtual void lookingForRooms(RoomRecipient *,Frustum *);
  virtual void lookingForRooms(RoomRecipient *,const Coordinate &,const Coordinate &); // by bounding box

  // createRoom creates a room without a lock
  // it will get deleted if no one looks for it for a certain time
  virtual void createRoom(ParseEvent *, const Coordinate &);  
  virtual void createEmptyRooms(const Coordinate &,const Coordinate &);

  virtual void scheduleAction(MapAction * action);
  
signals:

  // this signal is sent out if a room is deleted. So any clients still
  // working on this room can start some emergency action.
  void mapSizeChanged(const Coordinate &, const Coordinate &);
  void clearingMap();
};


#endif
