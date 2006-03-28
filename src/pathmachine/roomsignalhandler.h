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

#ifndef ROOMSIGNALHANDLER
#define ROOMSIGNALHANDLER

#include <map>
#include <set>
#include <qobject.h>
#include "room.h"
#include "roomrecipient.h"
#include "roomadmin.h"
#include "mapaction.h"

class RoomSignalHandler : public QObject
{
  Q_OBJECT
private:

  std::map<const Room *, RoomAdmin *> owners;
  std::map<const Room *, std::set<RoomRecipient *> > lockers;
  std::map<const Room *, int> holdCount;


public:
RoomSignalHandler(QObject * parent) : QObject(parent) {}
  /* receiving from our clients: */
  // hold the room, we don't know yet what to do, overrides release, re-caches if room was un-cached
  void hold(const Room * room, RoomAdmin * owner, RoomRecipient * locker);
  // room isn't needed anymore and can be deleted if no one else is holding it and no one else uncached it
  void release(const Room * room);
  // keep the room but un-cache it - overrides both hold and release
  // toId is negative if no exit should be added, else it's the id of
  // the room where the exit should lead
  void keep(const Room * room, uint dir, uint fromId);

  /* Sending to the rooms' owners:
     keepRoom: keep the room, but we don't need it anymore for now
     releaseRoom: delete the room, if you like */

  int getNumLockers(const Room * room) {return lockers[room].size();}

signals:
  void scheduleAction(MapAction *);
};
#ifdef DMALLOC
#include <mpatrol.h>
#endif
#endif
