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

#include "mmapper2pathmachine.h"
#include "mmapper2event.h"
#include "roomfactory.h"
//#include "parser.h"
#include "configuration.h"

#ifdef MODULAR
extern "C" MY_EXPORT Component * createComponent()
{
  return new Mmapper2PathMachine;
}
#else
Initializer<Mmapper2PathMachine> mmapper2PathMachine("Mmapper2PathMachine");
#endif

void Mmapper2PathMachine::event(ParseEvent * event)
{
  params.acceptBestRelative = config.m_acceptBestRelative;
  params.acceptBestAbsolute = config.m_acceptBestAbsolute;
  params.newRoomPenalty = config.m_newRoomPenalty;
  params.correctPositionBonus = config.m_correctPositionBonus;
  params.maxPaths = config.m_maxPaths;
  params.matchingTolerance = config.m_matchingTolerance;
	params.multipleConnectionsPenalty = config.m_multipleConnectionsPenalty;
	
  QString stringState = "received event, state: ";
  if (state == EXPERIMENTING) stringState += "EXPERIMENTING";
  else if (state == SYNCING) stringState += "SYNCING";
  else stringState += "APPROVED";
  QString me("PathMachine");
  emit log(me, stringState);
  PathMachine::event(event);
  stringState = "done processing event, state: ";
  if (state == EXPERIMENTING) stringState += "EXPERIMENTING";
  else if (state == SYNCING) stringState += "SYNCING";
  else stringState += "APPROVED";
  emit log(me, stringState);
}

Mmapper2PathMachine::Mmapper2PathMachine() : PathMachine(new RoomFactory, false), config(Config())
{}

