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

#include "onebyone.h"

using namespace std;

OneByOne::OneByOne(AbstractRoomFactory * in_factory, ParseEvent * in_event, PathParameters & in_params, RoomSignalHandler * in_handler) : Experimenting(new list<Path*>, in_event->getMoveType(), in_params, in_factory), event(in_event), handler(in_handler)
{}

void OneByOne::receiveRoom(RoomAdmin * admin, const Room * room)
{
  if (factory->compare(room, event, params.matchingTolerance) == CR_EQUAL)
    augmentPath(shortPaths->back(), admin, room);
  else { 
    // needed because the memory address is not unique and 
    // calling admin->release might destroy a room still held by some path
    handler->hold(room, admin, this);
    handler->release(room);
  }
}

void OneByOne::addPath(Path * path)
{
  shortPaths->push_back(path);
}
