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

#ifndef MMAPPER2EVENT_H
#define MMAPPER2EVENT_H

#include "abstractparser.h"

class QString;
class ParseEvent;

#define EV_NAME 0
#define EV_DESC 1
#define EV_PDESC 2
#define EV_EXITS 3
#define EV_PROMPT 4

ParseEvent * createEvent(const CommandIdType & c, const QString & roomName, const QString & roomDesc, 
                         const QString & parsedRoomDesc, const ExitsFlagsType & exitFlags, 
                         const PromptFlagsType & promptFlags);

QString getRoomName(const ParseEvent * e);

QString getRoomDesc(const ParseEvent * e);
  
QString getParsedRoomDesc(const ParseEvent * e);
  
ExitsFlagsType getExitFlags(const ParseEvent * e);
  
PromptFlagsType getPromptFlags(const ParseEvent * e);
  

#endif
