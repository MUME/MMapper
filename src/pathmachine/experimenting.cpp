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

#include "pathmachine.h"
#include "experimenting.h"

using namespace std;

Experimenting::Experimenting(list<Path *> * pat, uint in_dirCode, PathParameters & in_params, AbstractRoomFactory * in_factory) :
    direction(in_factory->exitDir(in_dirCode)), dirCode(in_dirCode), shortPaths(pat),
    paths(new list<Path *>), best(0), second(0),
    params(in_params), numPaths(0), factory(in_factory)
{}

void Experimenting::augmentPath(Path * path, RoomAdmin * map, const Room * room)
{
  Coordinate c = path->getRoom()->getPosition() + direction;
  Path * working = path->fork(room, c, map, params, this, dirCode, factory);
  if (best == 0) best = working;
  else if (working->getProb() > best->getProb())
  {
    paths->push_back(best);
    second = best;
    best = working;
  }
  else
  {
    if (second == 0 || working->getProb() > second->getProb())
      second = working;
    paths->push_back(working);
  }
  numPaths++;
}


list<Path *> * Experimenting::evaluate()
{
  Path * working = 0;
  while(!shortPaths->empty())
  {
    working = shortPaths->front();
    shortPaths->pop_front();
    if (!(working->hasChildren())) working->deny();
  }

  if (best != 0)
  {
    if (second == 0 || best->getProb() > second->getProb()*params.acceptBestRelative || best->getProb() > second->getProb()+params.acceptBestAbsolute)
    {
      for (list<Path *>::iterator i = paths->begin(); i != paths->end(); ++i)
      {
        (*i)->deny();
      }
      paths->clear();
      paths->push_front(best);
    }
    else
    {
      paths->push_back(best);
      Path * working = paths->front();
      while (working != best) 
      {
	paths->pop_front();
	// throw away if the probability is very low or not
	// distinguishable from best. Don't keep paths with equal
	// probability at the front, for we need to find a unique
	// best path eventually.
        if ( best->getProb() > working->getProb()*params.maxPaths/numPaths || ((!(best->getProb() > working->getProb())) && best->getRoom() == working->getRoom()))
        {
          working->deny();
        }
        else paths->push_back(working);
        working = paths->front();
      }
    }
  }
  second = 0;
  delete shortPaths;
  shortPaths = 0;
  best = 0;
  return paths;
}

