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

#include "oldconnection.h"
#include "olddoor.h"

Connection::Connection() : 
  m_type(CT_NORMAL),
  m_flags(0),
  m_index(0)
{
  m_rooms[LEFT] = NULL;
  m_rooms[RIGHT] = NULL;
  m_doors[LEFT] = NULL;
  m_doors[RIGHT] = NULL;
  m_directions[LEFT] = CD_UNKNOWN;
  m_directions[RIGHT] = CD_UNKNOWN;
}

Connection::~ Connection()
{ // no one keeps track of the doors so we have to remove them here
  delete m_doors[LEFT];
  delete m_doors[RIGHT];
}

ConnectionDirection opposite(ConnectionDirection in)
{

  switch (in)
  {
  case CD_NORTH: return CD_SOUTH;
  case CD_SOUTH: return CD_NORTH;
  case CD_WEST: return CD_EAST;
  case CD_EAST: return CD_WEST;
  case CD_UP: return CD_DOWN;
  case CD_DOWN: return CD_UP;
  default: return CD_UNKNOWN;
  }

}

