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

#include "roomsaver.h"
#include <assert.h>

RoomSaver::RoomSaver(RoomAdmin * in_admin, QList<const Room *> & list) :
roomsCount(0), roomList(list), admin(in_admin) {}


void RoomSaver::receiveRoom(RoomAdmin * in_admin, const Room * room)
{
  assert(in_admin == admin);
  if (room->isTemporary()) {
	admin->releaseRoom(this, room->getId());
  }
  else {
  	roomList.append(room);
  	roomsCount++;
  }
}

quint32 RoomSaver::getRoomsCount()
{
  return roomsCount;
}

RoomSaver::~RoomSaver()
{
  for(int i = 0; i < roomList.size(); ++i)
  {
    const Room * room = roomList[i];
    if (room)
    {
      admin->releaseRoom(this, room->getId());
      roomList[i] = 0;
    }
  }
}
