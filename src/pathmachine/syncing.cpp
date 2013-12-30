/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper project. 
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "syncing.h"
#include "pathparameters.h"
#include "path.h"
#include "roomadmin.h"
#include "roomsignalhandler.h"

#include <limits.h>

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
