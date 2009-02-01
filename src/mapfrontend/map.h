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

#ifndef MAP
#define MAP

#include "coordinate.h"
#include "room.h"
#include "roomcollection.h"
#include "abstractroomfactory.h"
#include <map>
#include "roomoutstream.h"

/**
 * The Map stores the geographic relations of rooms to each other
 * it doesn't store the search tree. The Map class is only used by the
 * RoomAdmin, which also stores the search tree
 */
class Map
{
  public:
    bool defined(const Coordinate &c);
    Coordinate setNearest(const Coordinate &c, Room *room);
    Room * get(const Coordinate &c);
    void remove(const Coordinate & c);
    void clear();
    void getRooms(RoomOutStream & stream, const Coordinate & ulf, const Coordinate & lrb);
    void fillArea(AbstractRoomFactory * factory, const Coordinate & ulf, const Coordinate & lrb);

  private:
    void set(const Coordinate &c, Room *room);
    Coordinate getNearestFree(const Coordinate & c);
    std::map<int, std::map<int, std::map<int, Room *> > > m_map;
};

class CoordinateIterator
{
  public:
    CoordinateIterator() : threshold(1), state(7) {}
    Coordinate & next();
  private:
    Coordinate c;
    int threshold;
    int state;
};

#endif
