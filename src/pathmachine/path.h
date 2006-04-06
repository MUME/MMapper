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

#ifndef PATH
#define PATH

#include <set>
#include "roomsignalhandler.h"
#include "room.h"
#include "roomrecipient.h"
#include "roomadmin.h"
#include "pathparameters.h"
#include "abstractroomfactory.h"

class Path
{

public:
  Path(const Room * room, RoomAdmin * owner, RoomRecipient * locker, RoomSignalHandler * signaler, uint direction = UINT_MAX);
  void insertChild(Path * p);
  void removeChild(Path * p);
  void setParent(Path * p);
  bool hasChildren() {return (!children.empty());}
  const Room * const getRoom() {return room;}

  //new Path is created, distance between rooms is calculated and probability is set accordingly
  Path * fork(const Room * room, Coordinate & expectedCoordinate, RoomAdmin * owner, PathParameters params, RoomRecipient * locker, uint dir, AbstractRoomFactory * factory);
  double getProb() {return probability;}
  void approve();

  // deletes this path and all parents up to the next branch
  void deny();
  void setProb(double p) {probability = p;}

  Path * getParent() {return parent;}

private:
  Path * parent;
  std::set<Path *> children;
  double probability;
  const Room * room; // in fact a path only has one room, one parent and some children (forks).
  RoomSignalHandler * signaler;
  uint dir;
  ~Path() {}}
;


#endif

#ifdef DMALLOC
#include <mpatrol.h>
#endif
