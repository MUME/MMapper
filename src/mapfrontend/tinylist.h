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

#ifndef TINYLIST
#define TINYLIST
/**
 * extremely shrinked array list implementation to be used for each room
 * because we have so many rooms and want to search them really fast, we
 *	- allocate as little memory as possible
 *	- allow only 128 elements (1 per character)
 *	- only need 3 lines to access an element
 */

template <class T>
class TinyList
{
public:
  TinyList() : list(0), listSize(0) {}

  virtual ~TinyList()
  {
    if (list) delete[] list;
  }

  virtual T get(unsigned char c)
  {
    if (c >= listSize) return 0;
    else return list[c];
  }
  
  virtual void put(unsigned char c, T object)
  {
    if (c >= listSize)
    {
      unsigned char i;
      T * nlist = new T[c+2];
      for (i = 0; i < listSize; i++) nlist[i] = list[i];
      for (; i < c; i++) nlist[i] = 0;
      if (list) delete[] list;
      list = nlist;
      listSize = c+1;
      list[listSize] = 0;
    }
    list[c] = object;
  }
  
  virtual void remove(unsigned char c)
  {
    if (c < listSize) list[c] = 0;
  }
  
  virtual unsigned char size()
  {
    return listSize;
  }

protected:
  T * list;
  unsigned char listSize;
};

#ifdef DMALLOC
#include <mpatrol.h>
#endif
#endif
