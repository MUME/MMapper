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

#include "syncing.h"

using namespace std;

Syncing::Syncing(PathParameters & in_p, std::list<Path *> * in_paths, RoomSignalHandler * in_signaler) :
    signaler(in_signaler),
    numPaths(0),
    params(in_p),
    paths(in_paths),
    parent(new Path(0,0,this,signaler))
{}

void Syncing::receiveRoom(RoomAdmin * sender, const Room * in_room)
{
  if (++numPaths > params.maxPaths)
  {
    if(!paths->empty())
    {
      for (list<Path *>::iterator i = paths->begin(); i != paths->end(); ++i)
      {
        (*i)->deny();
      }
      paths->clear();
      parent = 0;
    }
  }
  else
  {
    Path * p = new Path(in_room, sender, this, signaler, UINT_MAX - 1);
    p->setParent(parent);
    parent->insertChild(p);
    paths->push_back(p);
  }
}

list<Path *> * Syncing::evaluate()
{
  return paths;
}

Syncing::~Syncing() {
  if (parent) parent->deny();
}
