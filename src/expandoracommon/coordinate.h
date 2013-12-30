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

#ifndef COORDINATE
#define COORDINATE

class Coordinate
{
public:
  bool operator== (const Coordinate & other) const;
  bool operator!= (const Coordinate & other) const;
  void operator+= (const Coordinate & other);
  void operator-= (const Coordinate & other);
  Coordinate operator+ (const Coordinate & other) const;
  Coordinate operator- (const Coordinate & other) const;

  int distance(const Coordinate & other) const;
  void clear();
  Coordinate(int in_x = 0, int in_y = 0, int in_z = 0) : x(in_x), y(in_y), z(in_z) {}
  bool isNull() const {return (x == 0 && y == 0 && z == 0);}

  int x;
  int y;
  int z;
};

#ifdef DMALLOC
#include <mpatrol.h>
#endif
#endif


