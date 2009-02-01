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

#include "defs.h"
#include "parser.h"
#include "mmapper2event.h"

using namespace std;

ParseEvent * createEvent(const CommandIdType & c, const QString & roomName, const QString & roomDesc, const QString & parsedRoomDesc, const ExitsFlagsType & exitFlags, const PromptFlagsType & promptFlags)
{
  
  ParseEvent * event = new ParseEvent(c);
  deque<QVariant> & optional = event->getOptional();

  if (roomName.isNull())
  {
    event->push_back(new Property(true));
  }
  else {
    event->push_back(new Property(roomName.toLatin1()));
  }
  optional.push_back(roomName);
  optional.push_back(roomDesc);
  
  if (parsedRoomDesc.isNull())
  {
    event->push_back(new Property(true));
  }
  else 
    event->push_back(new Property(parsedRoomDesc.toLatin1()));
  optional.push_back(parsedRoomDesc);
  optional.push_back((uint)exitFlags);

  if (promptFlags & PROMPT_FLAGS_VALID)
  {
    char terrain = 0;
    terrain += (promptFlags & 15); //bit0-3 -> char representation of RoomTerrainType
    event->push_back(new Property(QByteArray(1, terrain)));
  }
  else
  {
    event->push_back(new Property(true));
  }
  optional.push_back((uint)promptFlags);
  event->countSkipped();
  return event;
}


QString getRoomName(const ParseEvent * e) 
  {return e->getOptional()[EV_NAME].toString();}
QString getRoomDesc(const ParseEvent * e)
  {return e->getOptional()[EV_DESC].toString();}
QString getParsedRoomDesc(const ParseEvent * e)
  {return e->getOptional()[EV_PDESC].toString();}
ExitsFlagsType getExitFlags(const ParseEvent * e)
  {return e->getOptional()[EV_EXITS].toUInt();}
PromptFlagsType getPromptFlags(const ParseEvent * e)
  {return e->getOptional()[EV_PROMPT].toUInt();}


