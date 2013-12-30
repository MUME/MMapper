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

#include "defs.h"
#include "parser.h"
#include "mmapper2event.h"
#include "property.h"
#include "parseevent.h"

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


