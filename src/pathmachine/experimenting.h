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

#ifndef EXPERIMENTING_H
#define EXPERIMENTING_H

#include "room.h"
#include "path.h"
#include "roomadmin.h"
#include "roomrecipient.h"
#include "pathparameters.h"

class PathMachine;

class Experimenting : public RoomRecipient {
 protected:
  void augmentPath(Path * path, RoomAdmin * map, const Room * room);
  Coordinate direction;
  uint dirCode;
  std::list<Path *> * shortPaths;
  std::list<Path *> * paths;
  Path * best;
  Path * second;
  PathParameters & params;
  double numPaths;
  AbstractRoomFactory * factory;
  

 public:
  Experimenting(std::list<Path *> * paths, uint dirCode, PathParameters & params, AbstractRoomFactory * factory);
  std::list<Path *> * evaluate();
  virtual void receiveRoom(RoomAdmin *, const Room *) = 0;

};

#endif

