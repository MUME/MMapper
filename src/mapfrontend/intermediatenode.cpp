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

#include "intermediatenode.h"

using namespace std;

IntermediateNode::IntermediateNode(ParseEvent * event)
{
  Property * prop = event->next();

  if (prop == 0 || prop->isSkipped())
  {
    myChars = new char[1];
    myChars[0] = 0;
  }
  else
  {
    myChars = new char[strlen(prop->rest()) + 1];
    strcpy(myChars, prop->rest());
  }
  rooms = 0;
  event->prev();
}

RoomCollection * IntermediateNode::insertRoom(ParseEvent * event)
{

  if (event->next() == 0)
  {
    if (rooms == 0) rooms = new RoomCollection;
    return rooms;
  }
  else if (event->current()->isSkipped()) return 0;

  return SearchTreeNode::insertRoom(event);
}


void IntermediateNode::getRooms(RoomOutStream & stream, ParseEvent * event)
{
  if (event->next() == 0)
  {
    for (set<Room *>::iterator i = rooms->begin(); i != rooms->end(); ++i)
      stream << *i;
  }
  else if (event->current()->isSkipped()) 
    SearchTreeNode::skipDown(stream, event);
  else
  {
    SearchTreeNode::getRooms(stream, event);
  }
}

void IntermediateNode::skipDown(RoomOutStream & stream, ParseEvent * event)
{
  getRooms(stream, event);
}
