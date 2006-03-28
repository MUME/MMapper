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

#include "roomsignalhandler.h"
#include <assert.h>

using namespace Qt;
using namespace std;

void RoomSignalHandler::hold(const Room * room, RoomAdmin * owner, RoomRecipient * locker)
{
  owners[room] = owner;
  if (lockers[room].empty()) holdCount[room] = 0;
  lockers[room].insert(locker);
  ++holdCount[room];
}

void RoomSignalHandler::release(const Room * room)
{
  assert(holdCount[room]);
  if (--holdCount[room] == 0)
  {
    RoomAdmin * rcv = owners[room];

    for(set<RoomRecipient *>::iterator i = lockers[room].begin(); i != lockers[room].end(); ++i)
    {
      if (*i) rcv->releaseRoom(*i, room->getId());
    }

    lockers.erase(room);
    owners.erase(room);
  }
}

void RoomSignalHandler::keep(const Room * room, uint dir, uint fromId)
{
  assert(holdCount[room]);

  RoomAdmin * rcv = owners[room];

  if ((uint)room->getExitsList().size() > dir)
  {
    emit scheduleAction(new AddExit(fromId, room->getId(), dir));
  }

  if (!lockers[room].empty())
  {
    RoomRecipient * locker = *(lockers[room].begin());
    rcv->keepRoom(locker, room->getId());
    lockers[room].erase(locker);
  }
  release(room);
}
