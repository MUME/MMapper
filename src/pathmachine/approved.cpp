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

#include "approved.h"
#include "mapaction.h"

void Approved::receiveRoom(RoomAdmin * sender, const Room * perhaps)
{
  if (matchedRoom == 0)
  {
    ComparisonResult indicator = factory->compare(perhaps, myEvent, matchingTolerance);
    if (indicator != CR_DIFFERENT)
    {
      matchedRoom = perhaps;
      owner = sender;
      if (indicator == CR_TOLERANCE && myEvent->getNumSkipped() == 0)
	update = true;
    }
    else
    {
      sender->releaseRoom(this, perhaps->getId());
    }
  }
  else
  {
    moreThanOne = true;
    sender->releaseRoom(this, perhaps->getId());
  }
}

Approved::~Approved()
{
  if(owner)
  {
    if (moreThanOne)
    {
      owner->releaseRoom(this, matchedRoom->getId());
    }
    else
    {
      owner->keepRoom(this, matchedRoom->getId());
      if (update)
	owner->scheduleAction(new SingleRoomAction(new Update( myEvent), matchedRoom->getId()));
    }
  }
}


Approved::Approved(AbstractRoomFactory * in_factory, ParseEvent * event, int tolerance) :
    matchedRoom(0),
    myEvent(event),
    matchingTolerance(tolerance),
    owner(0),
    moreThanOne(false),
    update(false),
    factory(in_factory)
{}


const Room * Approved::oneMatch()
{
  if (moreThanOne)
    return 0;
  else
    return matchedRoom;
}

void Approved::reset()
{
  if(matchedRoom)
  {
    owner->releaseRoom(this, matchedRoom->getId());
  }
  update = false;
  matchedRoom = 0;
  moreThanOne = false;
  owner = 0;
}



RoomAdmin * Approved::getOwner()
{
  if (moreThanOne)
    return 0;
  else
    return owner;
}
