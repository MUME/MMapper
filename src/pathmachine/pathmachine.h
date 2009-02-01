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

#ifndef PARSER
#define PARSER
#include <queue>
#include <list>
#include <queue>
#include <qobject.h>

#include "parseevent.h"
#include "property.h"
#include "path.h"
#include "room.h"
#include "coordinate.h"
#include "crossover.h"
#include "onebyone.h"
#include "approved.h"
#include "syncing.h"
#include "component.h"
#include "roomsignalhandler.h"
#include "abstractroomfactory.h"

#define APPROVED 0
#define EXPERIMENTING 1
#define SYNCING 2

class Approved;
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

